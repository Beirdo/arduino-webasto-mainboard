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


#endif
