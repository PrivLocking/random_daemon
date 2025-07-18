#ifndef YT_COMMON_H
#define YT_COMMON_H

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
#include <openssl/aes.h>
#include <math.h>
#include <sys/time.h>


#define DEFAULT_SOCKET_PATH "/wwwFS.out/sock.youtube.recommend.sock"
#define DEFAULT_PROXY_LIST1 "/etc/proxyList1.txt"
#define DEFAULT_PROXY_LIST2 "/etc/proxyList2.txt"
#define DEFAULT_PROXY_LIST3 "/etc/proxyList3.txt"
#define DEFAULT_PROXY_LIST4 "/etc/proxyList4.txt"
#define DEFAULT_PROXY_LIST5 "/etc/proxyList5.txt"
#define LOCK_PORT 38281
#define MAX_LINES 50000
#define MAX_LINE_LEN 256
#define BUFFER_SIZE 4096
#define RELOAD_INTERVAL 10
#define CONFIG_REFRESH_INTERVAL 60

// New macros for dynamic scheduling
//#define TIME_MIN 0.9
//#define TIME_MAX 3.9
//#define TIME_MIN 2.55 // 3*1.15 == 3.45   , 3 * 0.85 == 2.55
//#define TIME_MAX 3.45
#define TIME_MIN 2.805 // 3.3*1.15 == 3.795   , 3.3 * 0.85 == 2.805
#define TIME_MAX 3.795

//#define TOLERANCE 0.2   // 20% tolerance
//#define TOLERANCE 0.05   // 5% tolerance
#define TOLERANCE 0.01   // 1% tolerance
                         //
#define CHANNEL_SIZE 2  // Can be changed to 3, 4, 5, etc.
#define MAX_CHANNELS 5
#define MAX_SLOTS_PER_CHANNEL 12

void check_and_reload_config_if_more_than_60s(void) ;
void handle_html_request(int client_fd) ;
void init_seed(void) ;

typedef struct {
    float *slots;           // Array of time slots
    int slotsPerChannel;    // Number of slots per channel
    int totalSlots;         // Total number of slots
    int currentSlot;        // Current slot index
    time_t dayStart;        // Start of current day (midnight)
    int isGenerated;        // Flag to check if slots are generated for today
} ScheduleData;

typedef struct {
    char **lines;
    int count;
} VideoList;

extern char proxy_list_path[][256] ;
extern VideoList video_list ;

#endif
