#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    int count_mode;
} uniq_options;

int process_uniq(FILE* input, uniq_options* opts) {
    char current_line[4096];
    char previous_line[4096];
    int count = 0;
    int first_line = 1;
    
    previous_line[0] = '\0';
    
    while (fgets(current_line, sizeof(current_line), input) != NULL) {
        size_t len = strlen(current_line);
        if (len > 0 && current_line[len-1] == '\n') {
            current_line[len-1] = '\0';
        }
        
        if (first_line || strcmp(current_line, previous_line) != 0) {
            if (!first_line) {
                if (opts->count_mode) {
                    printf("%7d %s\n", count, previous_line);
                } else {
                    printf("%s\n", previous_line);
                }
            }
            strcpy(previous_line, current_line);
            count = 1;
            first_line = 0;
        } else {
            count++;
        }
    }
    
    if (!first_line) {
        if (opts->count_mode) {
            printf("%7d %s\n", count, previous_line);
        } else {
            printf("%s\n", previous_line);
        }
    }
    
    return 0;
}

int main(int argc, char* argv[]) {
    uniq_options opts = {0};
    FILE* input = stdin;
    int i;
    char* filename = NULL;
    
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0) {
            opts.count_mode = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "uniq: 알 수 없는 옵션: %s\n", argv[i]);
            fprintf(stderr, "사용법: uniq [-c] [파일]\n");
            return 2;
        } else {
            filename = argv[i];
        }
    }
    
    if (filename) {
        input = fopen(filename, "r");
        if (!input) {
            fprintf(stderr, "uniq: %s: 파일을 열 수 없습니다\n", filename);
            return 1;
        }
    }
    
    int result = process_uniq(input, &opts);
    
    if (input != stdin) {
        fclose(input);
    }
    
    return result;
} 