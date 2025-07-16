#include "inc/yt_common.h"



static char socket_path[256] = DEFAULT_SOCKET_PATH;

void print_help(const char *prog_name) {
    printf("YouTube Recommendation Daemon\n");
    printf("Usage: %s [-s socket_path] \n", prog_name);
    printf("    [-f proxy_list_file]\n" );
    printf("    [-2 proxy_list_file2]\n" );
    printf("    [-3 proxy_list_file3]\n" );
    printf("    [-4 proxy_list_file4]\n" );
    printf("    [-5 proxy_list_file5]\n" );
    printf("\nCurrent configuration:\n");
    printf("  Socket path: %s\n", socket_path);
    printf("  Channel size: %d\n", CHANNEL_SIZE);
    printf("  Proxy list files:\n");
    for(int i = 0; i < CHANNEL_SIZE; i++) {
        printf("    Channel %d: %s\n", i, proxy_list_path[i]);
    }
    printf("\nThis daemon listens on a Unix socket and serves YouTube channel recommendations.\n");
    printf("It uses dynamic scheduling with %d channels rotating based on time slots.\n", CHANNEL_SIZE);
}



int try_lock_port() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1) return 0;

    int reuse = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(LOCK_PORT);

    if(bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        close(sock);
        return 0;
    }

    if(listen(sock, 1) == -1) {
        close(sock);
        return 0;
    }

    return sock;
}

int main(int argc, char *argv[]) {
    // Parse command line arguments
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "-s") == 0 && i+1 < argc) {
            strncpy(socket_path, argv[++i], sizeof(socket_path)-1);
        } else if(strcmp(argv[i], "-f") == 0 && i+1 < argc) {
            strncpy(proxy_list_path[0], argv[++i], sizeof(proxy_list_path[0])-1);
        } else if(strcmp(argv[i], "-2") == 0 && i+1 < argc) {
            strncpy(proxy_list_path[1], argv[++i], sizeof(proxy_list_path[1])-1);
        } else if(strcmp(argv[i], "-3") == 0 && i+1 < argc) {
            strncpy(proxy_list_path[1], argv[++i], sizeof(proxy_list_path[2])-1);
        } else if(strcmp(argv[i], "-4") == 0 && i+1 < argc) {
            strncpy(proxy_list_path[1], argv[++i], sizeof(proxy_list_path[3])-1);
        } else if(strcmp(argv[i], "-5") == 0 && i+1 < argc) {
            strncpy(proxy_list_path[1], argv[++i], sizeof(proxy_list_path[4])-1);
        } else if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        }
    }

    print_help(argv[0]);

    // Try to lock port to prevent multiple instances
    int lock_fd = try_lock_port();
    if(!lock_fd) {
        fprintf(stderr, "ERROR: Another instance is already running (port %d is in use)\n", LOCK_PORT);
        fprintf(stderr, "Please stop the existing instance before starting a new one.\n");
        exit(1);
    }

    printf("Instance lock acquired on port %d\n", LOCK_PORT);

    signal(SIGPIPE, SIG_IGN);

    init_seed();
    //load_video_list();
    check_and_reload_config_if_more_than_60s();

    // Remove existing socket file
    if(unlink(socket_path) == 0) {
        printf("Removed existing socket file: %s\n", socket_path);
    }

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if(server_fd == -1) {
        fprintf(stderr, "ERROR: Failed to create Unix socket: %s\n", strerror(errno));
        exit(1);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, socket_path);

    if(bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        fprintf(stderr, "ERROR: Failed to bind socket to '%s': %s\n", socket_path, strerror(errno));
        fprintf(stderr, "Please check if the directory exists and has proper permissions.\n");
        exit(1);
    }

    if(chmod(socket_path, 0666) == -1) {
        fprintf(stderr, "WARNING: Failed to set socket permissions: %s\n", strerror(errno));
    }

    if(listen(server_fd, 10) == -1) {
        fprintf(stderr, "ERROR: Failed to listen on socket: %s\n", strerror(errno));
        exit(1);
    }

    printf("Daemon started successfully\n");
    printf("Listening on socket: %s\n", socket_path);
    printf("Ready to serve requests...\n");

    while(1) {
        int client_fd = accept(server_fd, NULL, NULL);
        if(client_fd == -1) {
            if(errno == EINTR) continue;
            fprintf(stderr, "WARNING: Accept failed: %s\n", strerror(errno));
            continue;
        }

        handle_html_request(client_fd);
        close(client_fd);
    }

    close(server_fd);
    close(lock_fd);
    unlink(socket_path);
    return 0;
}
