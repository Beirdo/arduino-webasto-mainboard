#ifndef __beeper_h_
#define __beeper_h_

#include <Arduino.h>
#include <pico.h>

class Beeper {
  public:
    Beeper(int pin) : _pin(pin), _active(false), _on(false), _count(0) {
      digitalWrite(_pin, 0);
      pinMode(_pin, OUTPUT);
      digitalWrite(_pin, 0);
    };
    void register_beeper(int count, int toneMs, int silenceMs);
    void timer_callback(int timerId, int delay);

  private:
    int _pin;
    int _active;
    bool _on;
    int _count;
    int _toneMs;
    int _silenceMs;

};

extern Beeper beeper;

void beeperTimerCallback(int timerId, int delay);

#endif
