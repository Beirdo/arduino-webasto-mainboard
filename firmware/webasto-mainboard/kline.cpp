
#include <cppQueue.h>
#include <Arduino.h>
#include <pico.h>
#include <stdlib.h>
#include <string.h>

#include "project.h"
#include "kline_packet.h"
#include "kline.h"
#include "webasto.h"

#define KLINE_RX_MATCH_ADDR 0xF4
#define KLINE_TX_ADDR       0x4F

#define KLINE_FIFO_SIZE   16
#define KLINE_BUFFER_SIZE 16

int tx_active;
int rx_active;

uint8_t kline_rx_buffer[KLINE_BUFFER_SIZE];
int kline_rx_tail;

cppQueue kline_rx_q(sizeof(klinePacket_t *), KLINE_FIFO_SIZE, FIFO);
cppQueue kline_tx_q(sizeof(klinePacket_t *), KLINE_FIFO_SIZE, FIFO);

void init_kline(void)
{
  Serial2.setRX(PIN_SERIAL2_RX);
  Serial2.setTX(PIN_SERIAL2_TX);
  Serial2.begin(2400, SERIAL_8E1);
  Serial2.flush();

  pinMode(PIN_KLINE_EN, OUTPUT);
  digitalWrite(PIN_KLINE_EN, LOW);
  tx_active = false;
  rx_active = false;
  kline_rx_tail = 0;  
}

void send_break(void)
{
  Serial2.end();

  // Send a break - may not be needed as we are the slave, not the master.
  digitalWrite(PIN_SERIAL2_TX, HIGH);
  pinMode(PIN_SERIAL2_TX, OUTPUT);
  digitalWrite(PIN_SERIAL2_TX, LOW);
  delay(50);
  digitalWrite(PIN_SERIAL2_TX, HIGH);
  delay(50);
  pinMode(PIN_SERIAL2_TX, INPUT);

  Serial2.setRX(PIN_SERIAL2_RX);
  Serial2.setTX(PIN_SERIAL2_TX);
  Serial2.begin(2400, SERIAL_8E1);
  Serial2.flush();
}

void kline_send_response(klinePacket_t *respPacket)
{
  if (!respPacket) {
    return;
  }

  kline_tx_q.push((const klinePacket_t *)&respPacket);

  if (!tx_active) {
    kline_send_next_packet();
  }
}

void kline_send_next_packet(void)
{
  if (tx_active) {
    return;
  }

  klinePacket_t *packet;
  kline_tx_q.pop(&packet);

  if (!packet) {
    return;
  }

  int len = packet->len;
  
  tx_active = true;
  digitalWrite(PIN_KLINE_EN, HIGH);
  // send_break();
  Serial2.write(packet->buf, len);
  digitalWrite(PIN_KLINE_EN, LOW);
  tx_active = false;

  free(packet->buf);
  free(packet);
}

uint8_t calc_kline_checksum(uint8_t *buf, int len)
{
  uint8_t checksum = 0x00;
  for (int i = 0; i < len; i++) {
    checksum ^= buf[i];
  }
  return checksum;
}

void process_kline(void)
{
  if (Serial2.getBreakReceived()) {
    rx_active = true;
    kline_rx_tail = 0;
    Serial2.flush();
  }

  while(Serial2 && Serial2.available()) {
    if (!rx_active) {
      Serial2.flush();
      continue;
    }
    
    uint8_t ch = Serial2.read();
    if (kline_rx_tail == 0 && ch != KLINE_RX_MATCH_ADDR) {
      rx_active = false;
      Serial2.flush();
      continue;
    }

    kline_rx_buffer[kline_rx_tail++] = ch;
    if (kline_rx_tail > 2) {
      int len = kline_rx_buffer[1] + 2;
      if (kline_rx_tail == len) {
        // Full frame received
        if (calc_kline_checksum(kline_rx_buffer, len)) {
          // Bad checksum.  Turf it.  Wait for new break
          rx_active = false;
          Serial2.flush();
          continue;          
        }

        uint8_t *buf = (uint8_t *)malloc(len);
        memcpy(buf, (const void *)kline_rx_buffer, len);                
        klinePacket_t *packet = (klinePacket_t *)malloc(sizeof(klinePacket_t));
        packet->buf = buf;
        packet->len = len;

        kline_rx_q.push(&packet);
        kline_rx_tail = 0;        
      }
    }            
  }

  while(!kline_rx_q.isEmpty()) {
    klinePacket_t *packet;
    kline_rx_q.pop(&packet);
    if (!packet) {
      continue;
    }

    uint8_t cmd = packet->buf[2];
    klinePacket_t *respPacket = kline_rx_dispatch(packet, cmd);
    if (respPacket) {
      kline_send_response(respPacket);
    }

    free(packet->buf);
    free(packet);
  }    

  if (!tx_active) {
    kline_send_next_packet();
  }
}

