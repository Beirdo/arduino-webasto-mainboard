#include <Arduino.h>
#include <ArduinoLog.h>
#include <pico.h>
#include <stdlib.h>
#include <string.h>
#include <CoreMutex.h>
#include <cppQueue.h>
#include <canbus_ids.h>
#include <webasto.h>

#include "project.h"
#include "wbus_packet.h"
#include "wbus.h"
#include "fsm.h"
#include "fram.h"
#include "fuel_pump.h"
#include "device_eeprom.h"
#include "canbus.h"
#include "sensor_registry.h"

#define WBUS_RX_MATCH_ADDR 0xF4
#define WBUS_TX_ADDR       0x4F

#define WBUS_BUFFER_SIZE 64

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

void receive_wbus_from_canbus(uint8_t *buf, int len)
{
  Log.notice("Processing WBus packet");
  hexdump(buf, len, 16);

  wbusPacket_t *respPacket = wbus_rx_dispatch(buf, len);
  if (respPacket) {
    canbus_send(CANBUS_ID_WBUS, respPacket->buf, respPacket->len);

    free(respPacket->buf);
    free(respPacket);
  }
}

wbusPacket_t *wbus_rx_dispatch(uint8_t *buf, int len)
{
  if (!buf || !len) {
    return 0;
  }

  uint8_t cmd = buf[2];
  wbusPacket_t *respPacket = 0;
  uint8_t *outbuf = 0;

  switch(cmd) {
    case 0x10:
      // Shutdown, no data
      outbuf = wbus_command_shutdown();
      break;

    case 0x20:
      // Start for x minutes, default mode
      outbuf = wbus_command_timed_start(WEBASTO_MODE_DEFAULT, buf[3]);
      break;

    case 0x21:
      // Start for x minutes, parking heater on
      outbuf = wbus_command_timed_start(WEBASTO_MODE_PARKING_HEATER, buf[3]);
      break;

    case 0x22:
      // Start for x minutes, ventilation on
      outbuf = wbus_command_timed_start(WEBASTO_MODE_VENTILATION, buf[3]);
      break;

    case 0x23:
      // Start for x minutes, supplemental heating on
      outbuf = wbus_command_timed_start(WEBASTO_MODE_SUPPLEMENTAL_HEATER, buf[3]);
      break;

    case 0x24:
      // Start for x minutes, circulation pump on
      outbuf = wbus_command_timed_start(WEBASTO_MODE_CIRCULATION_PUMP, buf[3]);
      break;

    case 0x25:
      // Start for x minutes, boost on
      outbuf = wbus_command_timed_start(WEBASTO_MODE_BOOST, buf[3]);
      break;

    case 0x26:
      // Start for x minutes, coolng on
      outbuf = wbus_command_timed_start(WEBASTO_MODE_COOLING, buf[3]);
      break;

    case 0x44:
      // Seems to be a timer keepalive, I'm going to assume it adds the same number of minutes originally requested
      // and returns a 16-bit "minutes remaining" time
      outbuf = wbus_command_keep_alive(buf[3], buf[4]);
      break;

    case 0x48:
      // Component test
      {
        uint16_t value = (buf[5] << 8) | buf[6];
        outbuf = wbus_command_component_test(buf[3], buf[4], value);
      }
      break;

    case 0x50:
      // Read sensors
      outbuf = wbus_command_read_sensor(buf[3]);
      break;

    case 0x51:
      // Read stuff
      outbuf = wbus_command_read_stuff(buf[3]);
      break;

    case 0x56:
      // event log
      outbuf = wbus_command_get_error_codes(buf[3], buf[4]);
      break;

    case 0x57:
      // CO2 calibration (umm, we have a sensor for that?!)
      outbuf = wbus_command_co2_calibration(buf[3], buf[4]);
      break;

    default:
      break;
  }

  if (outbuf) {
    len = outbuf[1] + 2;
    outbuf[0] = WBUS_TX_ADDR;
    if (!outbuf[2]) {
      outbuf[2] = cmd ^ 0x80;
    }
    outbuf[len - 1] = 0x00;
    outbuf[len - 1] = calc_wbus_checksum(buf, len);

    respPacket = (wbusPacket_t *)malloc(sizeof(wbusPacket_t));
    respPacket->buf = buf;
    respPacket->len = len;
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
    case 0x12:
      // Ventilation duration
      return wbus_read_ventilation_duration_sensor();
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

uint8_t *wbus_read_ventilation_duration_sensor(void)
{
  CoreMutex m(&fsm_mutex);

  uint8_t *buf = allocate_response(0x50, 7, 0x12);
  buf[4] = HI_BYTE(ventilation_duration.hours);
  buf[5] = LO_BYTE(ventilation_duration.hours);
  buf[6] = ventilation_duration.minutes;
  return buf;
}
