#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int head_file(const char* filename, int num_lines) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("head");
        return 1;
    }
    
    char* line = NULL;
    size_t len = 0;
    int count = 0;
    
    while (count < num_lines && getline(&line, &len, file) != -1) {
        printf("%s", line);
        count++;
    }
    
    free(line);
    fclose(file);
    return 0;
}

int head_stdin(int num_lines) {
    char* line = NULL;
    size_t len = 0;
    int count = 0;
    
    while (count < num_lines && getline(&line, &len, stdin) != -1) {
        printf("%s", line);
        count++;
    }
    
    free(line);
    return 0;
}

int main(int argc, char* argv[]) {
    int num_lines = 10;
    int start_idx = 1;
    
    if (argc > 2 && strcmp(argv[1], "-n") == 0) {
        num_lines = atoi(argv[2]);
        if (num_lines <= 0) {
            num_lines = 10;
        }
        start_idx = 3;
    }
    
    if (argc < start_idx + 1) {
        return head_stdin(num_lines);
    }
    
    for (int i = start_idx; i < argc; i++) {
        if (argc > start_idx + 1) {
            printf("==> %s <==\n", argv[i]);
        }
        
        if (head_file(argv[i], num_lines) != 0) {
            return 1;
        }
        
        if (i < argc - 1 && argc > start_idx + 1) {
            printf("\n");
        }
    }
    
    return 0;
} 