klinePacket_t *kline_rx_dispatch(klinePacket_t *packet, uint8_t cmd)
{
  if (!packet) {
    return 0;
  }

  int len;
  klinePacket_t *respPacket = 0;
  uint8_t *buf = 0;  

  switch(cmd) {
    case 0x10:
      // Shutdown, no data
      buf = kline_command_shutdown();
      break;

    case 0x20:
      // Start for x minutes, default mode
      buf = kline_command_timed_start(WEBASTO_MODE_DEFAULT, packet->buf[3]);
      break;      

    case 0x21:
      // Start for x minutes, parking heater on
      buf = kline_command_timed_start(WEBASTO_MODE_PARKING_HEATER, packet->buf[3]);
      break;      
      
    case 0x22:
      // Start for x minutes, ventilation on
      buf = kline_command_timed_start(WEBASTO_MODE_VENTILATION, packet->buf[3]);
      break;      

    case 0x23:
      // Start for x minutes, supplemental heating on
      buf = kline_command_timed_start(WEBASTO_MODE_SUPPLEMENTAL_HEATER, packet->buf[3]);
      break;      

    case 0x24:
      // Start for x minutes, circulation pump on
      buf = kline_command_timed_start(WEBASTO_MODE_CIRCULATION_PUMP, packet->buf[3]);
      break;      
      
    case 0x25:
      // Start for x minutes, boost on
      buf = kline_command_timed_start(WEBASTO_MODE_BOOST, packet->buf[3]);
      break;  

    case 0x38:
      // Diagnostic message, unknown use
      buf = kline_command_diagnostics();
      break;  

    case 0x44:
      // Seems to be a timer keepalive, I'm going to assume it adds the same number of minutes originally requested
      // and returns a 16-bit "minutes remaining" time
      buf = kline_command_keep_alive(packet->buf[3], packet->buf[4]);
      break;  

    case 0x48:
      // Component test
      {
        uint16_t value = (packet->buf[5] << 8) | packet->buf[6];
        buf = kline_command_component_test(packet->buf[3], packet->buf[4], value);
      }
      break;

    case 0x50:
      // Read sensors
      buf = kline_command_read_sensor(packet->buf[3]);
      break;

    case 0x51:
      // Read stuff
      buf = kline_command_read_stuff(packet->buf[3]);
      break;

    case 0x52:
      // Read voltage data
      buf = kline_command_read_voltage_data(packet->buf[3]);
      break;

    case 0x56:
      // event log
      buf = kline_command_get_error_codes(packet->buf[3], packet->buf[4]);
      break;

    case 0x57:
      // CO2 calibration (umm, we have a sensor for that?!)
      buf = kline_command_co2_calibration(packet->buf[3], packet->buf[4]);
      break;

    default:
      break;
  } 

  if (buf) {
    len = buf[1] + 2;
    buf[0] = KLINE_TX_ADDR;
    if (!buf[2]) {
      buf[2] = cmd ^ 0x80;
    }
    buf[len - 1] = 0x00;
    buf[len - 1] = calc_kline_checksum(buf, len);

    respPacket = (klinePacket_t *)malloc(sizeof(klinePacket_t));
    respPacket->buf = buf;
    respPacket->len = len;
  }

  return respPacket;
}

uint8_t *kline_command_shutdown(void)
{
  uint8_t *buf = (uint8_t *)malloc(4);
  buf[1] = 0x02;
  buf[2] = 0;
  return buf;
}

uint8_t *kline_command_timed_start(uint8_t mode, uint8_t minutes)
{
  uint8_t *buf = (uint8_t *)malloc(5);
  buf[1] = 0x03;
  buf[2] = 0;
  buf[3] = minutes;

  (void)mode;   // for now
  return buf;
}

