#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
 * Linux date 명령어 구현 (간단 버전)
 * 
 * 기능:
 * - 현재 날짜와 시간을 기본 형식으로 출력
 */

int main() {
    // 현재 시간을 가져오기 위한 변수들
    time_t current_time;
    struct tm *tm_info;
    
    // 요일과 월 이름을 위한 배열
    const char *weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    
    // 현재 시간 가져오기
    current_time = time(NULL);
    if (current_time == -1) {
        perror("time() 함수 실행 실패");
        return 1;
    }
    
    // 로컬 시간으로 변환
    tm_info = localtime(&current_time);
    if (tm_info == NULL) {
        perror("시간 변환 실패");
        return 1;
    }
    
    // 기본 포맷으로 날짜 출력 (예: Mon Jan 15 14:30:25 KST 2024)
    printf("%s %s %2d %02d:%02d:%02d KST %d\n",
           weekdays[tm_info->tm_wday],      // 요일
           months[tm_info->tm_mon],         // 월
           tm_info->tm_mday,                // 일
           tm_info->tm_hour,                // 시
           tm_info->tm_min,                 // 분
           tm_info->tm_sec,                 // 초
           tm_info->tm_year + 1900);        // 년도 (1900을 더해야 실제 년도)
    
    return 0;
}
