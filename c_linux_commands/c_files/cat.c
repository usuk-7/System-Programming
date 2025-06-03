#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char* argv[]) {
    int show_line_numbers = 0;
    int start_idx = 1;
    
    if (argc > 1 && strcmp(argv[1], "-n") == 0) {
        show_line_numbers = 1;
        start_idx = 2;
    }
    
    if (argc < start_idx + 1) {
        char c;
        int line_number = 1;
        
        if (show_line_numbers) {
            printf("%6d\t", line_number);
        }
        
        while ((c = getchar()) != EOF) {
            putchar(c);
            if (c == '\n' && show_line_numbers) {
                line_number++;
                if ((c = getchar()) != EOF) {
                    printf("%6d\t", line_number);
                    putchar(c);
                }
            }
        }
        return 0;
    }
    
    for (int i = start_idx; i < argc; i++) {
        FILE* file = fopen(argv[i], "r");
        if (!file) {
            perror("cat");
            continue;
        }
        
        char c;
        int line_number = 1;
        int at_line_start = 1;
        
        while ((c = fgetc(file)) != EOF) {
            if (show_line_numbers && at_line_start) {
                printf("%6d\t", line_number);
                at_line_start = 0;
            }
            
            putchar(c);
            
            if (c == '\n') {
                line_number++;
                at_line_start = 1;
            }
        }
        
        fclose(file);
    }
    
    return 0;
} 