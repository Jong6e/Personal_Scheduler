#include "common_input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <windows.h>
#include <ctype.h> // isalnum

#define KEY_ESC 27
#define KEY_ENTER 13
#define KEY_BACKSPACE 8

// 콘솔 화면을 깨끗하게 지웁니다.
void clear_screen()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// 사용자로부터 입력을 받되, ESC 키를 누르면 입력을 취소할 수 있습니다.
// masked 옵션은 비밀번호처럼 입력 내용을 '*'로 가릴지 결정합니다.
bool get_validated_input(char *buffer, int buffer_size, const char *prompt, bool is_password, bool alphanumeric_only)
{
    printf("%s: ", prompt);
    int i = 0;
    char ch;

    while (true)
    {
        ch = _getch();

        if (ch == 27) // ESC 키
        {
            printf("\n[입력 취소]\n");
            Sleep(500);
            return false;
        }
        else if (ch == '\r' || ch == '\n') // Enter 키
        {
            if (i > 0)
            {
                buffer[i] = '\0';
                printf("\n");
                break;
            }
        }
        else if (ch == '\b') // Backspace 키
        {
            if (i > 0)
            {
                i--;
                printf("\b \b");
            }
        }
        else if (i < buffer_size - 1)
        {
            if (alphanumeric_only && !isalnum((unsigned char)ch))
            {
                continue; // 영문/숫자가 아니면 무시
            }
            buffer[i++] = ch;
            if (is_password)
            {
                printf("*");
            }
            else
            {
                printf("%c", ch);
            }
        }
    }
    return true;
}

// get_line_input 함수를 _getch 기반으로 변경하여 ESC 취소 기능 추가
bool get_line_input(char *buffer, int buffer_size, const char *prompt)
{
    printf("%s: ", prompt);
    int i = 0;
    char ch;
    buffer[0] = '\0'; // 버퍼 초기화

    while (true)
    {
        ch = _getch();

        if (ch == 27) // ESC 키
        {
            printf("\n[입력 취소]\n");
            Sleep(500);
            return false;
        }
        else if (ch == '\r' || ch == '\n') // Enter 키
        {
            buffer[i] = '\0';
            printf("\n");
            break;
        }
        else if (ch == '\b') // Backspace 키
        {
            if (i > 0)
            {
                i--;
                printf("\b \b");
            }
        }
        else if (i < buffer_size - 1)
        {
            // 한글 등 멀티바이트 문자도 입력 가능하도록 별도 필터링 없음
            buffer[i++] = ch;
            printf("%c", ch);
        }
    }
    return true;
}
