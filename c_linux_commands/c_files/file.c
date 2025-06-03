#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#define BUFFER_SIZE 512

typedef struct {
    unsigned char magic[16];
    int magic_len;
    char* description;
} MagicEntry;

MagicEntry magic_table[] = {
    {{0x7f, 0x45, 0x4c, 0x46}, 4, "ELF"},
    {{0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a}, 8, "PNG image data"},
    {{0xff, 0xd8, 0xff}, 3, "JPEG image data"},
    {{0x25, 0x50, 0x44, 0x46}, 4, "PDF document"},
    {{0x50, 0x4b, 0x03, 0x04}, 4, "Zip archive data"},
    {{0x50, 0x4b, 0x05, 0x06}, 4, "Zip archive data"},
    {{0x50, 0x4b, 0x07, 0x08}, 4, "Zip archive data"},
    {{0x1f, 0x8b}, 2, "gzip compressed data"},
    {{0x42, 0x5a, 0x68}, 3, "bzip2 compressed data"},
    {{0xfd, 0x37, 0x7a, 0x58, 0x5a, 0x00}, 6, "XZ compressed data"},
    {{0x52, 0x61, 0x72, 0x21, 0x1a, 0x07}, 6, "RAR archive data"},
    {{0x37, 0x7a, 0xbc, 0xaf, 0x27, 0x1c}, 6, "7-zip archive data"},
    {{0x49, 0x44, 0x33}, 3, "Audio file with ID3"},
    {{0xff, 0xfb}, 2, "MPEG audio"},
    {{0x4f, 0x67, 0x67, 0x53}, 4, "Ogg data"},
    {{0x52, 0x49, 0x46, 0x46}, 4, "RIFF"},
    {{0x00, 0x00, 0x01, 0x00}, 4, "MS Windows icon resource"},
    {{0x47, 0x49, 0x46, 0x38}, 4, "GIF image data"},
    {{0x42, 0x4d}, 2, "PC bitmap"},
    {{0x23, 0x21}, 2, "script text"}
};

int magic_table_size = sizeof(magic_table) / sizeof(MagicEntry);

int is_text_file(const unsigned char* buffer, int size) {
    int text_chars = 0;
    int total_chars = 0;
    
    for (int i = 0; i < size && i < BUFFER_SIZE; i++) {
        total_chars++;
        if (isprint(buffer[i]) || isspace(buffer[i])) {
            text_chars++;
        }
        
        if (buffer[i] == 0 && i < size - 1) {
            return 0;
        }
    }
    
    if (total_chars == 0) return 1;
    
    double ratio = (double)text_chars / total_chars;
    return ratio > 0.75;
}

char* check_magic_number(const unsigned char* buffer, int size) {
    for (int i = 0; i < magic_table_size; i++) {
        if (size >= magic_table[i].magic_len) {
            if (memcmp(buffer, magic_table[i].magic, magic_table[i].magic_len) == 0) {
                if (strcmp(magic_table[i].description, "RIFF") == 0 && size >= 12) {
                    if (memcmp(buffer + 8, "WAVE", 4) == 0) {
                        return "WAVE audio";
                    } else if (memcmp(buffer + 8, "AVI ", 4) == 0) {
                        return "AVI video";
                    }
                    return "RIFF data";
                }
                
                if (strcmp(magic_table[i].description, "script text") == 0 && size >= 3) {
                    if (buffer[2] == '/') {
                        return "script text executable";
                    }
                }
                
                return magic_table[i].description;
            }
        }
    }
    return NULL;
}

int is_executable(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return (st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH);
    }
    return 0;
}

void analyze_file(const char* path) {
    struct stat st;
    
    if (lstat(path, &st) != 0) {
        printf("%s: cannot open: No such file or directory\n", path);
        return;
    }
    
    printf("%s: ", path);
    
    if (S_ISLNK(st.st_mode)) {
        char target[1024];
        ssize_t len = readlink(path, target, sizeof(target) - 1);
        if (len != -1) {
            target[len] = '\0';
            printf("symbolic link to %s", target);
        } else {
            printf("broken symbolic link");
        }
        printf("\n");
        return;
    }
    
    if (S_ISDIR(st.st_mode)) {
        printf("directory\n");
        return;
    }
    
    if (S_ISCHR(st.st_mode)) {
        printf("character special\n");
        return;
    }
    
    if (S_ISBLK(st.st_mode)) {
        printf("block special\n");
        return;
    }
    
    if (S_ISFIFO(st.st_mode)) {
        printf("fifo (named pipe)\n");
        return;
    }
    
    if (S_ISSOCK(st.st_mode)) {
        printf("socket\n");
        return;
    }
    
    if (!S_ISREG(st.st_mode)) {
        printf("unknown file type\n");
        return;
    }
    
    if (st.st_size == 0) {
        printf("empty\n");
        return;
    }
    
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("cannot open for reading\n");
        return;
    }
    
    unsigned char buffer[BUFFER_SIZE];
    ssize_t bytes_read = read(fd, buffer, BUFFER_SIZE);
    close(fd);
    
    if (bytes_read < 0) {
        printf("cannot read file\n");
        return;
    }
    
    char* magic_result = check_magic_number(buffer, bytes_read);
    if (magic_result) {
        printf("%s", magic_result);
        
        if (strcmp(magic_result, "ELF") == 0) {
            if (bytes_read >= 16) {
                if (buffer[4] == 1) {
                    printf(" 32-bit");
                } else if (buffer[4] == 2) {
                    printf(" 64-bit");
                }
                
                if (buffer[5] == 1) {
                    printf(" LSB");
                } else if (buffer[5] == 2) {
                    printf(" MSB");
                }
                
                if (bytes_read >= 18) {
                    if (buffer[16] == 2) {
                        printf(" executable");
                    } else if (buffer[16] == 3) {
                        printf(" shared object");
                    } else if (buffer[16] == 1) {
                        printf(" relocatable");
                    }
                }
            }
        }
        
        if (is_executable(path) && strstr(magic_result, "script") == NULL) {
            printf(", executable");
        }
        
        printf("\n");
        return;
    }
    
    if (is_text_file(buffer, bytes_read)) {
        printf("ASCII text");
        
        int has_long_lines = 0;
        int line_length = 0;
        for (int i = 0; i < bytes_read; i++) {
            if (buffer[i] == '\n') {
                line_length = 0;
            } else {
                line_length++;
                if (line_length > 200) {
                    has_long_lines = 1;
                    break;
                }
            }
        }
        
        if (has_long_lines) {
            printf(", with very long lines");
        }
        
        if (is_executable(path)) {
            printf(", executable");
        }
        
        printf("\n");
    } else {
        printf("data");
        if (is_executable(path)) {
            printf(", executable");
        }
        printf("\n");
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "file: 파일명이 필요합니다\n");
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        analyze_file(argv[i]);
    }
    
    return 0;
} 