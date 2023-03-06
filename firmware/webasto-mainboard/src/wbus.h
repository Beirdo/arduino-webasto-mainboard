#ifndef __wbus_h_
#define __wbus_h_

#include <Arduino.h>
#include "wbus_packet.h"

void receive_wbus_from_canbus(uint8_t *buf, int len);
wbusPacket_t *wbus_rx_dispatch(uint8_t *buf, int len);
uint8_t *allocate_response(uint8_t command, uint8_t len, uint8_t subcommand = 0);

uint8_t *wbus_command_shutdown(void);
uint8_t *wbus_command_timed_start(uint8_t mode, uint8_t minutes);
uint8_t *wbus_command_keep_alive(uint8_t mode, uint8_t minutes);
uint8_t *wbus_command_component_test(uint8_t component, uint8_t seconds, uint16_t value);
uint8_t *wbus_command_read_sensor(uint8_t sensornum);
uint8_t *wbus_command_read_stuff(uint8_t index);
uint8_t *wbus_command_get_error_codes(uint8_t subcmd, uint8_t index);
uint8_t *wbus_command_co2_calibration(uint8_t index, uint8_t value);

uint8_t *wbus_get_error_code_list(void);
uint8_t *wbus_get_error_code_details(uint8_t index);
uint8_t *wbus_clear_error_code_list(void);

uint8_t *wbus_get_co2(void);
uint8_t *wbus_set_co2(uint8_t value);

uint8_t *wbus_read_operating_time_sensor(void);
uint8_t *wbus_read_state_sensor(void);
uint8_t *wbus_read_burning_duration_sensor(void);
uint8_t *wbus_read_operating_duration_sensor(void);
uint8_t *wbus_read_start_counter_sensor(void);
uint8_t *wbus_read_ventilation_duration_sensor(void);

#endif
