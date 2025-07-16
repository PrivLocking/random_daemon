#include "inc/yt_common.h"

int request_count = 0;
unsigned char aes_seed[32];  // 256-bit AES key
unsigned char aes_counter[16];  // 128-bit counter
uint64_t counter = 0;


void md5_hash(const char *input, size_t len, unsigned char *output) {
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, input, len);
    MD5_Final(output, &ctx);
} // md5_hash

void init_seed(void) {
    // Collect multiple entropy sources
    time_t now = time(NULL);
    //char timestamp[32];
    //int len = snprintf(timestamp, sizeof(timestamp), "%ld", now);
    //md5_hash(timestamp, len, md5_seed);
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    // Mix multiple entropy sources
    char entropy_input[128];
    int len = snprintf(entropy_input, sizeof(entropy_input),
            "%ld_%ld_%d_%p_%u",
            now, ts.tv_nsec, getpid(), &entropy_input, (unsigned)random());

    // Generate AES key from mixed entropy
    unsigned char temp_hash[MD5_DIGEST_LENGTH];
    md5_hash(entropy_input, len, temp_hash);

    // Expand to 256-bit key using multiple MD5 rounds
    md5_hash((char*)temp_hash, MD5_DIGEST_LENGTH, aes_seed);
    md5_hash((char*)aes_seed, MD5_DIGEST_LENGTH, &aes_seed[16]);

    // Initialize counter with entropy
    memcpy(aes_counter, temp_hash, 16);
    counter = 0;
} // init_seed

