#include <sys/stat.h>
#include "client.h"

int parse_args(const struct dc_env *env, struct dc_error *err, void *arg)
{
    struct client_opts *opts = (struct client_opts *)arg;
    int ret;

    if(opts->argc < DEFAULT_ARGS_NUM)
    {
        opts->msg = strdup("Too few arguments\n");
        return FATALERROR;
    }

    opts->dest_ip = strdup(opts->argv[IP_INDEX]);

    opts->ip_family = get_ip_family(opts->dest_ip);
    if(opts->ip_family == -1)
    {
        opts->msg = strdup("IP address is not a valid IPv4 or IPv6 address\n");
        return FATALERROR;
    }

    ret = parse_in_port_t(opts);
    if(ret == -1)
    {
        return FATALERROR;
    }

    return GETFILEINFOS;
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

int parse_in_port_t(struct client_opts *opts)
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

    opts->dest_port = (in_port_t)parsed_value;
    return 0;
}

int get_files(const struct dc_env *env, struct dc_error *err, void *arg)
{
    struct client_opts *opts = (struct client_opts *)arg;

    opts->head = malloc(sizeof (struct file_list));
    opts->head->file = NULL;
    opts->head->next = NULL;
    opts->head->file_path = NULL;

    struct file_list *current_node;
    wordexp_t p;
    int ret;

    current_node = opts->head;
    ret = wordexp(opts->argv[FILE_INDEX], &p , 0);
    if(ret == -1)
    {
        opts->msg = strdup("wordexp failed\n");
        return FATALERROR;
    }

    current_node->file_path = strdup(p.we_wordv[0]);
    if(p.we_wordc > 1) //if first arg was expandable, iterate thru expanded first arg and store into linked list
    {
        for(size_t i = 1; i < p.we_wordc; ++i)
        {
            current_node->next = malloc(sizeof(struct file_list));
            current_node = current_node->next;
            current_node->file_path = strdup(p.we_wordv[i]);
            current_node->file = NULL;
            current_node->next = NULL;
        }
    }
    wordfree(&p);

    if(opts->argc > DEFAULT_ARGS_NUM)
    {
        //repeat for rest of file args
        for(int i = FILE_INDEX+1; i < opts->argc; ++i)
        {
            ret = wordexp(opts->argv[i], &p, 0);
            if(ret == 0)
            {
                for (size_t j = 0; j < p.we_wordc; ++j) {
                    current_node->next = malloc(sizeof(struct file_list));
                    current_node = current_node->next;
                    current_node->file_path = strdup(p.we_wordv[j]);
                    current_node->file = NULL;
                    current_node->next = NULL;
                }
                wordfree(&p);
            }
            else
            {
                opts->msg = strdup("wordexp failed\n");
                return FATALERROR;
            }
        }
    }

    ret = check_files(opts->head);
    if(ret == -1)
    {
        opts->msg = strdup("Error opening files\n");
        return FATALERROR;
    }

    get_file_infos(opts->head);
    print_list(opts->head);

    return SOCKET;
}

int check_files(struct file_list *head)
{
    struct file_list *file_to_check;
    int fd;

    file_to_check = head;
    while(file_to_check != NULL)
    {
        fd = open(file_to_check->file_path, O_RDONLY);
        if(fd == -1)
        {
            printf("failed to open file: %s\n", file_to_check->file_path);
            return -1;
        }
        file_to_check->file = malloc(sizeof(struct file_info));
        file_to_check->file->file_fd = fd;
        file_to_check = file_to_check->next;
    }

    return 0;
}

void get_file_infos(struct file_list *head)
{
    struct file_list *node;

    node = head;
    while(node != NULL)
    {
        node->file->file_name = sanitize_file_name(node->file_path);
        node->file->file_name_size = strlen((char *) node->file->file_name);

        struct stat st;
        fstat(node->file->file_fd, &st); //get file size
        node->file->file_size = st.st_size;
        node = node->next;
    }
}

uint8_t *sanitize_file_name(char *file_path)
{
    uint8_t *sanitized_name;
    const char *slash = strrchr(file_path, '/');

    if(slash == NULL)
    {
        sanitized_name = (uint8_t *) strdup(file_path);
    }
    else
    {
        sanitized_name = (uint8_t *) strdup(slash+1);
    }

    return sanitized_name;
}

int do_socket(const struct dc_env *env, struct dc_error *err, void *arg)
{
    struct client_opts *opts = (struct client_opts *)arg;
    opts->socket_fd = socket(opts->ip_family, SOCK_STREAM, 0);
    if(opts->socket_fd == -1)
    {
        opts->msg = strdup("socket failed\n");
        return FATALERROR;
    }
    return CONNECT;
}

