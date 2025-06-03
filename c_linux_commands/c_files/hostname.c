#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * Linux hostname 명령어 구현 (간단 버전)
 * 
 * 기능:
 * - 현재 시스템의 호스트네임을 출력
 */

// 호스트네임 최대 길이 정의 (POSIX 표준에 따라 255자 + null terminator)
#define HOSTNAME_MAX_LEN 256

int main() {
    // 호스트네임을 저장할 버퍼
    // 일반적으로 호스트네임은 255자를 넘지 않음
    char hostname[HOSTNAME_MAX_LEN];
    
    // gethostname() 함수를 사용하여 현재 시스템의 호스트네임 가져오기
    // 첫 번째 인수: 호스트네임을 저장할 버퍼
    // 두 번째 인수: 버퍼의 크기
    if (gethostname(hostname, sizeof(hostname)) == -1) {
        // gethostname() 함수가 실패한 경우 에러 메시지 출력
        perror("gethostname() 함수 실행 실패");
        return 1;
    }
    
    // 가져온 호스트네임을 표준 출력으로 출력
    printf("%s\n", hostname);
    
    return 0;
}
