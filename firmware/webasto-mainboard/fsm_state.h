#ifndef __fsm_state_h_
#define __fsm_state_h_

#include "tinyfsm.hpp"
#include "fsm.h"

class IdleState : public WebastoControlFSM
{
  public:
    void entry();

  protected:
    uint8_t state_num = 0x04;

};

class StandbyState : public WebastoControlFSM
{
  public:
    void entry();
    void react(TimerEvent const &e);

  protected:
    uint8_t state_num = 0x3D;
};

class PrefuelState : public WebastoControlFSM
{
  public:
    void entry();
    void react(TimerEvent const &e);

  protected:
    uint8_t state_num = 0x2D;
};

class InitialRampUpState : public WebastoControlFSM
{
  public:
    void entry();
    void react(TimerEvent const &e);

  protected:
    uint8_t state_num = 0x26;
};

class StabilizationState : public WebastoControlFSM
{
  public:
    void entry();
    void react(TimerEvent const &e);

  protected:
    uint8_t state_num = 0x25;
};

class SecondRampUpState : public WebastoControlFSM
{
  public:
    void entry();
    void react(TimerEvent const &e);

  protected:
    uint8_t state_num = 0x3F;
};

class TestBurnState : public WebastoControlFSM
{
  public:
    void entry();
    void react(TimerEvent       const &e);
    void react(FlameDetectEvent const &e);

  protected:
    uint8_t state_num = 0x40;
};

class FlameMeasureState : public WebastoControlFSM
{
  public:
    void entry();
    void react(TimerEvent const &e);

  protected:
    uint8_t state_num = 0x11;
};

class AutoBurnState : public WebastoControlFSM
{
  public:
    void entry();
    void react(TimerEvent       const &e);
    void react(FlameDetectEvent const &e);

  protected:
    uint8_t state_num = 0x41;
};

class CooldownState : public WebastoControlFSM
{
  public:
    void entry();
    void react(TimerEvent const &e);

  protected:
    uint8_t state_num = 0x1C;
};

class LockdownState : public WebastoControlFSM
{
  public:
    void entry();

  protected:
    uint8_t state_num = 0x28;
};


#endif