int do_connect(const struct dc_env *env, struct dc_error *err, void *arg)
{
    struct client_opts *opts = (struct client_opts *)arg;
    struct sockaddr_in ipv4_addr;
    struct sockaddr_in6 ipv6_addr;
    int ret;

    memset(&ipv4_addr, 0, sizeof(ipv4_addr));
    memset(&ipv6_addr, 0, sizeof(ipv6_addr));

    if(opts->ip_family == AF_INET)
    {
        if(inet_pton(opts->ip_family, opts->dest_ip, &ipv4_addr.sin_addr) != 1)
        {
            opts->msg = strdup("Invalid IP address\n");
            return FATALERROR;
        }
        ipv4_addr.sin_family = AF_INET;
        ipv4_addr.sin_port = htons(opts->dest_port);

        ret = connect(opts->socket_fd, (struct sockaddr *)&ipv4_addr, sizeof(ipv4_addr));
        if (ret == -1)
        {
            opts->msg = strdup("Failed to connect to server\n");
            return FATALERROR;
        }
    }
    else if(opts->ip_family == AF_INET6)
    {
        if(inet_pton(opts->ip_family, opts->dest_ip, &ipv6_addr.sin6_addr) != 1)
        {
            opts->msg = strdup("Invalid IP address\n");
            return FATALERROR;
        }
        ipv6_addr.sin6_family = AF_INET6;
        ipv6_addr.sin6_port = htons(opts->dest_port);

        ret = connect(opts->socket_fd, (struct sockaddr *)&ipv6_addr, sizeof(ipv6_addr));
        if (ret == -1)
        {
            opts->msg = strdup("Failed to connect to server\n");
            return FATALERROR;
        }
    }
    printf("Connected to socket: %s:%u\n", opts->dest_ip, opts->dest_port);
    return SENDALL;
}

int send_all(const struct dc_env *env, struct dc_error *err, void *arg)
{
    struct client_opts *opts = (struct client_opts *)arg;
    struct file_list *node_to_send;

    node_to_send = opts->head;
    while(node_to_send != NULL)
    {
        write(opts->socket_fd, &node_to_send->file->file_name_size, 1);
        write(opts->socket_fd, node_to_send->file->file_name, node_to_send->file->file_name_size);
        write(opts->socket_fd, &node_to_send->file->file_size, sizeof(off_t));
        send_file_content(node_to_send->file->file_fd, opts->socket_fd, node_to_send->file->file_size);
        printf("Wrote file_fd=%d to server\n", node_to_send->file->file_fd);
        node_to_send = node_to_send->next;
    }

    return CLEANUP;
}

void send_file_content(int src_fd, int dest_fd, size_t count)
{
    char *buffer;
    ssize_t rbytes;

    buffer = malloc(count);

    if(buffer == NULL)
    {
        perror("malloc\n");
    }

    while((rbytes = read(src_fd, buffer, count)) > 0)
    {
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

int clean_up(const struct dc_env *env, struct dc_error *err, void *arg)
{
    struct client_opts *opts = (struct client_opts *)arg;
    struct file_list *node_to_delete;

    if(opts->head != NULL)
    {
        while(opts->head != NULL)
        {
            node_to_delete = opts->head;
            opts->head = opts->head->next;
            if(node_to_delete->file_path != NULL)
            {
                free(node_to_delete->file_path);
            }
            if(node_to_delete->file != NULL)
            {
                if(node_to_delete->file->file_name != NULL)
                {
                    free(node_to_delete->file->file_name);
                }
                close(node_to_delete->file->file_fd);
                free(node_to_delete->file);
            }
            free(node_to_delete);
        }
    }

    if(opts->msg != NULL)
    {
        free(opts->msg);
    }

    close(opts->socket_fd);

    return DC_FSM_EXIT;
}

int print_error(const struct dc_env *env, struct dc_error *err, void *arg)
{
    struct client_opts *opts = (struct client_opts *)arg;
    printf("%s\n", opts->msg);
    printf("Client Usage: <ipv4 or ipv6 addr> <port> <file paths>\n");

    return CLEANUP;
}

void print_list(struct file_list *head)
{
    struct file_list *node_to_print;
    node_to_print = head;
    while(node_to_print != NULL && node_to_print->file != NULL)
    {
        printf("____________________________________\n");
        printf("File Path: %s\n", node_to_print->file_path);
        printf("File fd: %d\n", node_to_print->file->file_fd);
        printf("File Name: %s\n", node_to_print->file->file_name);
        printf("File Name Size: %hhu\n", node_to_print->file->file_name_size);
        printf("File Size: %llu\n", node_to_print->file->file_size);
        node_to_print = node_to_print->next;
    }
}
