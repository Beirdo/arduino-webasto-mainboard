#ifndef __fsm_state_h_
#define __fsm_state_h_

#include "tinyfsm.hpp"
#include "fsm.h"

class IdleState : public WebastoControlFSM
{
  public:
    void entry();
  protected:
    const static uint8_t _state_num = 0x04;

};

class PurgingState : public WebastoControlFSM
{
  public:
    void entry();
    void react(TimerEvent const &e);

  protected:
    const static uint8_t _state_num = 0x23;
};

class StandbyState : public WebastoControlFSM
{
  public:
    void entry();
    void react(TimerEvent const &e);

  protected:
    const static uint8_t _state_num = 0x3D;
};

class PrefuelState : public WebastoControlFSM
{
  public:
    void entry();
    void react(TimerEvent const &e);

  protected:
    const static uint8_t _state_num = 0x2D;
};

class FuelOffState : public WebastoControlFSM
{
  public:
    void entry();
    void react(TimerEvent const &e);

  protected:
    const static uint8_t _state_num = 0x3e;
};

class StabilizationState : public WebastoControlFSM
{
  public:
    void entry();
    void react(TimerEvent const &e);

  protected:
    const static uint8_t _state_num = 0x25;
};

class TestBurnState : public WebastoControlFSM
{
  public:
    void entry();
    void react(TimerEvent       const &e);

  protected:
    const static uint8_t _state_num = 0x40;
};

class FlameMeasureState : public WebastoControlFSM
{
  public:
    void entry();
    void react(TimerEvent const &e);
    void react(FlameDetectEvent const &e);

  protected:
    const static uint8_t _state_num = 0x11;
};

class AutoBurnState : public WebastoControlFSM
{
  public:
    void entry();
    void react(TimerEvent       const &e);

  protected:
    const static uint8_t _state_num = 0x41;
};

class CooldownState : public WebastoControlFSM
{
  public:
    void entry();
    void react(TimerEvent const &e);

  protected:
    const static uint8_t _state_num = 0x1C;
};

class LockdownState : public WebastoControlFSM
{
  public:
    void entry();
    void react(TimerEvent const &e);

  protected:
    const static uint8_t _state_num = 0x28;
};

class EmergencyOffState : public WebastoControlFSM
{
  public:
    void entry();

  protected:
    const static uint8_t _state_num = 0x02;
};


#endif
