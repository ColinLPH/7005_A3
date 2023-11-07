#include "server.h"

int volatile exit_flag = false;

void sigHandler(int signal) {
    printf("Exiting Run\n");
    exit_flag = true;
}

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
            {DC_FSM_INIT, PARSEARGS, parse_args},
            {PARSEARGS, MAKEDIR, do_mkdir},
            {MAKEDIR, SOCKET, do_socket},
            {SOCKET, BIND, do_bind},
            {BIND, LISTEN, do_listen},
            {LISTEN, RUN, run_server},
            {RUN, CLEANUP, clean_up},
            {RUN, FATALERROR, print_error},
            {PARSEARGS, FATALERROR, print_error},
            {MAKEDIR, FATALERROR, print_error},
            {SOCKET, FATALERROR, print_error},
            {BIND, FATALERROR, print_error},
            {LISTEN, FATALERROR, print_error},
            {FATALERROR, CLEANUP, clean_up},
            {CLEANUP, DC_FSM_EXIT, NULL}
    };

    fsm_info = dc_fsm_info_create(env, err, "server_fsm");

    if (dc_error_has_no_error(err)) {
        int from_state;
        int to_state;

        // Register signal interrupt
        signal(SIGINT, sigHandler);

        dc_fsm_run(env, err, fsm_info, &from_state, &to_state, &opts, transitions);
        dc_fsm_info_destroy(env, &fsm_info);
    }

    free(err);
    free(env);

    return EXIT_SUCCESS;
}

int run_server(const struct dc_env *env, struct dc_error *err, void *arg)
{
    struct server_opts *opts = (struct server_opts *)arg;

    int *client_sockets = NULL;
    size_t max_clients = 0;
    int max_fd, activity, new_socket, sd;
    int addrlen;
    struct sockaddr_un address; //this shouldn't be sockaddr_un but it still works?
    fd_set readfds;

    free(opts->host_ip);
    while(exit_flag != 1)
    {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add the server socket to the set
        FD_SET((unsigned int)opts->bind_fd, &readfds);
        max_fd = opts->bind_fd;

        // Add the client sockets to the set
        for(size_t i = 0; i < max_clients; i++)
        {
            sd = client_sockets[i];

            if(sd > 0)
            {
                FD_SET((unsigned int)sd, &readfds);
            }

            if(sd > max_fd)
            {
                max_fd = sd;
            }
        }

        // Use select to monitor sockets for read readiness
        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if(activity < 0)
        {
            perror("Select error");
            return -1;
        }

        // Handle new client connections
        if(FD_ISSET((unsigned int)opts->bind_fd, &readfds))
        {
            if((new_socket = accept(opts->bind_fd, (struct sockaddr *) &address, (socklen_t * ) & addrlen)) == -1)
            {
                perror("Accept error");
                return -1;
            }

            printf("New connection established\n");

            // Increase the size of the client_sockets array
            max_clients++;
            client_sockets = (int *)realloc(client_sockets, sizeof(int) * max_clients);
            client_sockets[max_clients - 1] = new_socket;
        }

        // Handle incoming data from existing clients
        for(size_t i = 0; i < max_clients; i++)
        {
            sd = client_sockets[i];

            if(FD_ISSET((unsigned int)sd, &readfds))
            {
                //handle data in
                handle_data_in(sd, opts, &readfds, client_sockets, i);
            }
        }

    }

    // Cleanup and close all client sockets
    for(size_t i = 0; i < max_clients; i++)
    {
        sd = client_sockets[i];
        if(sd > 0 && socket_close(sd) == -1)
        {
            opts->msg = strdup("Error closing client sockets\n");
            return FATALERROR;

        }
    }

    // Free the client_sockets array
    free(client_sockets);
    if(socket_close(opts->bind_fd) == -1)
    {
        opts->msg = strdup("Error closing bind socket\n");
        return FATALERROR;
    }

    printf("Server finished running... cleaning up\n");

    return CLEANUP;
}