uint8_t *kline_command_diagnostics(void)
{
  static const uint8_t canned_reply[11] = {
    0x00, 0x09, 0x00, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x03, 0xDD, 0x00
  };
  uint8_t *buf = (uint8_t *)malloc(11);
  memcpy(buf, canned_reply, 11);
  return buf;
}

uint8_t *kline_command_keep_alive(uint8_t mode, uint8_t minutes)
{
  uint8_t *buf = (uint8_t *)malloc(6);
  uint16_t remaining = 0x0088;  // Matching the example until this is wired in

  // Add x minutes to the timer for mode, and return the number of minutes left.
  buf[1] = 0x04;
  buf[3] = (remaining >> 8) & 0xFF;
  buf[4] = remaining & 0xFF;
  return buf;  
}

uint8_t *kline_command_component_test(uint8_t component, uint8_t seconds, uint16_t value)
{
  uint8_t *buf = (uint8_t *)malloc(7);
  buf[1] = 0x05;
  buf[3] = component;
  buf[4] = seconds;
  buf[5] = (value >> 8) & 0xFF;
  buf[6] = value & 0xFF;
  return buf;
}

uint8_t *kline_command_read_sensor(uint8_t sensornum)
{
  switch(sensornum) {
    case 0x02:
      // Status flags
      return kline_read_status_sensor();
    case 0x03:
      // Subsystem on flags
      return kline_read_subsystem_enabled_sensor();
    case 0x04:
      // Fuel parameters
      return kline_read_fuel_param_sensor();
    case 0x05:
      // Operational measurements
      return kline_read_operational_sensor();
    case 0x06:
      // Operating times
      return kline_read_operating_time_sensor();
    case 0x07:
      // Operating state
      return kline_read_state_sensor();
    case 10:
      // Burning level
      return kline_read_burning_level_sensor();
    case 11:
      // Burning duration
      return kline_read_burning_duration_sensor();
    case 12:
      // Start counters
      return kline_read_start_counter_sensor();
    case 15:
      // subsystem status
      return kline_read_subsystem_status_sensor();
    case 17:
      // Temperature thresholds
      return kline_read_temperature_thresh_sensor();
    case 18:
      // Ventilation duration
      return kline_read_ventilation_duration_sensor();
    case 19:
      // Fuel prewarming status
      return kline_read_fuel_prewarming_sensor();
    case 20:
      // Spark Transmission
      return kline_read_spark_transmission_sensor();
    default:
      return 0;
  }
}

uint8_t *kline_command_read_stuff(uint8_t index)
{
  switch(index) {
    case 0x01:
      // Device ID Number
      return kline_read_device_number();
    case 0x02:
      // Hardware Version
      return kline_read_hardware_version();
    case 0x03:
      // Data set ID Number
      return kline_read_data_set_number();
    case 0x04:      
      // Control Unit Build Date (DD MM YY)
      return kline_read_control_unit_man_date();
    case 0x05:
      // Heater manufacture date (DD MM YY)
      return kline_read_heater_man_date();
    case 0x06:
      // Telestart code
      return 0;
    case 0x07:
      // Customer ID Number (i.e. VW Part Number)
      return kline_read_customer_id();
    case 0x08:  // maybe 0x09?
      // Serial Number
      return kline_read_serial_number();
    case 0x0A:
      // W-Bus version (3.3 -> 0x33)
      return kline_read_wbus_version();
    case 0x0B:
      // Device name
      return kline_read_device_name();
    case 0x0C:
      // WBus Code - Flags of supported subsystems
      return kline_read_subsystems_supported();
    case 0x0D:
      // Software ID (unsupported for now)
      return 0;
    default:
      return 0;
  }
}  

uint8_t *kline_command_read_voltage_data(uint8_t index)
{
  if (index != 0x02) {
    return 0;
  }

  uint8_t *buf = (uint8_t *)malloc(19);
  buf[1] = 17;
  buf[3] = index;
  return buf;
}

uint8_t *kline_command_get_error_codes(uint8_t subcmd, uint8_t index)
{
  switch(subcmd) {
    case 0x01:
      return kline_get_error_code_list();
    case 0x02:
      return kline_get_error_code_details(index);
    case 0x03:
      return kline_clear_error_code_list();
    default:
      return 0;
  }
}

