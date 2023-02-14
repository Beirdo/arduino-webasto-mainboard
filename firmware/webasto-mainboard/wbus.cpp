
#include <cppQueue.h>
#include <Arduino.h>
#include <pico.h>
#include <stdlib.h>
#include <string.h>
#include <ArduinoLog.h>
#include <CoreMutex.h>

#include "project.h"
#include "wbus_packet.h"
#include "wbus.h"
#include "webasto.h"
#include "fsm.h"
#include "fram.h"
#include "fuel_pump.h"
#include "analog.h"
#include "device_eeprom.h"
#include "wifi.h"

#define WBUS_RX_MATCH_ADDR 0xF4
#define WBUS_TX_ADDR       0x4F

#define WBUS_FIFO_SIZE   64
#define WBUS_BUFFER_SIZE 64

int tx_active;
int rx_active;

uint8_t wbus_rx_buffer[WBUS_BUFFER_SIZE];
int wbus_rx_tail;

cppQueue wbus_rx_q(sizeof(wbusPacket_t *), WBUS_FIFO_SIZE, FIFO);
cppQueue wbus_tx_q(sizeof(wbusPacket_t *), WBUS_FIFO_SIZE, FIFO);

void init_wbus(void)
{
  Log.notice("Configuring Serial2 for WBus");
  Serial2.setRX(PIN_SERIAL2_RX);
  Serial2.setTX(PIN_SERIAL2_TX);
  Serial2.begin(2400, SERIAL_8E1);
  Serial2.flush();

  pinMode(PIN_KLINE_EN, OUTPUT);
  digitalWrite(PIN_KLINE_EN, LOW);
  tx_active = false;
  rx_active = false;
  wbus_rx_tail = 0;
}

