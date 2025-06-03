#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char** lines;
    int capacity;
    int start;
    int count;
} CircularBuffer;

CircularBuffer* create_buffer(int capacity) {
    CircularBuffer* buf = malloc(sizeof(CircularBuffer));
    buf->lines = malloc(sizeof(char*) * capacity);
    buf->capacity = capacity;
    buf->start = 0;
    buf->count = 0;
    return buf;
}

void add_line(CircularBuffer* buf, const char* line) {
    int pos = (buf->start + buf->count) % buf->capacity;
    
    if (buf->count == buf->capacity) {
        free(buf->lines[buf->start]);
        buf->start = (buf->start + 1) % buf->capacity;
    } else {
        buf->count++;
    }
    
    buf->lines[pos] = strdup(line);
}

void print_buffer(CircularBuffer* buf) {
    for (int i = 0; i < buf->count; i++) {
        int pos = (buf->start + i) % buf->capacity;
        printf("%s", buf->lines[pos]);
    }
}

void free_buffer(CircularBuffer* buf) {
    for (int i = 0; i < buf->count; i++) {
        int pos = (buf->start + i) % buf->capacity;
        free(buf->lines[pos]);
    }
    free(buf->lines);
    free(buf);
}

int tail_file(const char* filename, int num_lines) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("tail");
        return 1;
    }
    
    CircularBuffer* buf = create_buffer(num_lines);
    char* line = NULL;
    size_t len = 0;
    
    while (getline(&line, &len, file) != -1) {
        add_line(buf, line);
    }
    
    print_buffer(buf);
    
    free(line);
    free_buffer(buf);
    fclose(file);
    return 0;
}

int tail_stdin(int num_lines) {
    CircularBuffer* buf = create_buffer(num_lines);
    char* line = NULL;
    size_t len = 0;
    
    while (getline(&line, &len, stdin) != -1) {
        add_line(buf, line);
    }
    
    print_buffer(buf);
    
    free(line);
    free_buffer(buf);
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
        return tail_stdin(num_lines);
    }
    
    for (int i = start_idx; i < argc; i++) {
        if (argc > start_idx + 1) {
            printf("==> %s <==\n", argv[i]);
        }
        
        if (tail_file(argv[i], num_lines) != 0) {
            return 1;
        }
        
        if (i < argc - 1 && argc > start_idx + 1) {
            printf("\n");
        }
    }
    
    return 0;
} 