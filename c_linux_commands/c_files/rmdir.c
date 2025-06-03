#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "rmdir: missing operand\n");
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        if (rmdir(argv[i]) != 0) {
            perror("rmdir");
            return 1;
        }
    }
    
    return 0;
} 