#ifndef __kline_h_
#define __kline_h_

#include <pico.h>
#include "kline_packet.h"

void init_kline(void);
void send_break(void);
void kline_send_response(klinePacket_t *respPacket);
void kline_send_next_packet(void);
void process_kline(void);
klinePacket_t *kline_rx_dispatch(klinePacket_t *packet, uint8_t cmd);

uint8_t *kline_command_shutdown(void);
uint8_t *kline_command_timed_start(uint8_t mode, uint8_t minutes);
uint8_t *kline_command_diagnostics(void);
uint8_t *kline_command_keep_alive(uint8_t mode, uint8_t minutes);
uint8_t *kline_command_component_test(uint8_t component, uint8_t seconds, uint16_t value);
uint8_t *kline_command_read_sensor(uint8_t sensornum);
uint8_t *kline_command_read_stuff(uint8_t index);
uint8_t *kline_command_read_voltage_data(uint8_t index);
uint8_t *kline_command_get_error_codes(uint8_t subcmd, uint8_t index);
uint8_t *kline_command_co2_calibration(uint8_t index, uint8_t value);

uint8_t *kline_get_error_code_list(void);
uint8_t *kline_get_error_code_details(uint8_t index);
uint8_t *kline_clear_error_code_list(void);

uint8_t *kline_get_co2(void);
uint8_t *kline_set_co2(uint8_t index, uint8_t value);


uint8_t *kline_read_status_sensor(void);
uint8_t *kline_read_subsystem_enabled_sensor(void);
uint8_t *kline_read_fuel_param_sensor(void);
uint8_t *kline_read_operational_sensor(void);
uint8_t *kline_read_operating_time_sensor(void);
uint8_t *kline_read_state_sensor(void);
uint8_t *kline_read_burning_level_sensor(void);
uint8_t *kline_read_burning_duration_sensor(void);
uint8_t *kline_read_start_counter_sensor(void);
uint8_t *kline_read_subsystem_status_sensor(void);
uint8_t *kline_read_temperature_thresh_sensor(void);
uint8_t *kline_read_ventilation_duration_sensor(void);
uint8_t *kline_read_fuel_prewarming_sensor(void);
uint8_t *kline_read_spark_transmission_sensor(void);

uint8_t *kline_read_device_number(void);
uint8_t *kline_read_hardware_version(void);
uint8_t *kline_read_data_set_number(void);
uint8_t *kline_read_control_unit_man_date(void);
uint8_t *kline_read_heater_man_date(void);
uint8_t *kline_read_customer_id(void);
uint8_t *kline_read_serial_number(void);
uint8_t *kline_read_wbus_version(void);
uint8_t *kline_read_device_name(void);
uint8_t *kline_read_subsystems_supported(void);


#endif
