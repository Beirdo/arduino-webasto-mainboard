#ifndef __global_timer_h_
#define __global_timer_h_

#include <Arduino.h>
#include <pico.h>
#include <CoreMutex.h>

typedef void (*timer_callback)(int timer_id, int delay_ms);

struct timerItem_s;
typedef struct timerItem_s {
  int timer_id;
  int start_ms;
  int target_ms;
  timer_callback cb;
  struct timerItem_s *next;
} timerItem_t;

enum {
  TIMER_FUEL_PUMP,
  TIMER_TIMED_SHUT_DOWN,
  TIMER_STAGE_COMPLETE,
  TIMER_STAGE_RETRY,
  TIMER_RUN_TIME_MINUTE,
  TIMER_FUEL_FAN_DELTA,
  TIMER_BEEPER,
  TIMER_RESTART_BEEPS,
  TIMER_FSM_STARTUP,
};

class GlobalTimer {
  public:
    GlobalTimer(void);
    void register_timer(int timer_id, int delay_ms, timer_callback cb);
    void adjust_timer(int timer_id, int delay_ms);
    void tick(void);
    int get_remaining_time(int timer_id);
  
  private:
    timerItem_t *find_item(int timer_id, bool locked = true);
    timerItem_t *remove_item(int timer_id, bool locked = true);
    void insert_item(timerItem_t *item, bool locked = true);
  
    int _last_run;
    timerItem_t *_head;
    mutex_t _mutex;
};

extern GlobalTimer globalTimer;

#endif
