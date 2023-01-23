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

void GlobalTimer::register_timer(int timer_id, int delay_ms, timer_callback cb)
{
  if (delay_ms <= 0 || !cb) {
    return;
  }
  
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

  if (!locked) {
   mutex_exit(&_mutex);
  }
}

void GlobalTimer::adjust_timer(int timer_id, int delay_ms)
{
  mutex_enter_blocking(&_mutex);

  timerItem_t *item = remove_item(timer_id);

  if (item && delay_ms > 0) {
    item->target_ms += delay_ms;
    insert_item(item);
  }

  mutex_exit(&_mutex);
}

int GlobalTimer::get_remaining_time(int timer_id)
{
  mutex_enter_blocking(&_mutex);

  timerItem_t *item = find_item(timer_id);
  int remaining = 0;
  
  if (item) {
    remaining = max(0, item->target_ms - millis());
  }

  mutex_exit(&_mutex);
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
  timerItem_t *curr, *old_head = _head;
  for (curr = _head; curr; curr = curr->next) {
    if (curr->target_ms <= now) {
      _head = curr->next;
      (*curr->cb)(curr->timer_id, now - curr->start_ms);
    }
  }

  timerItem_t *next;
  for (curr = old_head; curr && curr != _head; curr = next) {
      next = curr->next;
      free(curr);
  }
  mutex_exit(&_mutex);  
}
