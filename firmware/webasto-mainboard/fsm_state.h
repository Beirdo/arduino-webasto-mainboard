#ifndef __fsm_state_h_
#define __fsm_state_h_

#include "tinyfsm.hpp"
#include "fsm.h"

class IdleState : public WebastoControlFSM
{
  void entry();
  void react(StartupEvent       const &e);
  void react(IgnitionEvent      const &e);
  void react(StartRunEvent      const &e);

};

#endif
