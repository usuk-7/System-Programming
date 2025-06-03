#include <stdio.h>
#include <stdlib.h>

extern char **environ;

int main(int argc, char* argv[]) {
    char **env = environ;
    
    if (argc > 1) {
        fprintf(stderr, "env: 인수가 필요하지 않습니다\n");
        return 1;
    }
    
    while (*env != NULL) {
        printf("%s\n", *env);
        env++;
    }
    
    return 0;
} 