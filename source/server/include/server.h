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
    RUN,
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
    int bind_fd;
};

struct file_info {
    int filefd;
    uint8_t file_name_size;
    char *file_name;
    off_t file_size;
};

int parse_args(const struct dc_env *env, struct dc_error *err, void *arg);
int get_ip_family(const char *ip_addr);
void sanitize_path(char *path);
int is_valid_char(char c);
int parse_in_port_t(struct server_opts *opts);

int do_mkdir(const struct dc_env *env, struct dc_error *err, void *arg);
int do_socket(const struct dc_env *env, struct dc_error *err, void *arg);
int do_bind(const struct dc_env *env, struct dc_error *err, void *arg);
int do_listen(const struct dc_env *env, struct dc_error *err, void *arg);
int run_server(const struct dc_env *env, struct dc_error *err, void *arg);
int socket_close(int fd);
void handle_data_in(int client_fd, struct server_opts *opts, fd_set *readfds, int *client_sockets, size_t i);
int create_file(struct server_opts *opts, struct file_info *file);
char *generate_file_name(char *dir_path, char *file_name);
void copy_paste(int src_fd, int dest_fd, int64_t count);

int clean_up(const struct dc_env *env, struct dc_error *err, void *arg);
int print_error(const struct dc_env *env, struct dc_error *err, void *arg);


#endif
