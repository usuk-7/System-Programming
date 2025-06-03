#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>

struct termios orig_termios;

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);
    
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int get_terminal_size() {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == -1) {
        return 24;
    }
    return w.ws_row;
}

void show_more_prompt(int percentage) {
    printf("\033[7m--More--");
    if (percentage >= 0) {
        printf("(%d%%)", percentage);
    }
    printf("\033[m");
    fflush(stdout);
}

void clear_line() {
    printf("\r\033[K");
}

int more_file(const char* filename, int lines_per_page) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("more");
        return 1;
    }
    
    char* line = NULL;
    size_t len = 0;
    int line_count = 0;
    int total_lines = 0;
    int displayed_lines = 0;
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    while (getline(&line, &len, file) != -1) {
        total_lines++;
    }
    fseek(file, 0, SEEK_SET);
    
    enable_raw_mode();
    
    while (getline(&line, &len, file) != -1) {
        printf("%s", line);
        displayed_lines++;
        line_count++;
        
        if (line_count >= lines_per_page) {
            int percentage = (displayed_lines * 100) / total_lines;
            show_more_prompt(percentage);
            
            char c = getchar();
            clear_line();
            
            if (c == 'q' || c == 'Q') {
                break;
            } else if (c == ' ') {
                line_count = 0;
            } else if (c == '\n' || c == '\r') {
                line_count = lines_per_page - 1;
            }
        }
    }
    
    disable_raw_mode();
    free(line);
    fclose(file);
    return 0;
}

int main(int argc, char* argv[]) {
    int lines_per_page = get_terminal_size() - 1;
    int start_idx = 1;
    
    if (argc > 2 && strcmp(argv[1], "-n") == 0) {
        lines_per_page = atoi(argv[2]);
        if (lines_per_page <= 0) {
            lines_per_page = get_terminal_size() - 1;
        }
        start_idx = 3;
    }
    
    if (argc < start_idx + 1) {
        char* line = NULL;
        size_t len = 0;
        int line_count = 0;
        
        enable_raw_mode();
        
        while (getline(&line, &len, stdin) != -1) {
            printf("%s", line);
            line_count++;
            
            if (line_count >= lines_per_page) {
                show_more_prompt(-1);
                
                char c = getchar();
                clear_line();
                
                if (c == 'q' || c == 'Q') {
                    break;
                } else if (c == ' ') {
                    line_count = 0;
                } else if (c == '\n' || c == '\r') {
                    line_count = lines_per_page - 1;
                }
            }
        }
        
        disable_raw_mode();
        free(line);
        return 0;
    }
    
    for (int i = start_idx; i < argc; i++) {
        if (argc > start_idx + 1) {
            printf(":::::::::::::::\n%s\n:::::::::::::::\n", argv[i]);
        }
        
        if (more_file(argv[i], lines_per_page) != 0) {
            return 1;
        }
        
        if (i < argc - 1) {
            printf("\n");
        }
    }
    
    return 0;
} 