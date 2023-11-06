#ifndef A3_SERVER_H
#define A3_SERVER_H

#include <ctype.h>
#include <dc_fsm/fsm.h>
#include <dc_error/error.h>
#include <dc_env/env.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <wordexp.h>

#define SERVER_ARGS 4
#define DIR_INDEX 3
#define PORT_INDEX 2
#define IP_INDEX 1
#define MAX_DIR_NAME_LEN 256

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

struct server_opts {
    int ip_family;
    int argc;
    char **argv;
    char *msg;
    char *host_ip;
    in_port_t host_port;
    char dir_path[MAX_DIR_NAME_LEN];
};

#endif
