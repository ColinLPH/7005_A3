#include <arpa/inet.h>
#include "server.h"

int parse_args(const struct dc_env *env, struct dc_error *err, void *arg)
{
    struct server_opts *opts = (struct server_opts *)arg;

    if(opts->argc != SERVER_ARGS)
    {
        opts->msg = strdup("Invalid number of arguments\n");
        return FATALERROR;
    }

    opts->host_ip = strdup(opts->argv[IP_INDEX]);
    opts->ip_family = get_ip_family(opts->host_ip);
    if(opts->ip_family == -1)
    {
        opts->msg = strdup("IP address is not a valid IPv4 or IPv6 address\n");
        return FATALERROR;
    }

    char *path = strdup(opts->argv[DIR_INDEX]);
    printf("Pre-Sanitized Path: %s\n", path);
    sanitize_path(path);
    printf("Sanitized Path: %s\n", path);
    size_t len = strlen(path);
    strcpy(opts->dir_path, path);
    opts->dir_path[len] = '\0';
    free(path);

    if(parse_in_port_t(opts) == -1)
    {
        return FATALERROR;
    }

    printf("---------------------------- Server Options ----------------------------\n");
    printf("Server IP Address: %s\n", opts->host_ip);
    printf("Server Domain: %d\n", opts->ip_family);
    printf("Server Port: %hu\n", opts->host_port);
    printf("Server Directory: %s\n", opts->dir_path);
    printf("---------------------------- Server Options ----------------------------\n");

    return MAKEDIR;
}

int get_ip_family(const char *ip_addr)
{
    int domain;
    domain = -1;

    if(strstr(ip_addr, ":"))
    {
        domain = AF_INET6;
    }
    else if (strstr(ip_addr, "."))
    {
        domain = AF_INET;
    }

    return domain;

}

void sanitize_path(char *path)
{
    //change path so that it's a valid dir path
    size_t len = strlen(path);

    for(size_t i = 0; i < len; ++i)
    {
        if(is_valid_char(path[i]) == 0)
        {
            path[i] = '-';
        }
    }

}

int is_valid_char(const char c)
{
    return isalnum(c) || c == '_' || c == '-' || c == '/' || c == '.';
}

int parse_in_port_t(struct server_opts *opts)
{
    char *endptr;
    uintmax_t parsed_value;

    parsed_value = strtoumax(opts->argv[PORT_INDEX], &endptr, 10);

    if (errno != 0)
    {
        opts->msg = strdup("Error parsing in_port_t\n");
        return -1;
    }

    // Check if there are any non-numeric characters in the input string
    if (*endptr != '\0')
    {
        opts->msg = strdup("Invalid characters in input.\n");
        return -1;
    }

    // Check if the parsed value is within the valid range for in_port_t
    if (parsed_value > UINT16_MAX)
    {
        opts->msg = strdup("in_port_t value out of range.\n");
        return -1;
    }

    opts->host_port = (in_port_t)parsed_value;
    return 0;
}


int do_mkdir(const struct dc_env *env, struct dc_error *err, void *arg)
{
    struct server_opts *opts = (struct server_opts *)arg;

    if(mkdir(opts->dir_path, S_IRWXU) == -1)
    {
        if(errno != EEXIST)
        {
            opts->msg = strdup("Error creating directory\n");
            return FATALERROR;
        }

    }
    printf("Directory created: %s\n", opts->dir_path);

    return SOCKET;
}

int do_socket(const struct dc_env *env, struct dc_error *err, void *arg)
{
    struct server_opts *opts = (struct server_opts *)arg;

    opts->bind_fd = socket(opts->ip_family, SOCK_STREAM, 0);

    if(opts->bind_fd == -1)
    {
        opts->msg = strdup("Socket creation failed");
        return FATALERROR;
    }

    return BIND;
}

int do_bind(const struct dc_env *env, struct dc_error *err, void *arg)
{
    struct server_opts *opts = (struct server_opts *)arg;
    struct sockaddr_in ipv4_addr;
    struct sockaddr_in6 ipv6_addr;

    memset(&ipv4_addr, 0, sizeof(ipv4_addr));
    memset(&ipv6_addr, 0, sizeof(ipv6_addr));

    if(opts->ip_family == AF_INET)
    {
        if(inet_pton(opts->ip_family, opts->host_ip, &ipv4_addr.sin_addr) != 1)
        {
            opts->msg = strdup("Invalid IP address\n");
            return FATALERROR;
        }
        ipv4_addr.sin_family = AF_INET;
        ipv4_addr.sin_port = htons(opts->host_port);
        if(bind(opts->bind_fd, (struct sockaddr *)&ipv4_addr, sizeof(ipv4_addr)) == -1)
        {
            opts->msg = strdup("Binding failed\n");
            return FATALERROR;
        }
    }
    else if(opts->ip_family == AF_INET6)
    {
        if(inet_pton(opts->ip_family, opts->host_ip, &ipv6_addr.sin6_addr) != 1)
        {
            opts->msg = strdup("Invalid IP address\n");
            return FATALERROR;
        }
        ipv6_addr.sin6_family = AF_INET6;
        ipv6_addr.sin6_port = htons(opts->host_port);
        if(bind(opts->bind_fd, (struct sockaddr *)&ipv6_addr, sizeof(ipv6_addr)) == -1)
        {
            opts->msg = strdup("Binding failed\n");
            return FATALERROR;
        }
    }
    else
    {
        opts->msg = strdup("Invalid domain\n");
        return FATALERROR;
    }

    printf("Bound to socket: %s:%u\n", opts->host_ip, opts->host_port);
    return LISTEN;
}

