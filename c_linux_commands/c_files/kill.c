#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

int main(int argc, char* argv[]) {
    pid_t pid;
    int i;
    
    if (argc < 2) {
        fprintf(stderr, "kill: PID가 필요합니다\n");
        return 1;
    }
    
    for (i = 1; i < argc; i++) {
        pid = atoi(argv[i]);
        
        if (pid <= 0) {
            fprintf(stderr, "kill: %s: 잘못된 PID\n", argv[i]);
            continue;
        }
        
        if (kill(pid, SIGTERM) == -1) {
            fprintf(stderr, "kill: %d: %s\n", pid, strerror(errno));
        }
    }
    
    return 0;
} 