void send_break(void)
{
  Serial2.end();

  Log.notice("Sending a break on Serial2");
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

void wbus_send_response(wbusPacket_t *respPacket)
{
  if (!respPacket) {
    return;
  }

  if (respPacket->fromWiFi) {
    cbor_send((const uint8_t *)respPacket->buf, respPacket->len);
    return;
  }

  wbus_tx_q.push((const wbusPacket_t *)&respPacket);

  if (!tx_active) {
    wbus_send_next_packet();
  }
}

void wbus_send_next_packet(void)
{
  if (tx_active) {
    return;
  }

  if (wbus_tx_q.isEmpty()) {
    return;
  }

  wbusPacket_t *packet;
  wbus_tx_q.pop(&packet);

  if (!packet) {
    return;
  }

  int len = packet->len;

  Log.notice("Transmitting WBus response");
  hexdump(packet->buf, len, 16);

  tx_active = true;
  digitalWrite(PIN_KLINE_EN, HIGH);
  // send_break();  ?? - I think as slave we don't need to.
  Serial2.write(packet->buf, len);
  digitalWrite(PIN_KLINE_EN, LOW);
  tx_active = false;

  free(packet->buf);
  free(packet);
}

uint8_t calc_wbus_checksum(uint8_t *buf, int len)
{
  uint8_t checksum = 0x00;
  for (int i = 0; i < len; i++) {
    checksum ^= buf[i];
  }
  return checksum;
}

uint8_t *allocate_response(uint8_t command, uint8_t len, uint8_t subcommand)
{
  uint8_t *buf = (uint8_t *)malloc(len);
  buf[1] = len - 2;
  buf[2] = command ^ 0x80;
  if (subcommand) {
    buf[3] = subcommand;
  }
  buf[len - 1] = 0x00;

  return buf;
}

void receive_wbus_from_cbor(uint8_t *cbor_buf, int len)
{
  uint8_t *buf = (uint8_t *)malloc(len);
  memcpy(buf, cbor_buf, len);

  wbusPacket_t *packet = (wbusPacket_t *)malloc(sizeof(wbusPacket_t));
  packet->buf = buf;
  packet->len = len;
  packet->fromWiFi = true;

  wbus_rx_q.push(&packet);
}

void receive_wbus_from_serial(void)
{
  if (Serial2.getBreakReceived()) {
    Log.notice("Received break on Serial2");
    rx_active = true;
    wbus_rx_tail = 0;
    Serial2.flush();
  }

  while(Serial2 && Serial2.available()) {
    if (!rx_active) {
      Serial2.flush();
      continue;
    }

    uint8_t ch = Serial2.read();
    if (wbus_rx_tail == 0 && ch != WBUS_RX_MATCH_ADDR) {
      rx_active = false;
      Serial2.flush();
      continue;
    }

    wbus_rx_buffer[wbus_rx_tail++] = ch;
    if (wbus_rx_tail > 2) {
      int len = wbus_rx_buffer[1] + 2;
      if (wbus_rx_tail == len) {
        Log.notice("Received WBus packet");
        // Full frame received
        if (calc_wbus_checksum(wbus_rx_buffer, len)) {
          // Bad checksum.  Turf it.  Wait for new break
          Log.warning("Received packet had bad checksum.  Discarding");
          rx_active = false;
          Serial2.flush();
          continue;
        }

        uint8_t *buf = (uint8_t *)malloc(len);
        memcpy(buf, (const void *)wbus_rx_buffer, len);

        wbusPacket_t *packet = (wbusPacket_t *)malloc(sizeof(wbusPacket_t));
        packet->buf = buf;
        packet->len = len;
        packet->fromWiFi = false;

        wbus_rx_q.push(&packet);
        wbus_rx_tail = 0;
      }
    }
  }
}

void process_wbus(void)
{
  while(!wbus_rx_q.isEmpty()) {
    wbusPacket_t *packet;
    wbus_rx_q.pop(&packet);
    if (!packet) {
      continue;
    }

    Log.notice("Processing WBus packet");
    hexdump(packet->buf, packet->len, 16);

    uint8_t cmd = packet->buf[2];
    wbusPacket_t *respPacket = wbus_rx_dispatch(packet, cmd);
    if (respPacket) {
      wbus_send_response(respPacket);
    }

    free(packet->buf);
    free(packet);
  }

  if (!tx_active) {
    wbus_send_next_packet();
  }
}

wbusPacket_t *wbus_rx_dispatch(wbusPacket_t *packet, uint8_t cmd)
{
  if (!packet) {
    return 0;
  }

  int len;
  wbusPacket_t *respPacket = 0;
  uint8_t *buf = 0;

  switch(cmd) {
    case 0x10:
      // Shutdown, no data
      buf = wbus_command_shutdown();
      break;

    case 0x20:
      // Start for x minutes, default mode
      buf = wbus_command_timed_start(WEBASTO_MODE_DEFAULT, packet->buf[3]);
      break;

    case 0x21:
      // Start for x minutes, parking heater on
      buf = wbus_command_timed_start(WEBASTO_MODE_PARKING_HEATER, packet->buf[3]);
      break;

    case 0x22:
      // Start for x minutes, ventilation on
      buf = wbus_command_timed_start(WEBASTO_MODE_VENTILATION, packet->buf[3]);
      break;

    case 0x23:
      // Start for x minutes, supplemental heating on
      buf = wbus_command_timed_start(WEBASTO_MODE_SUPPLEMENTAL_HEATER, packet->buf[3]);
      break;

    case 0x24:
      // Start for x minutes, circulation pump on
      buf = wbus_command_timed_start(WEBASTO_MODE_CIRCULATION_PUMP, packet->buf[3]);
      break;

    case 0x25:
      // Start for x minutes, boost on
      buf = wbus_command_timed_start(WEBASTO_MODE_BOOST, packet->buf[3]);
      break;

    case 0x26:
      // Start for x minutes, coolng on
      buf = wbus_command_timed_start(WEBASTO_MODE_COOLING, packet->buf[3]);
      break;

    case 0x38:
      // Diagnostic message, unknown use
      buf = wbus_command_diagnostics();
      break;

    case 0x44:
      // Seems to be a timer keepalive, I'm going to assume it adds the same number of minutes originally requested
      // and returns a 16-bit "minutes remaining" time
      buf = wbus_command_keep_alive(packet->buf[3], packet->buf[4]);
      break;

    case 0x48:
      // Component test
      {
        uint16_t value = (packet->buf[5] << 8) | packet->buf[6];
        buf = wbus_command_component_test(packet->buf[3], packet->buf[4], value);
      }
      break;

    case 0x50:
      // Read sensors
      buf = wbus_command_read_sensor(packet->buf[3]);
      break;

    case 0x51:
      // Read stuff
      buf = wbus_command_read_stuff(packet->buf[3]);
      break;

    case 0x53:
      // Read voltage data
      buf = wbus_command_read_voltage_data(packet->buf[3]);
      break;

    case 0x56:
      // event log
      buf = wbus_command_get_error_codes(packet->buf[3], packet->buf[4]);
      break;

    case 0x57:
      // CO2 calibration (umm, we have a sensor for that?!)
      buf = wbus_command_co2_calibration(packet->buf[3], packet->buf[4]);
      break;

    default:
      break;
  }

  if (buf) {
    len = buf[1] + 2;
    buf[0] = WBUS_TX_ADDR;
    if (!buf[2]) {
      buf[2] = cmd ^ 0x80;
    }
    buf[len - 1] = 0x00;
    buf[len - 1] = calc_wbus_checksum(buf, len);

    respPacket = (wbusPacket_t *)malloc(sizeof(wbusPacket_t));
    respPacket->buf = buf;
    respPacket->len = len;
    respPacket->fromWiFi = packet->fromWiFi;
  }

  return respPacket;
}

uint8_t *wbus_command_shutdown(void)
{
  uint8_t *buf = allocate_response(0x10, 4);

  ShutdownEvent event;
  event.mode = WEBASTO_MODE_DEFAULT;
  WebastoControlFSM::dispatch(event);
  return buf;
}

uint8_t *wbus_command_timed_start(uint8_t mode, uint8_t minutes)
{
  uint8_t *buf = allocate_response(0x20, 5);
  buf[3] = minutes;

  StartupEvent event;
  event.mode = (int)mode;
  event.minutes = (int)minutes;
  WebastoControlFSM::dispatch(event);
  return buf;
}

uint8_t *wbus_command_diagnostics(void)
{
  static const uint8_t canned_reply[7] = {
    0x0B, 0x00, 0x00, 0x00, 0x00, 0x03, 0xDD
  };
  uint8_t *buf = allocate_response(0x30, 11);
  memcpy(&buf[3], canned_reply, 7);
  return buf;
}

uint8_t *wbus_command_keep_alive(uint8_t mode, uint8_t minutes)
{
  uint8_t *buf = allocate_response(0x44, 6);

  AddTimeEvent event;
  event.mode = mode;
  event.minutes = minutes;
  WebastoControlFSM::dispatch(event);

  int remaining = globalTimer.get_remaining_time(TIMER_TIMED_SHUT_DOWN) / 60000;

  // Add x minutes to the timer for mode, and return the number of minutes left.
  buf[3] = HI_BYTE(remaining);
  buf[4] = LO_BYTE(remaining);
  return buf;
}

uint8_t *wbus_command_component_test(uint8_t component, uint8_t seconds, uint16_t value)
{
  // TODO:  Implement tests
  uint8_t *buf = allocate_response(0x45, 7, component);
  buf[4] = seconds;
  buf[5] = HI_BYTE(value);
  buf[6] = LO_BYTE(value);
  return buf;
}

uint8_t *wbus_command_read_sensor(uint8_t sensornum)
{
  switch(sensornum) {
    case 0x02:
      // Status flags
      return wbus_read_status_sensor();
    case 0x03:
      // Subsystem on flags
      return wbus_read_subsystem_enabled_sensor();
    case 0x04:
      // Fuel parameters
      return wbus_read_fuel_param_sensor();
    case 0x05:
      // Operational measurements
      return wbus_read_operational_sensor();
    case 0x06:
      // Operating times
      return wbus_read_operating_time_sensor();
    case 0x07:
      // Operating state
      return wbus_read_state_sensor();
    case 0x0A:
      // Burning duration
      return wbus_read_burning_duration_sensor();
    case 0x0B:
      // Operating duration
      return wbus_read_operating_duration_sensor();
    case 0x0C:
      // Start counters
      return wbus_read_start_counter_sensor();
    case 0x0F:
      // subsystem status
      return wbus_read_subsystem_status_sensor();
    case 0x11:
      // Temperature thresholds
      return wbus_read_temperature_thresh_sensor();
    case 0x12:
      // Ventilation duration
      return wbus_read_ventilation_duration_sensor();
    case 0x13:
      // Fuel prewarming status
      return wbus_read_fuel_prewarming_sensor();
    case 0x14:
      // Spark Transmission
      return wbus_read_spark_transmission_sensor();
    default:
      return 0;
  }
}

uint8_t *wbus_command_read_stuff(uint8_t index)
{
  if (index <= 0x00 || index > 0x0D) {
    return 0;
  }

  device_info_t *info = get_device_info(index - 1);
  if (!info) {
    return 0;
  }

  uint8_t *buf = allocate_response(0x51, 5 + info->len, index);
  memcpy(&buf[4], info->buf, info->len);
  return buf;
}

uint8_t *wbus_command_read_voltage_data(uint8_t index)
{
  if (index != 0x02) {
    return 0;
  }

  //   bytes:
  // 0: dont know
  // 1,2: Minimum Voltage threshold
  // 3,4,5,6: dont know
  // 7: Minimum voltage detection delay (delay)
  // 8,9: Maximum voltage threshold
  // 10,11,12,13: dont know
  // 14: Max voltage detection delay (seconds)

  static const uint8_t canned_reply[14] = {
    0x2C, 0x24, 0x25, 0x1C, 0x30, 0xD4, 0xFA, 0x40, 0x74, 0x00, 0x00, 0x63, 0x9C, 0x05
  };
  uint8_t *buf = allocate_response(0x53, 19, index);
  memcpy(&buf[4], canned_reply, 14);
  return buf;
}

uint8_t *wbus_command_get_error_codes(uint8_t subcmd, uint8_t index)
{
  switch(subcmd) {
    case 0x01:
      return wbus_get_error_code_list();
    case 0x02:
      return wbus_get_error_code_details(index);
    case 0x03:
      return wbus_clear_error_code_list();
    default:
      return 0;
  }
}

uint8_t *wbus_get_error_code_list(void)
{
  CoreMutex m(&fram_mutex);

  int error_list_len = fram_data.current.error_list_count;
  int len = 5 + 2 * error_list_len;
  uint8_t *buf = allocate_response(0x56, len, 0x01);

  buf[4] = error_list_len;
  for (int i = 0; i < error_list_len; i++) {
    buf[5 + 2*i] = fram_data.current.error_list[i].code;
    buf[6 + 2*i] = fram_data.current.error_list[i].count;
  }
  return buf;
}

uint8_t *wbus_get_error_code_details(uint8_t code)
{
  CoreMutex m(&fram_mutex);

  int error_list_len = fram_data.current.error_list_count;
  int i;
  for (i = 0; i < error_list_len; i++) {
    if (fram_data.current.error_list[i].code == code) {
      break;
    }
  }

  if (i == error_list_len) {
    return 0;
  }

  int index = i;

  uint8_t *buf = allocate_response(0x56, 16, 0x02);

  buf[4] = code;
  buf[5] = fram_data.current.error_list[index].status;
  buf[6] = fram_data.current.error_list[index].count;

  uint16_t state = fram_data.current.error_list[index].state;
  buf[7] = HI_BYTE(state);
  buf[8] = LO_BYTE(state);
  buf[9] = fram_data.current.error_list[i].temperature;

  uint16_t millivolts = fram_data.current.error_list[index].vbat;
  buf[10] = HI_BYTE(millivolts);
  buf[11] = LO_BYTE(millivolts);

  time_sensor_t *operating_time = &fram_data.current.error_list[i].operating_time;
  buf[12] = HI_BYTE(operating_time->hours);
  buf[13] = LO_BYTE(operating_time->hours);
  buf[14] = operating_time->minutes;
  return buf;
}

uint8_t *wbus_clear_error_code_list(void)
{
  uint8_t *buf = allocate_response(0x56, 5, 0x03);
  fram_clear_error_list();
  return buf;
}

uint8_t *wbus_command_co2_calibration(uint8_t index, uint8_t value)
{
  switch(index) {
    case 0x01:
      // read CO2 values
      return wbus_get_co2();
    case 0x03:
      // write CO2 value
      return wbus_set_co2(value);
    default:
      return 0;
  }
}

uint8_t *wbus_get_co2(void)
{
  CoreMutex m(&fram_mutex);

  uint8_t *buf = allocate_response(0x57, 8, 0x01);

  buf[4] = fram_data.current.current_co2;
  buf[5] = fram_data.current.minimum_co2;
  buf[6] = fram_data.current.maximum_co2;
  return buf;
}

uint8_t *wbus_set_co2(uint8_t value)
{
  uint8_t *buf = allocate_response(0x57, 5, 0x03);
  fram_write_co2(value);
  buf[4] = value;
  return buf;
}

uint8_t *wbus_read_status_sensor(void)
{
  CoreMutex m(&fsm_mutex);

  uint8_t *buf = allocate_response(0x50, 9, 0x02);
  uint8_t flags = 0x00;
  flags |= (fsm_mode == WEBASTO_MODE_SUPPLEMENTAL_HEATER ? 0x10 : 0x00);
  flags |= ((fsm_mode == WEBASTO_MODE_SUPPLEMENTAL_HEATER || fsm_mode == WEBASTO_MODE_PARKING_HEATER) ? 0x01 : 0x00);
  buf[4] = flags;

  flags = 0x00;
  flags |= (externalTempSensor->get_value() >= 1000 ? 0x01 : 0x00);    // >= 10C - summer.  Below - winter
  buf[5] = flags;
  buf[6] = 0x00;      // Generator signal D+  (whaaa?)
  buf[7] = 0x00;      // boost mode, auxiliary drive

  flags = 0x00;
  flags |= (ignitionOn ? 0x01 : 0x00);
  buf[8] = flags;
  return buf;
}

uint8_t *wbus_read_subsystem_enabled_sensor(void)
{
  CoreMutex m(&fsm_mutex);

  uint8_t *buf = allocate_response(0x50, 5, 0x03);
  uint8_t flags = 0x00;
  flags |= (combustionFanPercent ? 0x01 : 0x00);
  flags |= (glowPlugOutEnable ? 0x02 : 0x00);
  flags |= (fuelPumpTimer.getBurnPower() ? 0x04 : 0x00);
  flags |= (circulationPumpOn ? 0x08 : 0x00);
  flags |= (vehicleFanPercent ? 0x10 : 0x00);
  flags |= 0x00;        // Nozzle stock heating... we don't have that??!
  flags |= (glowPlugInEnable ? 0x40 : 0x00);
  buf[4] = flags;
  return buf;
}

uint8_t *wbus_read_fuel_param_sensor(void)
{
  uint8_t *buf = allocate_response(0x50, 7, 0x04);
  buf[4] = 0x1D;      // from libwbus example, change to what it reads with OEM controller
  buf[5] = 0x3C;      // from libwbus example, change to what it reads with OEM controller
  buf[6] = 0x3C;      // from libwbus example, change to what it reads with OEM controller
  return buf;
}

uint8_t *wbus_read_operational_sensor(void)
{
  uint8_t *buf = allocate_response(0x50, 12, 0x05);

  buf[4] = (uint8_t)(((externalTempSensor->get_value() / 50) + 1 / 2) + 50);

  int vbat = batteryVoltageSensor->get_value();
  buf[5] = HI_BYTE(vbat);
  buf[6] = LO_BYTE(vbat);
  buf[7] = (glowPlugInEnable ? 0x01 : 0x00);

  int power = fuelPumpTimer.getBurnPower();
  buf[8] = HI_BYTE(power);
  buf[9] = LO_BYTE(power);

  int milliohms = flameDetectorSensor->get_value();
  buf[10] = HI_BYTE(milliohms);
  buf[11] = LO_BYTE(milliohms);

  return buf;
}

uint8_t *wbus_read_operating_time_sensor(void)
{
  CoreMutex m(&fram_mutex);

  int len = 12;
  uint8_t *buf = (uint8_t *)malloc(len);
  buf[1] = 0x06;
  buf[3] = len - 2;

  buf[4] = HI_BYTE(fram_data.current.total_burn_duration.hours);
  buf[5] = LO_BYTE(fram_data.current.total_burn_duration.hours);
  buf[6] = fram_data.current.total_burn_duration.minutes;
  buf[7] = HI_BYTE(fram_data.current.total_working_duration.hours);
  buf[8] = LO_BYTE(fram_data.current.total_working_duration.hours);
  buf[9] = fram_data.current.total_working_duration.minutes;
  buf[10] = HI_BYTE(fram_data.current.total_start_counter);
  buf[11] = LO_BYTE(fram_data.current.total_start_counter);

  return buf;
}

uint8_t *wbus_read_state_sensor(void)
{
  uint8_t *buf = allocate_response(0x50, 10, 0x07);

  mutex_enter_blocking(&fsm_mutex);
  buf[4] = fsm_state;
  mutex_exit(&fsm_mutex);

  buf[5] = 0x00;                // Operating state state number ???

  mutex_enter_blocking(&fram_mutex);
  buf[6] = fram_data.current.device_status; // Device state bitfield,  0x01 = STFL, 0x02 = UEHFL, 0x04 = SAFL, 0x08 = RZFL
  mutex_exit(&fram_mutex);

  buf[7] = 0x00;                // unknown
  buf[8] = 0x00;                // unknown
  buf[9] = 0x00;                // unknown
  return buf;
}

uint8_t *wbus_read_burning_duration_sensor(void)
{
  uint8_t *buf = allocate_response(0x50, 28, 0x0A);

  CoreMutex m(&fram_mutex);

  int index = 4;
  for (int i = 0; i < 4; i++) {
    buf[index++] = HI_BYTE(fram_data.current.burn_duration_parking_heater[i].hours);
    buf[index++] = LO_BYTE(fram_data.current.burn_duration_parking_heater[i].hours);
    buf[index++] = fram_data.current.burn_duration_parking_heater[i].minutes;
  }

  for (int i = 0; i < 4; i++) {
    buf[index++] = HI_BYTE(fram_data.current.burn_duration_supplemental_heater[i].hours);
    buf[index++] = LO_BYTE(fram_data.current.burn_duration_supplemental_heater[i].hours);
    buf[index++] = fram_data.current.burn_duration_supplemental_heater[i].minutes;
  }

  return buf;
}

uint8_t *wbus_read_operating_duration_sensor(void)
{
  uint8_t *buf = allocate_response(0x50, 10, 0x0B);

  CoreMutex m(&fram_mutex);

  buf[4] = HI_BYTE(fram_data.current.working_duration_parking_heater.hours);
  buf[5] = LO_BYTE(fram_data.current.working_duration_parking_heater.hours);
  buf[6] = fram_data.current.working_duration_parking_heater.minutes;
  buf[7] = HI_BYTE(fram_data.current.working_duration_supplemental_heater.hours);
  buf[8] = LO_BYTE(fram_data.current.working_duration_supplemental_heater.hours);
  buf[9] = fram_data.current.working_duration_supplemental_heater.minutes;

  return buf;
}

uint8_t *wbus_read_start_counter_sensor(void)
{
  uint8_t *buf = allocate_response(0x50, 10, 0x0C);

  CoreMutex m(&fram_mutex);

  buf[4] = HI_BYTE(fram_data.current.start_counter_parking_heater);
  buf[5] = LO_BYTE(fram_data.current.start_counter_parking_heater);
  buf[6] = HI_BYTE(fram_data.current.start_counter_supplemental_heater);
  buf[7] = LO_BYTE(fram_data.current.start_counter_supplemental_heater);
  buf[8] = HI_BYTE(fram_data.current.counter_emergency_shutdown);
  buf[9] = LO_BYTE(fram_data.current.counter_emergency_shutdown);
  return buf;
}

uint8_t *wbus_read_subsystem_status_sensor(void)
{
  CoreMutex m(&fsm_mutex);

  uint8_t *buf = allocate_response(0x50, 9, 0x0F);
  buf[4] = (uint8_t)glowPlugPercent;
  buf[5] = fuelPumpTimer.getFuelPumpFrequencyKline();
  buf[6] = (uint8_t)combustionFanPercent;
  buf[7] = 0x00;    // Unknown
  buf[8] = (uint8_t)(circulationPumpOn ? 100 : 0);
  return buf;
}

uint8_t *wbus_read_temperature_thresh_sensor(void)
{
  // lower and upper temperature thresholds (degC + 50)
  // TODO: read from current firmware in unit
  uint8_t *buf = allocate_response(0x50, 6, 0x11);
  buf[4] = 40;    // -10C
  buf[5] = 70;    // 20C
  return buf;
}

uint8_t *wbus_read_ventilation_duration_sensor(void)
{
  CoreMutex m(&fsm_mutex);

  uint8_t *buf = allocate_response(0x50, 7, 0x12);
  buf[4] = HI_BYTE(ventilation_duration.hours);
  buf[5] = LO_BYTE(ventilation_duration.hours);
  buf[6] = ventilation_duration.minutes;
  return buf;
}

uint8_t *wbus_read_fuel_prewarming_sensor(void)
{
  // This is only wired in on ThermoTop V.  I have a ThermoTop C.
  return 0;
}

uint8_t *wbus_read_spark_transmission_sensor(void)
{
  // This is only on gasoline models.  I have a diesel model.
  return 0;
}