int do_listen(const struct dc_env *env, struct dc_error *err, void *arg)
{
    struct server_opts *opts = (struct server_opts *)arg;
    if(listen(opts->bind_fd, SOMAXCONN) == -1)
    {
        opts->msg = strdup("Listen failed\n");
        return FATALERROR;
    }
    printf("Listening for incoming connections...\n");
    return RUN;
}

void handle_data_in(int client_fd, struct server_opts *opts, fd_set *readfds, int *client_sockets, size_t i)
{
    struct file_info file;
    ssize_t rbytes;

    memset(&file, 0, sizeof(struct file_info));
    rbytes = read(client_fd, &file.file_name_size, 1);
    if(rbytes <= 0)
    {
        // Connection closed or error
        printf("Client %d disconnected\n", client_fd);
        close(client_fd);
        FD_CLR((unsigned int)client_fd, readfds); // Remove the closed socket from the set
        client_sockets[i] = 0;
        return;
    }
    printf("-------------------------------------------\n");
    printf("File name size: %d\n", file.file_name_size);

    //read file_name
    file.file_name = malloc(file.file_name_size+1);
    if(file.file_name == NULL)
    {
        printf("error malloc-ing\n");
        return;
    }
    rbytes = read(client_fd, file.file_name, file.file_name_size);
    if(rbytes <= 0)
    {
        // Connection closed or error
        printf("Client %d disconnected\n", client_fd);
        close(client_fd);
        FD_CLR((unsigned int)client_fd, readfds); // Remove the closed socket from the set
        client_sockets[i] = 0;
    }
    file.file_name[file.file_name_size] = '\0';
    printf("File name: %s\n", file.file_name);

    //read file_size
    rbytes = read(client_fd, &file.file_size, sizeof(file.file_size));
    if(rbytes <= 0)
    {
        // Connection closed or error
        printf("Client %d disconnected\n", client_fd);
        close(client_fd);
        FD_CLR((unsigned int)client_fd, readfds); // Remove the closed socket from the set
        client_sockets[i] = 0;

    }
    printf("File size: %lld\n", file.file_size);

    //create file
    printf("Creating file...\n");
    create_file(opts, &file);
    //copy file content over to created file
    copy_paste(client_fd, file.filefd, file.file_size);
    printf("File contents copied\n");
    close(file.filefd);
    free(file.file_name);
}

int create_file(struct server_opts *opts, struct file_info *file)
{
    char *final_path;
    final_path = generate_file_name(opts->dir_path, file->file_name);

    file->filefd = open(final_path, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
    if(file->filefd == -1)
    {
        printf("Failed to create file, file path: %s\n", final_path);
        free(final_path);
        return -1;
    }
    printf("Created file with path: %s\n", final_path);
    free(final_path);

    return 0;
}

char *generate_file_name(char *dir_path, char *file_name)
{
    char *final_path;
    size_t dir_len;
    size_t file_len;
    size_t final_len;

    dir_len = strlen(dir_path);
    file_len = strlen(file_name);

    final_len = dir_len + file_len + 2;
    final_path = malloc(final_len);

    strcpy(final_path, dir_path);
    final_path[dir_len] = '/';
    strcpy(&final_path[dir_len+1], file_name);

    printf("Final file path: %s\n", final_path);

    return final_path;
}

void copy_paste(int src_fd, int dest_fd, int64_t count)
{
    char *buffer;
    ssize_t rbytes;
    ssize_t index;

    buffer = malloc(count);

    if(buffer == NULL)
    {
        perror("malloc\n");
        return;
    }

    rbytes = 0;
    index = 0;
    while(rbytes < count)
    {
        count -= rbytes;
        index += rbytes;
        rbytes = read(src_fd, &buffer[index], count);
        if(rbytes == -1)
        {
            perror("read\n");
        }
        ssize_t wbytes;

        wbytes = write(dest_fd, buffer, rbytes);

        if(wbytes == -1)
        {
            perror("write\n");
        }
    }

    if(rbytes == -1)
    {
        perror("read\n");
    }

    free(buffer);
}

int socket_close(int fd)
{
    return close(fd);
}

int clean_up(const struct dc_env *env, struct dc_error *err, void *arg)
{
    struct server_opts *opts = (struct server_opts *)arg;

    close(opts->bind_fd);
    if(opts->msg != NULL)
    {
        free(opts->msg);
    }

    return DC_FSM_EXIT;
}



