#include <Arduino.h>
#include <sensor.h>
#include <CoreMutex.h>

#include "sensor_registry.h"

SensorRegistry sensorRegistry;

void SensorRegistry::add(int id, void *sensor)
{
  sensor_reg_item_t *item = new sensor_reg_item_t;
  item->id = id;
  item->sensor = sensor;

  CoreMutex m(&_mutex);

  sensor_reg_item_t *curr, *prev = 0;
  for (curr = _head; curr && curr->id < id; prev = curr, curr = curr->next);

  item->next = curr;
  item->prev = prev;

  if (curr) {
    curr->prev = item;
  }

  if (prev) {
    prev->next = item;
  } else {
    _head = item;
  }
}

void *SensorRegistry::get(int id) 
{
  CoreMutex m(&_mutex);

  sensor_reg_item_t *curr;

  for (curr = _head; curr && curr->id < id; curr = curr->next);

  if (!curr || curr->id != id) {
    return 0;
  }

  return curr->sensor;
}

void SensorRegistry::remove(int id)
{
  CoreMutex m(&_mutex);

  sensor_reg_item_t *curr, *prev = 0;

  for (curr = _head; curr && curr->id < id; prev = curr, curr = curr->next);

  if (!curr && curr->id != id) {
    return;
  }

  // OK, found it.
  if (prev) {
    prev->next = curr->next;
  } else {
    _head = curr->next;
  }

  if (curr->next) {
    curr->next->prev = prev;
  }

  delete curr;
}
