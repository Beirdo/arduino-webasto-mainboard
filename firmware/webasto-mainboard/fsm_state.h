#ifndef __fsm_state_h_
#define __fsm_state_h_

#include "tinyfsm.hpp"
#include "fsm.h"

class IdleState : public WebastoControlFSM
{
  public:
    void entry();
    void react(IgnitionEvent      const &e);
    void react(StartRunEvent      const &e);

  protected:
    uint8_t state_num = 0x04;

};

#endif
