#include <Arduino.h>
#include <pico.h>
#include <ArduinoLog.h>
#include <stdlib.h>

#include <cppQueue.h>

#include "project.h"
#include "beeper.h"
#include "global_timer.h"

typedef struct {
  int count;
  int toneMs;
  int silenceMs;
} beeperItem_t;

cppQueue beep_q(sizeof(beeperItem_t *), 16, FIFO);

Beeper beeper(PIN_ALERT_BUZZER);

void Beeper::register_beeper(int count, int toneMs, int silenceMs)
{
  if (!count) {
    return;
  }

  beeperItem_t *item = (beeperItem_t *)malloc(sizeof(beeperItem_t));
  item->count = count;
  item->toneMs = toneMs;
  item->silenceMs = silenceMs;
  beep_q.push(&item);

  if (!_active) {
    timer_callback(TIMER_BEEPER, 0);
  }
}

void Beeper::timer_callback(int timerId, int delay)
{
  (void)timerId;
  (void)delay;

  Log.notice("Received beep timer, active: %d, count: %d, on: %d", _active, _count, _on);

  if (!_active) {
    beeperItem_t *item = 0;
    beep_q.pop(&item);

    if (!item) {
      return;
    }

    _count = item->count;
    _toneMs = item->toneMs;
    _silenceMs = item->silenceMs;
    free(item);

    _on = false;
    _active = true;
  }

  int delayMs;

  if (!_on) {
    if (!_count) {
      delayMs = 2500;
      _active = false;
    } else {
      _count--;
      analogWrite(_pin, 0x7F);
      _on = true;
      delayMs = _toneMs;
    }
  } else {
    analogWrite(_pin, 0x00);
    _on = false;
    delayMs = _silenceMs;
  }

  globalTimer.register_timer(TIMER_BEEPER, delayMs, &beeperTimerCallback);
}

void beeperTimerCallback(int timerId, int delay)
{
  if (timerId == TIMER_BEEPER) {
    beeper.timer_callback(timerId, delay);
  }
}

