#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

double parse_time_value(const char* str) {
    char* endptr;
    double value = strtod(str, &endptr);
    
    if (value < 0) {
        fprintf(stderr, "sleep: 음수 시간은 허용되지 않습니다: %s\n", str);
        return -1;
    }
    
    if (endptr == str) {
        fprintf(stderr, "sleep: 잘못된 시간 형식: %s\n", str);
        return -1;
    }
    
    char unit = *endptr;
    if (unit == '\0' || unit == 's') {
        return value;
    } else if (unit == 'm') {
        return value * 60;
    } else if (unit == 'h') {
        return value * 3600;
    } else if (unit == 'd') {
        return value * 86400;
    } else {
        fprintf(stderr, "sleep: 알 수 없는 시간 단위: %c\n", unit);
        return -1;
    }
}

int sleep_seconds(double seconds) {
    if (seconds <= 0) {
        return 0;
    }
    
    struct timespec req, rem;
    req.tv_sec = (time_t)seconds;
    req.tv_nsec = (long)((seconds - req.tv_sec) * 1000000000);
    
    while (nanosleep(&req, &rem) == -1) {
        if (errno == EINTR) {
            req = rem;
        } else {
            perror("sleep");
            return 1;
        }
    }
    
    return 0;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "sleep: 시간이 필요합니다\n");
        fprintf(stderr, "사용법: sleep 시간[단위] ...\n");
        fprintf(stderr, "단위: s(초), m(분), h(시), d(일)\n");
        return 1;
    }
    
    double total_seconds = 0;
    
    for (int i = 1; i < argc; i++) {
        double seconds = parse_time_value(argv[i]);
        if (seconds < 0) {
            return 1;
        }
        total_seconds += seconds;
    }
    
    return sleep_seconds(total_seconds);
} 