#ifndef __global_timer_h_
#define __global_timer_h_

#include <Arduino.h>
#include <pico.h>
#include <CoreMutex.h>

typedef void (*timer_callback)(int delay_ms);

struct timerItem_s;
typedef struct timerItem_s {
  int start_ms;
  int target_ms;
  timer_callback cb;
  struct timerItem_s *next;
} timerItem_t;

class GlobalTimer {
  public:
    GlobalTimer(void);
    void register_timer(int delay_ms, timer_callback cb);
    void tick(void);
  
  private:
    int _last_run;
    timerItem_t *_head;
    mutex_t _mutex;
};

extern GlobalTimer globalTimer;

#endif
