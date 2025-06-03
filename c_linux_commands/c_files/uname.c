#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include <string.h>

/*
 * Linux uname 명령어 구현 (-a 옵션만 지원)
 * 
 * 기능:
 * - 옵션 없이 실행: 시스템 이름만 출력
 * - -a 옵션: 모든 시스템 정보 출력 (시스템 이름, 노드명, 릴리즈, 버전, 하드웨어)
 */

// 도움말 메시지를 출력하는 함수
void print_help() {
    printf("사용법: uname [-a]\n");
    printf("시스템 정보를 출력합니다.\n\n");
    printf("옵션:\n");
    printf("  -a    모든 정보 출력\n");
    printf("  -h    이 도움말을 출력하고 종료\n");
}

int main(int argc, char *argv[]) {
    // utsname 구조체 선언
    // 이 구조체에는 시스템의 다양한 정보가 저장됨
    struct utsname system_info;
    
    // 옵션 플래그
    int show_all = 0;  // -a 옵션 여부
    
    // 명령행 인수 처리
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-a") == 0) {
            show_all = 1;
        }
        else if (strcmp(argv[i], "-h") == 0) {
            print_help();
            return 0;
        }
        else {
            // 알 수 없는 옵션인 경우 에러 메시지 출력
            fprintf(stderr, "uname: 잘못된 옵션 '%s'\n", argv[i]);
            fprintf(stderr, "'uname -h'를 실행하여 사용법을 확인하세요.\n");
            return 1;
        }
    }
    
    // uname() 시스템 콜을 사용하여 시스템 정보 가져오기
    if (uname(&system_info) == -1) {
        perror("uname() 함수 실행 실패");
        return 1;
    }
    
    if (show_all) {
        // -a 옵션: 모든 시스템 정보를 공백으로 구분하여 출력
        // sysname: 시스템 이름 (예: Linux)
        // nodename: 네트워크 노드명 (호스트네임)
        // release: 운영체제 릴리즈 (커널 버전)
        // version: 운영체제 버전 (빌드 정보)
        // machine: 하드웨어 아키텍처 (예: x86_64)
        printf("%s %s %s %s %s\n",
               system_info.sysname,   // 시스템 이름
               system_info.nodename,  // 노드명 (호스트네임)
               system_info.release,   // 릴리즈 버전
               system_info.version,   // 버전 정보
               system_info.machine);  // 하드웨어 플랫폼
    } else {
        // 기본 동작: 시스템 이름만 출력
        printf("%s\n", system_info.sysname);
    }
    
    return 0;
}
