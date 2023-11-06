#ifndef A3_SERVER_H
#define A3_SERVER_H

#include <dc_fsm/fsm.h>

enum server_fsm_state{
    PARSEARGS = DC_FSM_USER_START,
    MAKEDIR,
    SOCKET,
    BIND,
    LISTEN,
    ACCEPT,
    POLL,
    HANDLEDATAIN,
    SIGFIN,
    FATALERROR,
    CLEANUP
};

#endif
