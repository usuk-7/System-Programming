#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    char delimiter;
    int field_num;
} cut_options;

int process_cut(FILE* input, cut_options* opts) {
    char line[4096];
    
    while (fgets(line, sizeof(line), input) != NULL) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') {
            line[len-1] = '\0';
        }
        
        char* line_copy = malloc(strlen(line) + 1);
        strcpy(line_copy, line);
        
        char delim_str[2] = {opts->delimiter, '\0'};
        char* token = strtok(line_copy, delim_str);
        int current_field = 1;
        
        while (token && current_field < opts->field_num) {
            token = strtok(NULL, delim_str);
            current_field++;
        }
        
        if (token) {
            printf("%s\n", token);
        } else {
            printf("\n");
        }
        
        free(line_copy);
    }
    
    return 0;
}

int main(int argc, char* argv[]) {
    cut_options opts = {0};
    FILE* input = stdin;
    int i;
    char* filename = NULL;
    
    opts.delimiter = '\t';
    opts.field_num = 1;
    
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "cut: -d 옵션에는 구분자가 필요합니다\n");
                return 2;
            }
            opts.delimiter = argv[++i][0];
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "cut: 알 수 없는 옵션: %s\n", argv[i]);
            return 2;
        } else {
            filename = argv[i];
        }
    }
    
    if (filename) {
        input = fopen(filename, "r");
        if (!input) {
            fprintf(stderr, "cut: %s: 파일을 열 수 없습니다\n", filename);
            return 1;
        }
    }
    
    int result = process_cut(input, &opts);
    
    if (input != stdin) {
        fclose(input);
    }
    
    return result;
} 