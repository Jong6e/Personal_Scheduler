#include "common_input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <windows.h>

#define KEY_ESC 27
#define KEY_ENTER 13
#define KEY_BACKSPACE 8

// ✅ 콘솔 화면을 초기화합니다.
void clear_screen()
{
    system("cls");
}

// ✅ 사용자 입력을 안전하게 받는 함수 (ESC 키로 취소 가능)
bool get_escapable_input(char *buffer, size_t size, const char *label, bool masked)
{
    int i = 0;
    char ch;

    printf("%s (ESC: 취소): ", label);

    while (true)
    {
        ch = getch();

        // Enter 키: 입력 완료
        if (ch == KEY_ENTER)
        {
            if (i == 0)
            { // 빈 입력 방지
                printf("\n[오류] 값을 입력해야 합니다. 다시 시도하세요.\n");
                printf("%s (ESC: 취소): ", label);
                continue;
            }
            buffer[i] = '\0';
            printf("\n");
            return true;
        }

        // ESC 키: 입력 취소
        if (ch == KEY_ESC)
        {
            buffer[0] = '\0';
            printf("\n[알림] 입력이 취소되었습니다.\n");
            Sleep(500); // 0.5초 대기
            return false;
        }

        // Backspace 키: 글자 삭제
        if (ch == KEY_BACKSPACE && i > 0)
        {
            printf("\b \b");
            i--;
            continue;
        }

        // 입력 가능한 문자인 경우
        if (ch >= 32 && ch <= 126)
        {
            if (i < size - 1)
            {
                buffer[i++] = ch;
                printf(masked ? "*" : "%c", ch);
            }
            else
            {
                // 버퍼가 가득 찼을 때 경고음
                printf("\a");
            }
        }
    }
}
