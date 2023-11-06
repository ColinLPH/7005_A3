#ifndef A3_CLIENT_H
#define A3_CLIENT_H

#include <arpa/inet.h>
#include <dc_c/dc_stdlib.h>
#include <dc_error/error.h>
#include <dc_env/env.h>
#include <dc_fsm/fsm.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <wordexp.h>

#define SLEEP_TIMER 3
#define DEFAULT_ARGS_NUM 4
#define FILE_INDEX 3
#define PORT_INDEX 2
#define IP_INDEX 1

enum client_fsm_state{
    PARSEARGS = DC_FSM_USER_START,
    GETFILEINFOS,
    SOCKET,
    CONNECT,
    SENDALL,
    FATALERROR,
    CLEANUP
};

struct client_opts {
    int argc;
    char **argv;
    int socket_fd;
    int ip_family;
    in_port_t dest_port;
    char *dest_ip;
    char *msg;
    struct file_list *head;
};

struct file_info {
    int file_fd;
    uint8_t file_name_size;
    uint8_t *file_name;
    off_t file_size;
};

struct file_list {
    char *file_path;
    struct file_info *file;
    struct file_list *next;
};

int parse_args(const struct dc_env *env, struct dc_error *err, void *arg);
int check_ip(const char *ip_addr);
int get_ip_family(const char *ip_addr);
int parse_in_port_t(struct client_opts *opts);

int get_files(const struct dc_env *env, struct dc_error *err, void *arg);
int check_files(struct file_list *head);
void get_file_infos(struct file_list *head);
uint8_t *sanitize_file_name(char *file_path);

int do_socket(const struct dc_env *env, struct dc_error *err, void *arg);

int do_connect(const struct dc_env *env, struct dc_error *err, void *arg);

int send_all(const struct dc_env *env, struct dc_error *err, void *arg);
void send_file_content(int src_fd, int dest_fd, size_t count);

int clean_up(const struct dc_env *env, struct dc_error *err, void *arg);

int print_error(const struct dc_env *env, struct dc_error *err, void *arg);

void print_list(struct file_list *head);

#endif
