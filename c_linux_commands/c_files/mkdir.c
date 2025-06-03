#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <libgen.h>
#include <unistd.h>

int create_dir_recursive(const char* path) {
    char* path_copy = strdup(path);
    char* parent_dir = dirname(path_copy);
    
    if (strcmp(parent_dir, ".") != 0 && strcmp(parent_dir, "/") != 0) {
        if (access(parent_dir, F_OK) != 0) {
            if (create_dir_recursive(parent_dir) != 0) {
                free(path_copy);
                return -1;
            }
        }
    }
    
    free(path_copy);
    
    if (mkdir(path, 0755) != 0) {
        if (errno != EEXIST) {
            return -1;
        }
    }
    
    return 0;
}

int main(int argc, char* argv[]) {
    int use_parents = 0;
    int start_idx = 1;
    
    if (argc < 2) {
        fprintf(stderr, "mkdir: missing operand\n");
        return 1;
    }
    
    if (strcmp(argv[1], "-p") == 0) {
        use_parents = 1;
        start_idx = 2;
        
        if (argc < 3) {
            fprintf(stderr, "mkdir: missing operand\n");
            return 1;
        }
    }
    
    for (int i = start_idx; i < argc; i++) {
        if (use_parents) {
            if (create_dir_recursive(argv[i]) != 0) {
                perror("mkdir");
                return 1;
            }
        } else {
            if (mkdir(argv[i], 0755) != 0) {
                perror("mkdir");
                return 1;
            }
        }
    }
    
    return 0;
} 