uint8_t *kline_get_error_code_list(void)
{
  uint8_t error_list_len = 2;   // for now
  uint8_t len = 5 + 2 * error_list_len;    
  uint8_t *buf = (uint8_t *)malloc(len);

  buf[1] = len - 2;
  buf[3] = 1;
  buf[4] = error_list_len;
  for (int i = 0; i < (int)error_list_len; i++) {
    buf[5 + 2*i] = 0xFF;  // arbitrary error code for now
    buf[6 + 2*i] = 0x05;  // error count of 5 for now
  }
  return buf;
}

uint8_t *kline_get_error_code_details(uint8_t index)
{
  uint8_t error_list_len = 2;   // for now
  uint8_t *buf = (uint8_t *)malloc(16);

  buf[1] = 14;
  buf[3] = 2;
  buf[4] = index;
  buf[5] = 0x00;  // status - stubbed
  buf[6] = 0x03;  // counter - stubbed
  uint16_t state = 0x3445;  // from command 0x50, index 7, bytes 0-1
  buf[7] = (state >> 8) & 0xFF;
  buf[8] = state & 0xFF;
  buf[9] = 50 + 35;         // temperature +50C offset
  
  uint16_t operating_hours = 500;
  buf[12] = (operating_hours >> 8) & 0xFF;
  buf[13] = operating_hours & 0xFF;
  buf[14] = 45;   // operating minutes
  return buf;
}

uint8_t *kline_clear_error_code_list(void)
{
  uint8_t *buf = (uint8_t *)malloc(4);
  buf[1] = 3;
  buf[3] = 3;
  return buf;
}

uint8_t *kline_command_co2_calibration(uint8_t index, uint8_t value)
{
  switch(index) {
    case 0x01:
      // read CO2 values
      return kline_get_co2();
    case 0x02:
      // write CO2 value - minimum - not listed upstream.
      return kline_set_co2(index, value);
    case 0x03:
      // write CO2 value - maximum
      return kline_set_co2(index, value);
    default:
      return 0;
  }
}

uint8_t *kline_get_co2(void)
{
  return 0;
}

uint8_t *kline_set_co2(uint8_t index, uint8_t value)
{
  return 0;
}



uint8_t *kline_read_status_sensor(void)
{
  return 0;
}

uint8_t *kline_read_subsystem_enabled_sensor(void)
{
  return 0;
}

uint8_t *kline_read_fuel_param_sensor(void)
{
  return 0;
}

uint8_t *kline_read_operational_sensor(void)
{
  return 0;
}

uint8_t *kline_read_operating_time_sensor(void)
{
  return 0;
}

uint8_t *kline_read_state_sensor(void)
{
  return 0;
}

uint8_t *kline_read_burning_level_sensor(void)
{
  return 0;
}

uint8_t *kline_read_burning_duration_sensor(void)
{
  return 0;
}

uint8_t *kline_read_start_counter_sensor(void)
{
  return 0;
}

uint8_t *kline_read_subsystem_status_sensor(void)
{
  return 0;
}

uint8_t *kline_read_temperature_thresh_sensor(void)
{
  return 0;
}

uint8_t *kline_read_ventilation_duration_sensor(void)
{
  return 0;
}

uint8_t *kline_read_fuel_prewarming_sensor(void)
{
  return 0;
}

uint8_t *kline_read_spark_transmission_sensor(void)
{
  return 0;
}


uint8_t *kline_read_device_number(void)
{
  return 0;
}

uint8_t *kline_read_hardware_version(void)
{
  return 0;
}

uint8_t *kline_read_data_set_number(void)
{
  return 0;
}

uint8_t *kline_read_control_unit_man_date(void)
{
  return 0;
}

uint8_t *kline_read_heater_man_date(void)
{
  return 0;
}

uint8_t *kline_read_customer_id(void)
{
  return 0;
}

uint8_t *kline_read_serial_number(void)
{
  return 0;
}

uint8_t *kline_read_wbus_version(void)
{
  return 0;
}

uint8_t *kline_read_device_name(void)
{
  return 0;
}

uint8_t *kline_read_subsystems_supported(void)
{
  return 0;
}
