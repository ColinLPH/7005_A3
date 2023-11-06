#include "server.h"

int main(int argc, char *argv[])
{
    struct dc_env *env;
    struct dc_error *err;
    struct server_opts opts;

    memset(&opts, 0, sizeof(struct server_opts));
    err = dc_error_create(false);
    env = dc_env_create(err, false, NULL);
    opts.argc = argc;
    opts.argv = argv;

    struct dc_fsm_info *fsm_info;
    struct dc_fsm_transition transitions[] = {

    };
}
