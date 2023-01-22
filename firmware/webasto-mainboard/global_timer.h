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
    timerItem_t *register_timer(int delay_ms, timer_callback cb);
    void adjust_timer(timerItem_t *item, int delay_ms);
    void tick(void);
  
  private:
    timerItem_t *remove_item(timerItem_t *item, bool locked = true);
    void insert_item(timerItem_t *item, bool locked = true);
  
    int _last_run;
    timerItem_t *_head;
    mutex_t _mutex;
};

extern GlobalTimer globalTimer;

#endif
