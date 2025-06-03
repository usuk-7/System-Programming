#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("whatis what?\n");
        return 1;
    }
    
    char command[256];
    snprintf(command, sizeof(command), "/usr/bin/whatis %s", argv[1]);
    
    int result = system(command);
    
    if (WEXITSTATUS(result) != 0) {
        printf("%s: nothing appropriate.\n", argv[1]);
        return 1;
    }
    
    return 0;
} 