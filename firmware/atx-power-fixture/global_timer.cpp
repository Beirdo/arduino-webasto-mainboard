#include <Arduino.h>
#include <pico.h>
#include <ArduinoLog.h>
#include <CoreMutex.h>

#include "global_timer.h"

GlobalTimer globalTimer;

GlobalTimer::GlobalTimer(void)
{
  _head = 0;
  _last_run = 0;
  mutex_init(&_mutex);
}

void GlobalTimer::register_timer(int timer_id, int delay_ms, timer_callback cb)
{
  if (delay_ms <= 0 || !cb) {
    return;
  }

  Log.notice("Registering timer: %d -> %dms", timer_id, delay_ms);
  
  timerItem_t *item = (timerItem_t *)malloc(sizeof(timerItem_t));

  item->timer_id = timer_id;
  item->start_ms = millis();
  item->target_ms = item->start_ms + delay_ms;
  item->cb = cb;
  item->next = 0;

  insert_item(item, false);
}

timerItem_t *GlobalTimer::remove_item(int timer_id, bool locked)
{
  if (!locked) {
    mutex_enter_blocking(&_mutex);
  }

  timerItem_t *curr, *prev;
  for (curr = _head, prev = 0; curr; prev = curr, curr = curr->next) {
    if (curr->timer_id == timer_id) {
      if (!prev) {
        _head = curr->next;
      } else {
        prev->next = curr->next;
      }
      break;
    }
  }

  if (!locked) {
   mutex_exit(&_mutex);
  }  

  return curr;
}

timerItem_t *GlobalTimer::find_item(int timer_id, bool locked)
{
  if (!locked) {
    mutex_enter_blocking(&_mutex);
  }

  timerItem_t *curr, *prev;
  for (curr = _head, prev = 0; curr; prev = curr, curr = curr->next) {
    if (curr->timer_id == timer_id) {
      break;
    }
  }

  if (!locked) {
   mutex_exit(&_mutex);
  }  

  return curr;
}


void GlobalTimer::insert_item(timerItem_t *item, bool locked)
{
  int key = item->target_ms;

  if (!locked) {
    mutex_enter_blocking(&_mutex);
  }

  Log.notice("Inserting item, key: %d", key);
  item->next = 0;  
  if (!_head) {
    _head = item;
  } else {
    timerItem_t *curr, *prev;
    for (curr = _head, prev = 0; curr; prev = curr, curr = curr->next) {
      if (curr->target_ms > key) {
        item->next = curr;
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

  if (!locked) {
    mutex_exit(&_mutex);
  }
}

void GlobalTimer::adjust_timer(int timer_id, int delay_ms)
{
  CoreMutex m(&_mutex);

  timerItem_t *item = remove_item(timer_id);

  if (item && delay_ms > 0) {
    item->target_ms += delay_ms;
    insert_item(item);
  }
}

int GlobalTimer::get_remaining_time(int timer_id)
{
  CoreMutex m(&_mutex);

  timerItem_t *item = find_item(timer_id);
  int remaining = 0;
  
  if (item) {
    remaining = max(0, item->target_ms - millis());
  }

  return remaining;
}


void GlobalTimer::tick(void)
{
  int now = millis();
  if (now == _last_run) {
    return;
  }

  _last_run = now;

  mutex_enter_blocking(&_mutex);
  timerItem_t *curr, *old_head = _head, *new_head;
  for (curr = _head; curr; curr = curr->next) {
    if (curr->target_ms > now) {
      break;
    }            
    _head = curr->next;
  }
  new_head = _head;
  mutex_exit(&_mutex);  

  timerItem_t *next;
  for (curr = old_head; curr && curr != new_head; curr = next) {
    int elapsed = now - curr->start_ms;
    Log.notice("Calling callback: %d, %dms", curr->timer_id, elapsed);
    (*curr->cb)(curr->timer_id, elapsed);

    next = curr->next;
    free(curr);
  }
}
