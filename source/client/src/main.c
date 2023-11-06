#include "client.h"

int main(int argc, char *argv[])
{
    struct dc_error *err;
    struct dc_env *env;
    struct client_opts opts;
    int ret_val;

    memset(&opts, 0, sizeof(struct client_opts));
    opts.argc = argc;
    opts.argv = argv;
    err = dc_error_create(false);
    env = dc_env_create(err, false, NULL);
    ret_val = EXIT_SUCCESS;

    struct dc_fsm_info *fsm_info;
    struct dc_fsm_transition transitions[] = {
            {DC_FSM_INIT, PARSEARGS, parse_args},
            {PARSEARGS, GETFILEINFOS, get_files},
            {GETFILEINFOS, SOCKET, do_socket},
            {SOCKET, CONNECT, do_connect},
            {CONNECT, SENDALL, send_all},
            {SENDALL, CLEANUP, clean_up},
            {PARSEARGS, FATALERROR, print_error},
            {GETFILEINFOS, FATALERROR, print_error},
            {SOCKET, FATALERROR, print_error},
            {CONNECT, FATALERROR, print_error},
            {SENDALL, FATALERROR, print_error},
            {FATALERROR, CLEANUP, clean_up},
            {CLEANUP, DC_FSM_EXIT, NULL}
    };

    fsm_info = dc_fsm_info_create(env, err, "client_fsm");

    if (dc_error_has_no_error(err))
    {
        int from_state;
        int to_state;

        ret_val = dc_fsm_run(env, err, fsm_info, &from_state, &to_state, &opts, transitions);
        dc_fsm_info_destroy(env, &fsm_info);
    }
    free(err);
    free(env);

    return ret_val;
}
