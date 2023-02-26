#ifndef __sensor_registry_h_
#define __sensor_registry_h_

#include <Arduino.h>
#include <sensor.h>

struct sensor_reg_item_s;
typedef struct sensor_reg_item_s {
  int id;
  Sensor *sensor;
  struct sensor_reg_item_s *next;
  struct sensor_reg_item_s *prev;
} sensor_reg_item_t;

class SensorRegistry
{
  public:
    SensorRegistry() : _head(0) { mutex_init(&_mutex); };
    void add(int id, Sensor *sensor);
    Sensor *get(int id);
    void remove(int id);

  protected:
    mutex_t _mutex;
    sensor_reg_item_t *_head;
};

extern SensorRegistry sensorRegistry;

#endif