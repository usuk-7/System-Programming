#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main() {
    char cwd[1024];
    
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
        return 0;
    } else {
        perror("getcwd");
        return 1;
    }
} 