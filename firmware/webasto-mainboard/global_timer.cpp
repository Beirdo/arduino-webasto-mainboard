#include <Arduino.h>
#include <pico.h>
#include <CoreMutex.h>

#include "global_timer.h"

GlobalTimer globalTimer;

GlobalTimer::GlobalTimer(void)
{
  _head = 0;
  _last_run = 0;
  mutex_init(&_mutex);
}

void GlobalTimer::register_timer(int delay_ms, timer_callback cb)
{
  if (delay_ms <= 0 || !cb) {
    return;
  }
  
  timerItem_t *item = (timerItem_t *)malloc(sizeof(timerItem_t));

  item->start_ms = millis();
  item->target_ms = item->start_ms + delay_ms;
  item->cb = cb;
  item->next = 0;

  int key = item->target_ms;

  mutex_enter_blocking(&_mutex);
  if (!_head) {
    _head = item;
  } else {
    timerItem_t *curr, *prev;
    for (curr = _head, prev = 0; curr; prev = curr, curr = curr->next) {
      if (curr->target_ms > key) {
        item->next = curr->next; 
        if (!prev) {
          _head = item;
        } else {
          prev->next = item;
        }
        break;
      }
    }

    if (!curr) {
      prev->next = item;
    }
  }    
  mutex_exit(&_mutex);
}

void GlobalTimer::tick(void)
{
  int now = millis();
  if (now == _last_run) {
    return;
  }

  _last_run = now;

  mutex_enter_blocking(&_mutex);
  timerItem_t *curr, *old_head = _head;
  for (curr = _head; curr; curr = curr->next) {
    if (curr->target_ms <= now) {
      _head = curr->next;
      (*curr->cb)(now - curr->start_ms);
    }
  }

  timerItem_t *next;
  for (curr = old_head; curr; curr = next) {
      next = curr->next;
      free(curr);
  }
  mutex_exit(&_mutex);  
}
