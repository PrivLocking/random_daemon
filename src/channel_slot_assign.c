#include "inc/yt_common.h"

VideoList video_list = {0};
ScheduleData schedule = {0};
int channelIndex = 0;

char proxy_list_path[MAX_CHANNELS][256] = {
    DEFAULT_PROXY_LIST1, DEFAULT_PROXY_LIST2, DEFAULT_PROXY_LIST3,
    DEFAULT_PROXY_LIST4, DEFAULT_PROXY_LIST5
};

char *nowStr(void) {
    time_t now = time(NULL);
    if (now == (time_t)-1) {
        return NULL;
    }

    struct tm *tm_info = localtime(&now);
    if (tm_info == NULL) {
        return NULL;
    }

    char *timeStr = malloc(20 * sizeof(char));
    if (timeStr == NULL) {
        return NULL;
    }

    strftime(timeStr, 20, "%Y-%m-%d %H:%M:%S", tm_info);
    return timeStr;
}

// Get midnight of current day
time_t get_day_start(void) {
    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    tm->tm_hour = 0;
    tm->tm_min = 0;
    tm->tm_sec = 0;
    return mktime(tm);
}

// Generate random float between min and max
float random_float(float min, float max) {
    return min + (max - min) * ((float)rand() / (float)RAND_MAX);
}

// Adjust slots to meet all constraints
void adjust_slots_to_constraints(float slots[], int slotsPerChannel, int channelSize) {
    float targetPerChannel = 24.0 / channelSize;
    float channelMin = targetPerChannel * (1.0 - TOLERANCE);
    float channelMax = targetPerChannel * (1.0 + TOLERANCE);
    int totalSlots = slotsPerChannel * channelSize;
    
    // Step 1: Fix individual channel violations
    for(int ch = 0; ch < channelSize; ch++) {
        float channelSum = 0;
        for(int s = 0; s < slotsPerChannel; s++) {
            channelSum += slots[ch * slotsPerChannel + s];
        }
        
        if(channelSum < channelMin) {
            // Calculate exact adjustment needed
            float deficit = channelMin - channelSum;
            float adjustment = deficit / slotsPerChannel;
            
            for(int s = 0; s < slotsPerChannel; s++) {
                slots[ch * slotsPerChannel + s] += adjustment;
                // Clamp to bounds
                if(slots[ch * slotsPerChannel + s] > TIME_MAX) {
                    slots[ch * slotsPerChannel + s] = TIME_MAX;
                } else if(slots[ch * slotsPerChannel + s] < TIME_MIN) {
                    slots[ch * slotsPerChannel + s] = TIME_MIN;
                }
            }
        } else if(channelSum > channelMax) {
            // Calculate exact adjustment needed
            float excess = channelSum - channelMax;
            float adjustment = excess / slotsPerChannel;
            
            for(int s = 0; s < slotsPerChannel; s++) {
                slots[ch * slotsPerChannel + s] -= adjustment;
                // Clamp to bounds
                if(slots[ch * slotsPerChannel + s] < TIME_MIN) {
                    slots[ch * slotsPerChannel + s] = TIME_MIN;
                } else if(slots[ch * slotsPerChannel + s] > TIME_MAX) {
                    slots[ch * slotsPerChannel + s] = TIME_MAX;
                }
            }
        }
    }
    
    // Step 2: Fix total time to exactly 24.0
    float totalSum = 0;
    for(int i = 0; i < totalSlots; i++) {
        totalSum += slots[i];
    }
    
    if(totalSum != 24.0) {
        float difference = 24.0 - totalSum;
        float adjustment = difference / totalSlots;
        
        for(int i = 0; i < totalSlots; i++) {
            slots[i] += adjustment;
            // Clamp to bounds
            if(slots[i] > TIME_MAX) {
                slots[i] = TIME_MAX;
            } else if(slots[i] < TIME_MIN) {
                slots[i] = TIME_MIN;
            }
        }
    }
    
    // Step 3: Final precise adjustment to exactly 24.0
    totalSum = 0;
    for(int i = 0; i < totalSlots; i++) {
        totalSum += slots[i];
    }
    
    if(totalSum != 24.0) {
        float remainder = 24.0 - totalSum;
        int lastSlot = totalSlots - 1;
        
        slots[lastSlot] += remainder;
        // Final bounds check
        if(slots[lastSlot] > TIME_MAX) {
            slots[lastSlot] = TIME_MAX;
        } else if(slots[lastSlot] < TIME_MIN) {
            slots[lastSlot] = TIME_MIN;
        }
    }
}

