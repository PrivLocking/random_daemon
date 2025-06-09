#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <openssl/md5.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DEFAULT_SOCKET_PATH "/wwwFS.out/sock.youtube.recommend.sock"
#define DEFAULT_PROXY_LIST "/etc/proxyList.txt"
#define LOCK_PORT 38281
#define MAX_LINES 1000
#define MAX_LINE_LEN 256
#define BUFFER_SIZE 4096
#define RELOAD_INTERVAL 1000

typedef struct {
    char **lines;
    int count;
} VideoList;

static VideoList video_list = {0};
static int request_count = 0;
static unsigned char md5_seed[MD5_DIGEST_LENGTH];
static char socket_path[256] = DEFAULT_SOCKET_PATH;
static char proxy_list_path[256] = DEFAULT_PROXY_LIST;

void print_help(const char *prog_name) {
    printf("YouTube Recommendation Daemon\n");
    printf("Usage: %s [-s socket_path] [-f proxy_list_file]\n", prog_name);
    printf("\nCurrent configuration:\n");
    printf("  Socket path: %s\n", socket_path);
    printf("  Proxy list file: %s\n", proxy_list_path);
    printf("\nThis daemon listens on a Unix socket and serves YouTube channel recommendations.\n");
    printf("It reads YouTube URLs from the proxy list file and redirects users through a\n");
    printf("two-step process for referrer header purposes.\n");
}

void md5_hash(const char *input, size_t len, unsigned char *output) {
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, input, len);
    MD5_Final(output, &ctx);
}

void init_seed() {
    time_t now = time(NULL);
    char timestamp[32];
    int len = snprintf(timestamp, sizeof(timestamp), "%ld", now);
    md5_hash(timestamp, len, md5_seed);
}

void generate_random_url(char *url_buf, int *video_index) {
    time_t now = time(NULL);
    char input[64];
    unsigned char new_md5[MD5_DIGEST_LENGTH];
    
    int timestamp_len = snprintf(input, sizeof(input), "%ld", now);
    memcpy(input + timestamp_len, md5_seed, MD5_DIGEST_LENGTH);
    
    md5_hash(input, timestamp_len + MD5_DIGEST_LENGTH, new_md5);
    memcpy(md5_seed, new_md5, MD5_DIGEST_LENGTH);
    
    unsigned char *last_5 = &new_md5[MD5_DIGEST_LENGTH-5];
    int url_len = (last_5[4] & 0x07) + 8;
    uint32_t video_idx = *((uint32_t*)last_5) % video_list.count;
    
    for(int i = 0; i < url_len; i++) {
        sprintf(&url_buf[i*2], "%02x", new_md5[i]);
    }
    url_buf[url_len*2] = '\0';
    *video_index = video_idx;
}

int is_valid_youtube_url(const char *line) {
    const char prefix[] = "https://www.youtube.com/watch?v=";
    if(strncmp(line, prefix, strlen(prefix)) != 0) return 0;
    
    const char *video_id = line + strlen(prefix);
    if(strlen(video_id) != 11) return 0;
    
    for(int i = 0; i < 11; i++) {
        char c = video_id[i];
        if(!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || 
             (c >= '0' && c <= '9') || c == '_' || c == '-')) {
            return 0;
        }
    }
    return 1;
}

void load_video_list() {
    FILE *fp = fopen(proxy_list_path, "r");
    if(!fp) {
        fprintf(stderr, "ERROR: Cannot open proxy list file '%s': %s\n", 
                proxy_list_path, strerror(errno));
        fprintf(stderr, "Please ensure the file exists and is readable.\n");
        exit(1);
    }
    
    if(video_list.lines) {
        for(int i = 0; i < video_list.count; i++) {
            free(video_list.lines[i]);
        }
        free(video_list.lines);
    }
    
    video_list.lines = malloc(MAX_LINES * sizeof(char*));
    if(!video_list.lines) {
        fprintf(stderr, "ERROR: Memory allocation failed for video list\n");
        exit(1);
    }
    video_list.count = 0;
    
    char line[MAX_LINE_LEN];
    int line_num = 0;
    while(fgets(line, sizeof(line), fp) && video_list.count < MAX_LINES) {
        line_num++;
        // Remove both \r and \n
        char *p = line;
        while(*p && *p != '\r' && *p != '\n') p++;
        *p = '\0';
        
        if(strlen(line) == 0) continue;
        
        if(is_valid_youtube_url(line)) {
            video_list.lines[video_list.count] = strdup(line);
            if(!video_list.lines[video_list.count]) {
                fprintf(stderr, "ERROR: Memory allocation failed for line %d\n", line_num);
                exit(1);
            }
            video_list.count++;
        } else {
            printf("WARNING: Skipping invalid YouTube URL at line %d: %s\n", line_num, line);
        }
    }
    
    fclose(fp);
    
    if(video_list.count == 0) {
        fprintf(stderr, "ERROR: No valid YouTube URLs found in '%s'\n", proxy_list_path);
        fprintf(stderr, "Expected format: https://www.youtube.com/watch?v=VIDEO_ID\n");
        fprintf(stderr, "Where VIDEO_ID is exactly 11 characters [A-Za-z0-9_-]\n");
        exit(1);
    }
    
    printf("Loaded %d valid YouTube URLs from %s\n", video_list.count, proxy_list_path);
}

