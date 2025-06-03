#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("What manual page do you want?\n");
        return 1;
    }
    
    char command[256];
    snprintf(command, sizeof(command), "/usr/bin/man %s", argv[1]);
    
    int result = system(command);
    
    if (WEXITSTATUS(result) != 0) {
        printf("No manual entry for %s\n", argv[1]);
        return 1;
    }
    
    return 0;
}
