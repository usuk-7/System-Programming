#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <utime.h>
#include <fcntl.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "touch: missing file operand\n");
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        struct stat st;
        
        if (stat(argv[i], &st) == 0) {
            if (utime(argv[i], NULL) != 0) {
                perror("touch");
                return 1;
            }
        } else {
            int fd = open(argv[i], O_CREAT | O_WRONLY, 0644);
            if (fd == -1) {
                perror("touch");
                return 1;
            }
            close(fd);
        }
    }
    
    return 0;
} 