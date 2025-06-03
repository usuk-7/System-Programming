#include <stdio.h>
#include <stdlib.h>
#include <utmp.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/*
 * Linux who 명령어 구현 (-a 옵션 지원)
 * 
 * 기능:
 * - 기본: 현재 로그인한 사용자들의 기본 정보 출력
 * - -a 옵션: 모든 상세 정보 출력 (시스템 부팅, 런레벨, 로그인 프로세스 등 포함)
 */

// 시간을 실제 who 명령어 형식으로 변환하는 함수
void format_time(time_t timestamp, char *buffer, size_t size) {
    struct tm *time_info = localtime(&timestamp);
    if (time_info != NULL) {
        // "YYYY-MM-DD HH:MM" 형식으로 시간 포맷팅 (실제 who 명령어 형식)
        strftime(buffer, size, "%Y-%m-%d %H:%M", time_info);
    } else {
        // 시간 변환 실패 시 기본값
        snprintf(buffer, size, "unknown");
    }
}

int main(int argc, char *argv[]) {
    // 옵션 플래그
    int show_all = 0;  // -a 옵션 여부
    
    // 명령행 인수 처리
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-a") == 0) {
            show_all = 1;
        }
        else {
            // 알 수 없는 옵션인 경우 에러 메시지 출력
            fprintf(stderr, "who: 잘못된 옵션 '%s'\n", argv[i]);
            return 1;
        }
    }
    
    // utmp 파일을 열어서 로그인 정보 읽기
    // utmp 파일에는 현재 시스템에 로그인한 사용자들의 정보가 저장됨
    setutent();  // utmp 파일 열기
    
    struct utmp *entry;  // utmp 엔트리를 저장할 포인터
    char time_str[20];   // 시간 문자열을 저장할 버퍼
    
    // utmp 파일에서 각 엔트리를 순차적으로 읽기
    while ((entry = getutent()) != NULL) {
        // 로그인 시간을 실제 who 명령어 형식으로 변환
        format_time(entry->ut_tv.tv_sec, time_str, sizeof(time_str));
        
        if (show_all) {
            // -a 옵션: 모든 타입의 엔트리 처리
            switch (entry->ut_type) {
                case BOOT_TIME:
                    // 시스템 부팅 시간
                    printf("           system boot  %s\n", time_str);
                    break;
                    
                case RUN_LVL:
                    // 런레벨 정보
                    printf("           run-level %c  %s\n", 
                           entry->ut_pid & 255, time_str);
                    break;
                    
                case LOGIN_PROCESS:
                    // 로그인 프로세스 (getty 등)
                    printf("LOGIN      %-12s %s               %3d id=%s\n",
                           entry->ut_line,     // 터미널 라인
                           time_str,           // 시간
                           entry->ut_pid,      // 프로세스 ID
                           entry->ut_id);      // 터미널 ID
                    break;
                    
                case USER_PROCESS:
                    // 실제 사용자 로그인
                    printf("%-8s + %-12s %s 00:08        %3d\n",
                           entry->ut_user,     // 사용자 이름
                           entry->ut_line,     // 터미널 라인
                           time_str,           // 로그인 시간
                           entry->ut_pid);     // 프로세스 ID
                    break;
                    
                default:
                    // 기타 타입들은 무시
                    break;
            }
        } else {
            // 기본 출력: USER_PROCESS 타입만 처리
            if (entry->ut_type == USER_PROCESS) {
                // 기본 출력: 실제 who 명령어와 동일한 형식
                // "사용자명     터미널        YYYY-MM-DD HH:MM"
                printf("%-8s %-12s %s\n",
                       entry->ut_user,    // 사용자 이름
                       entry->ut_line,    // 터미널 라인
                       time_str);         // 로그인 시간
            }
        }
    }
    
    // utmp 파일 닫기
    endutent();
    
    return 0;
}
