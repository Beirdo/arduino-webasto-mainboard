#ifndef __wbus_h_
#define __wbus_h_

#include <pico.h>
#include "wbus_packet.h"

void init_wbus(void);
void send_break(void);
void wbus_send_response(wbusPacket_t *respPacket);
void wbus_send_next_packet(void);
void receive_wbus_from_serial(void);
void receive_wbus_from_cbor(uint8_t *cbor_buf, int cbor_len);
void process_wbus(void);
wbusPacket_t *wbus_rx_dispatch(wbusPacket_t *packet, uint8_t cmd);
uint8_t *allocate_response(uint8_t command, uint8_t len, uint8_t subcommand = 0);

uint8_t *wbus_command_shutdown(void);
uint8_t *wbus_command_timed_start(uint8_t mode, uint8_t minutes);
uint8_t *wbus_command_diagnostics(void);
uint8_t *wbus_command_keep_alive(uint8_t mode, uint8_t minutes);
uint8_t *wbus_command_component_test(uint8_t component, uint8_t seconds, uint16_t value);
uint8_t *wbus_command_read_sensor(uint8_t sensornum);
uint8_t *wbus_command_read_stuff(uint8_t index);
uint8_t *wbus_command_read_voltage_data(uint8_t index);
uint8_t *wbus_command_get_error_codes(uint8_t subcmd, uint8_t index);
uint8_t *wbus_command_co2_calibration(uint8_t index, uint8_t value);

uint8_t *wbus_get_error_code_list(void);
uint8_t *wbus_get_error_code_details(uint8_t index);
uint8_t *wbus_clear_error_code_list(void);

uint8_t *wbus_get_co2(void);
uint8_t *wbus_set_co2(uint8_t value);

uint8_t *wbus_read_status_sensor(void);
uint8_t *wbus_read_subsystem_enabled_sensor(void);
uint8_t *wbus_read_fuel_param_sensor(void);
uint8_t *wbus_read_operational_sensor(void);
uint8_t *wbus_read_operating_time_sensor(void);
uint8_t *wbus_read_state_sensor(void);
uint8_t *wbus_read_burning_duration_sensor(void);
uint8_t *wbus_read_operating_duration_sensor(void);
uint8_t *wbus_read_start_counter_sensor(void);
uint8_t *wbus_read_subsystem_status_sensor(void);
uint8_t *wbus_read_temperature_thresh_sensor(void);
uint8_t *wbus_read_ventilation_duration_sensor(void);
uint8_t *wbus_read_fuel_prewarming_sensor(void);
uint8_t *wbus_read_spark_transmission_sensor(void);

#endif