// Generate daily schedule
void generate_daily_schedule(void) {
    // Calculate slots per channel
    float avgSlotTime = (TIME_MIN + TIME_MAX) / 2.0;
    int minRequiredSlots = ceil(24.0 / TIME_MAX);
    int baseSlots = (minRequiredSlots > ceil(24.0 / avgSlotTime)) ? minRequiredSlots : ceil(24.0 / avgSlotTime);
    
    schedule.slotsPerChannel = (baseSlots + CHANNEL_SIZE - 1) / CHANNEL_SIZE;  // Round up division
    schedule.totalSlots = schedule.slotsPerChannel * CHANNEL_SIZE;
    
    // Allocate memory for slots
    if(schedule.slots) {
        free(schedule.slots);
    }
    schedule.slots = malloc(schedule.totalSlots * sizeof(float));
    if(!schedule.slots) {
        fprintf(stderr, "ERROR: Memory allocation failed for schedule slots\n");
        exit(1);
    }
    
    // Seed random number generator with current date
    time_t dayStart = get_day_start();
    //srand(dayStart);
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand( dayStart ^ tv.tv_usec ^ time(NULL));
    
    // Generate random slots
    for(int i = 0; i < schedule.totalSlots; i++) {
        schedule.slots[i] = random_float(TIME_MIN, TIME_MAX);
    }
    
    // Adjust slots to meet constraints
    adjust_slots_to_constraints(schedule.slots, schedule.slotsPerChannel, CHANNEL_SIZE);
    
    // Reset current slot and mark as generated
    schedule.currentSlot = 0;
    schedule.dayStart = dayStart;
    schedule.isGenerated = 1;
    
    // Debug output
    printf("Generated daily schedule for %s:\n", nowStr());
    printf("  Slots per channel: %d\n", schedule.slotsPerChannel);
    printf("  Total slots: %d\n", schedule.totalSlots);
    
    float channelSums[CHANNEL_SIZE] = {0};
    for(int ch = 0; ch < CHANNEL_SIZE; ch++) {
        printf("  Channel %d: ", ch);
        for(int s = 0; s < schedule.slotsPerChannel; s++) {
            int slotIndex = ch * schedule.slotsPerChannel + s;
            printf("%.2f ", schedule.slots[slotIndex]);
            channelSums[ch] += schedule.slots[slotIndex];
        }
        printf("= %.2f hours\n", channelSums[ch]);
    }
    
    float totalSum = 0;
    for(int i = 0; i < CHANNEL_SIZE; i++) {
        totalSum += channelSums[i];
    }
    printf("  Total: %.2f hours\n", totalSum);
}

// Check if we need to regenerate schedule for new day
void check_daily_schedule(void) {
    time_t currentDayStart = get_day_start();
    
    if(!schedule.isGenerated || schedule.dayStart != currentDayStart) {
        printf("Generating new daily schedule...\n");
        generate_daily_schedule();
    }
}

// Get current channel index based on elapsed time
int get_current_channel_index(void) {
    check_daily_schedule();
    
    time_t now = time(NULL);
    double elapsedHours = difftime(now, schedule.dayStart) / 3600.0;
    
    // Find which slot we're currently in
    float accumulatedTime = 0;
    for(int i = 0; i < schedule.totalSlots; i++) {
        accumulatedTime += schedule.slots[i];
        if(elapsedHours <= accumulatedTime) {
            schedule.currentSlot = i;
            int tmpChannelIndex = i % CHANNEL_SIZE;
            return tmpChannelIndex;
        }
    }
    
    // If we've exceeded all slots, wrap around to first slot
    schedule.currentSlot = 0;
    return 0;
}

// Placeholder for YouTube URL validation (you need to implement this)
int is_valid_youtube_url(const char *url) {
    // Simple check for YouTube URL format
    if(strncmp(url, "https://www.youtube.com/watch?v=", 32) == 0) {
        // Check if video ID is 11 characters
        const char *videoId = url + 32;
        if(strlen(videoId) >= 11) {
            return 1;  // Valid
        }
    }
    return 0;  // Invalid
}

void load_video_list() {
    channelIndex = get_current_channel_index();
    FILE *fp = fopen(proxy_list_path[channelIndex], "r");
    if(!fp) {
        fprintf(stderr, "ERROR: Cannot open proxy list file '%s': %s\n", 
                proxy_list_path[channelIndex], strerror(errno));
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
        fprintf(stderr, "ERROR: No valid YouTube URLs found in '%s'\n", proxy_list_path[channelIndex]);
        fprintf(stderr, "Expected format: https://www.youtube.com/watch?v=VIDEO_ID\n");
        fprintf(stderr, "Where VIDEO_ID is exactly 11 characters [A-Za-z0-9_-]\n");
        exit(1);
    }

    printf("Loaded %d valid YouTube URLs from channel %d (%s). Current slot: %d, Time: %s\n", 
           video_list.count, channelIndex, proxy_list_path[channelIndex], 
           schedule.currentSlot, nowStr());
}

void check_and_reload_config_if_more_than_60s(void) {
    static time_t last_called = 0;
    static int is_initialized = 0;

    time_t now = time(NULL);

    if (!is_initialized) {
        load_video_list();
        last_called = now;
        is_initialized = 1;
        return;
    }

    if (difftime(now, last_called) >= CONFIG_REFRESH_INTERVAL) {
        load_video_list();
        last_called = now;
    } else {
        // Less than 60s since last call; do nothing
        printf("Config reload skipped (only %3.2f seconds since last call). channel %d (%s). Current slot: %d(%8.2f), %s\n", 
               difftime(now, last_called), 
               channelIndex, proxy_list_path[channelIndex], schedule.currentSlot, schedule.slots[schedule.currentSlot],
               nowStr());
    }
}

// Cleanup function
void cleanup_schedule(void) {
    if(schedule.slots) {
        free(schedule.slots);
        schedule.slots = NULL;
    }
    schedule.isGenerated = 0;
}

// Main function would go here with socket handling, etc.
// For now, just showing the core scheduling logic
