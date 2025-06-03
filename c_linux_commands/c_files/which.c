#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

int main(int argc, char* argv[]) {
    if (argc < 2) {
        return 1;
    }
    
    char* path_env = getenv("PATH");
    if (!path_env) {
        return 1;
    }
    
    char* path_copy = strdup(path_env);
    char* dir = strtok(path_copy, ":");
    char full_path[1024];
    
    while (dir != NULL) {
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, argv[1]);
        
        if (access(full_path, X_OK) == 0) {
            printf("%s\n", full_path);
            free(path_copy);
            return 0;
        }
        
        dir = strtok(NULL, ":");
    }
    
    free(path_copy);
    return 1;
} 