void send_html_response(int client_fd, const char *redirect_url) {

    const char *html_template =
        "<!DOCTYPE html>\n"
        "<html lang=\"en\">\n"
        "<head>\n"
        "    <meta charset=\"UTF-8\">\n"
        "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "    <title>Recommendation</title>\n"
        "    <style>\n"
        "        * {\n"
        "            margin: 0;\n"
        "            padding: 0;\n"
        "            box-sizing: border-box;\n"
        "        }\n"
        "        \n"
        "        body {\n"
        "            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;\n"
        "            line-height: 1.6;\n"
        "            color: #333;\n"
        "            background: linear-gradient(135deg, #667eea 0%%, #764ba2 100%%);\n"
        "            min-height: 100vh;\n"
        "            display: flex;\n"
        "            flex-direction: column;\n"
        "            justify-content: center;\n"
        "            align-items: center;\n"
        "            padding: 20px;\n"
        "        }\n"
        "        \n"
        "        .container {\n"
        "            background: rgba(255, 255, 255, 0.95);\n"
        "            backdrop-filter: blur(10px);\n"
        "            border-radius: 20px;\n"
        "            padding: 30px;\n"
        "            max-width: 500px;\n"
        "            width: 100%%;\n"
        "            text-align: center;\n"
        "            box-shadow: 0 20px 40px rgba(0, 0, 0, 0.1);\n"
        "            animation: fadeIn 0.8s ease-out;\n"
        "        }\n"
        "        \n"
        "        @keyframes fadeIn {\n"
        "            from {\n"
        "                opacity: 0;\n"
        "                transform: translateY(30px);\n"
        "            }\n"
        "            to {\n"
        "                opacity: 1;\n"
        "                transform: translateY(0);\n"
        "            }\n"
        "        }\n"
        "        \n"
        "        h1 {\n"
        "            color: #4a5568;\n"
        "            margin-bottom: 25px;\n"
        "            font-size: clamp(1.8rem, 4vw, 2.5rem);\n"
        "            font-weight: 700;\n"
        "        }\n"
        "        \n"
        "        p {\n"
        "            margin-bottom: 20px;\n"
        "            font-size: clamp(1rem, 2.5vw, 1.2rem);\n"
        "            color: #2d3748;\n"
        "        }\n"
        "        \n"
        "        .countdown-container {\n"
        "            margin-top: 30px;\n"
        "            padding: 20px;\n"
        "            background: linear-gradient(45deg, #ff6b6b, #4ecdc4);\n"
        "            border-radius: 15px;\n"
        "            color: white;\n"
        "        }\n"
        "        \n"
        "        .countdown-text {\n"
        "            font-size: clamp(1rem, 2.5vw, 1.1rem);\n"
        "            margin-bottom: 15px;\n"
        "            font-weight: 500;\n"
        "        }\n"
        "        \n"
        "        .countdown-number {\n"
        "            font-size: clamp(3rem, 8vw, 4rem);\n"
        "            font-weight: bold;\n"
        "            text-shadow: 2px 2px 4px rgba(0, 0, 0, 0.3);\n"
        "            animation: pulse 1s infinite;\n"
        "            min-height: 80px;\n"
        "            display: flex;\n"
        "            align-items: center;\n"
        "            justify-content: center;\n"
        "        }\n"
        "        \n"
        "        @keyframes pulse {\n"
        "            0%%, 100%% {\n"
        "                transform: scale(1);\n"
        "            }\n"
        "            50%% {\n"
        "                transform: scale(1.1);\n"
        "            }\n"
        "        }\n"
        "        \n"
        "        .progress-bar {\n"
        "            width: 100%%;\n"
        "            height: 6px;\n"
        "            background: rgba(255, 255, 255, 0.3);\n"
        "            border-radius: 3px;\n"
        "            margin-top: 15px;\n"
        "            overflow: hidden;\n"
        "        }\n"
        "        \n"
        "        .progress-fill {\n"
        "            height: 100%%;\n"
        "            background: white;\n"
        "            border-radius: 3px;\n"
        "            transition: width 1s linear;\n"
        "        }\n"
        "        \n"
        "        .control-button {\n"
        "            margin-top: 15px;\n"
        "            padding: 10px 20px;\n"
        "            background: rgba(255, 255, 255, 0.2);\n"
        "            border: 2px solid white;\n"
        "            border-radius: 25px;\n"
        "            color: white;\n"
        "            font-size: clamp(0.9rem, 2vw, 1rem);\n"
        "            font-weight: 600;\n"
        "            cursor: pointer;\n"
        "            transition: all 0.3s ease;\n"
        "        }\n"
        "        \n"
        "        .control-button:hover {\n"
        "            background: rgba(255, 255, 255, 0.3);\n"
        "            transform: translateY(-2px);\n"
        "        }\n"
        "        \n"
        "        @media (max-width: 480px) {\n"
        "            .container {\n"
        "                padding: 20px;\n"
        "                margin: 10px;\n"
        "            }\n"
        "            \n"
        "            body {\n"
        "                padding: 10px;\n"
        "            }\n"
        "        }\n"
        "        \n"
        "        @media (max-width: 320px) {\n"
        "            .container {\n"
        "                padding: 15px;\n"
        "            }\n"
        "        }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class=\"container\">\n"
        "        <h1>Hi, I am the owner of the blogger</h1>\n"
        "        <p>I recommend my friend's channel.<br>\n"
        "        She works as a Cantonese teacher for more than 40 years.<br>\n"
        "        Her students mostly speak English.<br>\n"
        "        I recommend it.</p>\n"
        "        \n"
        "        <p>嗨，我係呢個博客嘅主人<br>\n"
        "        我推薦我朋友嘅頻道。<br>\n"
        "        佢做咗粵語老師超過40年。<br>\n"
        "        佢嘅學生大部分都係講英文嘅。<br>\n"
        "        我推薦佢。</p>\n"
        "        \n"
        "        <div class=\"countdown-container\">\n"
        "            <div class=\"countdown-text\">Redirecting to YouTube in:</div>\n"
        "            <div class=\"countdown-number\" id=\"countdown\">7</div>\n"
        "            <div class=\"progress-bar\">\n"
        "                <div class=\"progress-fill\" id=\"progress\"></div>\n"
        "            </div>\n"
        "            <button class=\"control-button\" id=\"controlBtn\" onclick=\"toggleTimer()\">Stop</button>\n"
        "        </div>\n"
        "    </div>\n"
        "\n"
        "    <script>\n"
        "        let countdown = 7;\n"
        "        let timer;\n"
        "        let isRunning = true;\n"
        "        const countdownElement = document.getElementById('countdown');\n"
        "        const progressElement = document.getElementById('progress');\n"
        "        const controlBtn = document.getElementById('controlBtn');\n"
        "        \n"
        "        progressElement.style.width = '100%%';\n"
        "        \n"
        "        function startTimer() {\n"
        "            timer = setInterval(function() {\n"
        "                countdown--;\n"
        "                countdownElement.textContent = countdown;\n"
        "                \n"
        "                const progressPercent = (countdown / 7) * 100;\n"
        "                progressElement.style.width = progressPercent + '%%';\n"
        "                \n"
        "                if (countdown <= 0) {\n"
        "                    clearInterval(timer);\n"
        "                    countdownElement.textContent = 'Redirecting...';\n"
        "                    window.location.href = '%s';\n"
        "                }\n"
        "            }, 1000);\n"
        "        }\n"
        "        \n"
        "        function toggleTimer() {\n"
        "            if (isRunning) {\n"
        "                clearInterval(timer);\n"
        "                controlBtn.textContent = 'Start';\n"
        "                isRunning = false;\n"
        "            } else {\n"
        "                startTimer();\n"
        "                controlBtn.textContent = 'Stop';\n"
        "                isRunning = true;\n"
        "            }\n"
        "        }\n"
        "        \n"
        "        startTimer();\n"
        "    </script>\n"
        "</body></html>";

    char html_content[8192];
    snprintf(html_content, sizeof(html_content), html_template, redirect_url);

    char response[8192];
    snprintf(response, sizeof(response),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/html; charset=UTF-8\r\n"
            "Content-Length: %ld\r\n"
            "Cache-Control: no-store, no-cache, must-revalidate, private\r\n"
            "Pragma: no-cache\r\n"
            "Expires: 0\r\n"
            "\r\n"
            "%s", strlen(html_content), html_content);

    send(client_fd, response, strlen(response), 0);
} // send_html_response

