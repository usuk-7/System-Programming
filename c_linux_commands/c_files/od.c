#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    FILE* file;
    unsigned char buffer[16];
    size_t bytes_read;
    long offset = 0;
    int i;
    
    if (argc > 2) {
        fprintf(stderr, "od: 너무 많은 인수\n");
        return 1;
    }
    
    if (argc == 1) {
        file = stdin;
    } else {
        file = fopen(argv[1], "rb");
        if (!file) {
            fprintf(stderr, "od: %s: 파일을 열 수 없습니다\n", argv[1]);
            return 1;
        }
    }
    
    while ((bytes_read = fread(buffer, 1, 16, file)) > 0) {
        printf("%07lo", offset);
        
        for (i = 0; i < bytes_read; i += 2) {
            if (i + 1 < bytes_read) {
                printf(" %06o", (buffer[i + 1] << 8) | buffer[i]);
            } else {
                printf(" %06o", buffer[i]);
            }
        }
        
        printf("\n");
        offset += bytes_read;
    }
    
    printf("%07lo\n", offset);
    
    if (argc == 2) {
        fclose(file);
    }
    
    return 0;
} 