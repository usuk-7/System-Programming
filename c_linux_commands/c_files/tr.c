#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    int delete_mode;
    char* set1;
    char* set2;
} tr_options;

int process_tr(FILE* input, tr_options* opts) {
    int ch;
    
    while ((ch = fgetc(input)) != EOF) {
        if (opts->delete_mode) {
            if (strchr(opts->set1, ch) == NULL) {
                putchar(ch);
            }
        } else {
            char* pos = strchr(opts->set1, ch);
            if (pos != NULL) {
                int index = pos - opts->set1;
                if (index < strlen(opts->set2)) {
                    putchar(opts->set2[index]);
                } else {
                    putchar(opts->set2[strlen(opts->set2) - 1]);
                }
            } else {
                putchar(ch);
            }
        }
    }
    
    return 0;
}

int main(int argc, char* argv[]) {
    tr_options opts = {0};
    FILE* input = stdin;
    int i;
    
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            opts.delete_mode = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "tr: 알 수 없는 옵션: %s\n", argv[i]);
            return 2;
        } else {
            if (!opts.set1) {
                opts.set1 = argv[i];
            } else if (!opts.set2 && !opts.delete_mode) {
                opts.set2 = argv[i];
            } else {
                fprintf(stderr, "tr: 너무 많은 인수\n");
                return 2;
            }
        }
    }
    
    if (!opts.set1) {
        fprintf(stderr, "tr: 문자셋이 필요합니다\n");
        return 2;
    }
    
    if (!opts.delete_mode && !opts.set2) {
        fprintf(stderr, "tr: 두 번째 문자셋이 필요합니다\n");
        return 2;
    }
    
    return process_tr(input, &opts);
} 