void extract_host_and_path(const char *request, char *host, char *path) {
    const char *host_start = strstr(request, "Host: ");
    if(host_start) {
        host_start += 6;
        const char *host_end = strstr(host_start, "\r\n");
        if(!host_end) host_end = strstr(host_start, "\n");
        if(host_end) {
            int host_len = host_end - host_start;
            strncpy(host, host_start, host_len);
            host[host_len] = '\0';
        }
    }
    
    // Extract path from "GET /path HTTP/1.1"
    if(strncmp(request, "GET ", 4) == 0) {
        const char *path_start = request + 4;
        const char *path_end = strchr(path_start, ' ');
        if(path_end) {
            int path_len = path_end - path_start;
            strncpy(path, path_start, path_len);
            path[path_len] = '\0';
        }
    }
}

void send_html_response(int client_fd, const char *redirect_url) {
    const char *html_template = 
        "<!DOCTYPE html>\n"
        "<html><head><meta charset=\"UTF-8\"><title>Recommendation</title></head>\n"
        "<body>\n"
        "<h1>Hi, I am the owner of the blogger</h1>\n"
        "<p>I recommend my friend's channel.<br>\n"
        "She works as a Cantonese teacher for more than 40 years.<br>\n"
        "Her students mostly speak English.<br>\n"
        "I recommend it.</p>\n"
        "<p>嗨，我係呢個博客嘅主人<br>\n"
        "我推薦我朋友嘅頻道。<br>\n"
        "佢做咗粵語老師超過40年。<br>\n"
        "佢嘅學生大部分都係講英文嘅。<br>\n"
        "我推薦佢。</p>\n"
        "<script>\n"
        "setTimeout(function(){\n"
        "window.location.href='%s';\n"
        "}, 10000);\n"
        "</script>\n"
        "</body></html>";
    
    char html_content[4096];
    snprintf(html_content, sizeof(html_content), html_template, redirect_url);
    
    char response[8192];
    snprintf(response, sizeof(response),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "Content-Length: %ld\r\n"
        "\r\n"
        "%s", strlen(html_content), html_content);
    
    send(client_fd, response, strlen(response), 0);
}

void handle_request(int client_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer)-1, 0);
    if(bytes_read <= 0) return;
    
    buffer[bytes_read] = '\0';
    
    char host[256] = {0};
    char path[256] = {0};
    
    if(strncmp(buffer, "GET ", 4) != 0) return;
    
    extract_host_and_path(buffer, host, path);
    printf( "=== buffer %ld [%s] === host %ld [%s] === path %ld [%s] \n", strlen(buffer), buffer, strlen(host), host, strlen(path), path );

    
    request_count++;
    if(request_count % RELOAD_INTERVAL == 0) {
        load_video_list();
    }
    
    char random_url[64];
    int video_index;
    generate_random_url(random_url, &video_index);
    
    int path_len = strlen(path);
    if(path_len > 0 && path[path_len-1] == '/') {
        // Path ends with /, redirect to same path + random
        char redirect_url[512];
        if(strcmp(path, "/") == 0) {
            snprintf(redirect_url, sizeof(redirect_url), "https://%s/%s", host, random_url);
        } else {
            snprintf(redirect_url, sizeof(redirect_url), "https://%s%s%s", host, path, random_url);
        }
        send_html_response(client_fd, redirect_url);
    } else {
        // Path doesn't end with /, redirect to YouTube
        send_html_response(client_fd, video_list.lines[video_index]);
    }
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
            strncpy(proxy_list_path, argv[++i], sizeof(proxy_list_path)-1);
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
    load_video_list();
    
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
        
        handle_request(client_fd);
        close(client_fd);
    }
    
    close(server_fd);
    close(lock_fd);
    unlink(socket_path);
    return 0;
}
