#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

typedef struct {
    long lines;
    long words;
    long chars;
} WcCount;

typedef struct {
    int show_lines;
    int show_words;
    int show_chars;
} WcOptions;

int count_words_in_line(const char* line) {
    int word_count = 0;
    int in_word = 0;
    
    for (int i = 0; line[i] != '\0'; i++) {
        if (isspace(line[i])) {
            in_word = 0;
        } else {
            if (!in_word) {
                word_count++;
                in_word = 1;
            }
        }
    }
    
    return word_count;
}

WcCount wc_file(const char* filename) {
    WcCount count = {0, 0, 0};
    FILE* file = fopen(filename, "r");
    
    if (!file) {
        perror("wc");
        return count;
    }
    
    char* line = NULL;
    size_t len = 0;
    ssize_t read;
    
    while ((read = getline(&line, &len, file)) != -1) {
        count.lines++;
        count.chars += read;
        count.words += count_words_in_line(line);
    }
    
    free(line);
    fclose(file);
    return count;
}

WcCount wc_stdin() {
    WcCount count = {0, 0, 0};
    char* line = NULL;
    size_t len = 0;
    ssize_t read;
    
    while ((read = getline(&line, &len, stdin)) != -1) {
        count.lines++;
        count.chars += read;
        count.words += count_words_in_line(line);
    }
    
    free(line);
    return count;
}

void print_count(const WcCount* count, const char* filename, const WcOptions* opts) {
    int default_mode = !opts->show_lines && !opts->show_words && !opts->show_chars;
    
    if (default_mode || opts->show_lines) {
        printf("%8ld", count->lines);
    }
    
    if (default_mode || opts->show_words) {
        printf("%8ld", count->words);
    }
    
    if (default_mode || opts->show_chars) {
        printf("%8ld", count->chars);
    }
    
    if (filename) {
        printf(" %s", filename);
    }
    
    printf("\n");
}

int main(int argc, char* argv[]) {
    WcOptions opts = {0, 0, 0};
    int start_idx = 1;
    
    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            for (int j = 1; argv[i][j]; j++) {
                switch (argv[i][j]) {
                    case 'l':
                        opts.show_lines = 1;
                        break;
                    case 'w':
                        opts.show_words = 1;
                        break;
                    case 'c':
                        opts.show_chars = 1;
                        break;
                    default:
                        fprintf(stderr, "wc: 잘못된 옵션 '-%c'\n", argv[i][j]);
                        return 1;
                }
            }
            start_idx = i + 1;
        } else {
            break;
        }
    }
    
    if (argc < start_idx + 1) {
        WcCount count = wc_stdin();
        print_count(&count, NULL, &opts);
        return 0;
    }
    
    WcCount total = {0, 0, 0};
    int file_count = argc - start_idx;
    
    for (int i = start_idx; i < argc; i++) {
        WcCount count = wc_file(argv[i]);
        print_count(&count, argv[i], &opts);
        
        total.lines += count.lines;
        total.words += count.words;
        total.chars += count.chars;
    }
    
    if (file_count > 1) {
        print_count(&total, "total", &opts);
    }
    
    return 0;
} 