// Case-insensitive string search function
const char* strcasestr_custom(const char *haystack, const char *needle) {
    if (!haystack || !needle) return NULL;

    size_t needle_len = strlen(needle);
    if (needle_len == 0) return haystack;

    for (const char *p = haystack; *p; p++) {
        if (strncasecmp(p, needle, needle_len) == 0) {
            return p;
        }
    }
    return NULL;
} // strcasestr_custom

void extract_host_and_path(const char *request, char *host, char *path) {
    // Clear output buffers
    host[0] = '\0';
    path[0] = '\0';

    // Extract host (case-insensitive search for "Host:")
    const char *host_start = strcasestr_custom(request, "Host:");
    if (host_start) {
        host_start += 5; // Skip "Host:"

        // Skip any whitespace after "Host:"
        while (*host_start == ' ' || *host_start == '\t') {
            host_start++;
        }

        const char *host_end = strstr(host_start, "\r\n");
        if (!host_end) host_end = strstr(host_start, "\n");
        if (host_end) {
            int host_len = host_end - host_start;
            strncpy(host, host_start, host_len);
            host[host_len] = '\0';
        }
    }

    // Extract path (case-insensitive check for GET method)
    if (strncasecmp(request, "GET ", 4) == 0) {
        const char *path_start = request + 4;
        const char *path_end = strchr(path_start, ' ');
        if (path_end) {
            int path_len = path_end - path_start;
            strncpy(path, path_start, path_len);
            path[path_len] = '\0';
        }
    }
} // extract_host_and_path

void generate_aes_random(unsigned char *output, int output_len) {
    AES_KEY aes_key;
    AES_set_encrypt_key(aes_seed, 256, &aes_key);

    // Add system random and increment counter for each call
    counter++;
    uint32_t sys_rand = random();

    // Mix counter with system random
    uint64_t mixed_counter = counter ^ ((uint64_t)sys_rand << 32) ^ sys_rand;
    memcpy(&aes_counter[8], &mixed_counter, 8);

    // Generate blocks of random data
    for (int i = 0; i < output_len; i += 16) {
        unsigned char block[16];
        AES_encrypt(aes_counter, block, &aes_key);

        int copy_len = (output_len - i < 16) ? output_len - i : 16;
        memcpy(&output[i], block, copy_len);

        // Increment counter for next block
        counter++;
        memcpy(&aes_counter[8], &counter, 8);
    }

} // generate_aes_random


void generate_random_url(char *url_buf, int *video_index) {
    /*
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
    */
    unsigned char random_bytes[32];
    generate_aes_random(random_bytes, sizeof(random_bytes));

    // Use different parts of random data for different purposes
    // Bytes 0-7: video index (use 64-bit for better mixing, then mod)
    uint64_t video_big = *((uint64_t*)&random_bytes[0]);
    if (video_list.count == 0) {
        printf("ERROR: video_list.count is 0! This will cause division by zero!\n"); fflush(stdout);
        return;
    }
    uint32_t video_idx = (uint32_t)(video_big % video_list.count);

    // Bytes 8-11: URL length parameters
    uint32_t url_params = *((uint32_t*)&random_bytes[8]);
    int url_len = (url_params & 0x07) + 8;


    for(int i = 0; i < url_len; i++) {
        //sprintf(&url_buf[i*2], "%02x", new_md5[i]);
        sprintf(&url_buf[i*2], "%02x", random_bytes[12 + i]);
    }
    url_buf[url_len*2] = '\0';
    *video_index = video_idx;

} // generate_random_url


void handle_html_request(int client_fd) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer)-1, 0);
    if(bytes_read <= 0) return;

    buffer[bytes_read] = '\0';

    char host[256] = {0};
    char path[256] = {0};

    if(strncmp(buffer, "GET ", 4) != 0) return;

    extract_host_and_path(buffer, host, path);
    //printf( "=== buffer %ld [%s] === host %ld [%s] === path %ld [%s] \n", strlen(buffer), buffer, strlen(host), host, strlen(path), path ); // debug


    request_count++;
    //if(request_count % RELOAD_INTERVAL == 0) {
    //    load_video_list();
    //}
    check_and_reload_config_if_more_than_60s();

    char random_url[64];
    int video_index;
    generate_random_url(random_url, &video_index);

    int path_len = strlen(path);
    char redirect_url[512];
    if(path_len > 0 && path[path_len-1] == '/') {
        // Path ends with /, redirect to same path + random
        if(strcmp(path, "/") == 0) {
            snprintf(redirect_url, sizeof(redirect_url), "https://%s/%s", host, random_url);
        } else {
            snprintf(redirect_url, sizeof(redirect_url), "https://%s%s%s", host, path, random_url);
        }
    } else {
        // Path doesn't end with /, redirect to YouTube
        //send_html_response(client_fd, video_list.lines[video_index]);
        //snprintf(redirect_url, sizeof(redirect_url), "%s%s", video_list.lines[video_index], "&autoplay=0" );
        snprintf(redirect_url, sizeof(redirect_url), "%s", video_list.lines[video_index]);
    }
    send_html_response(client_fd, redirect_url);
} // handle_html_request
