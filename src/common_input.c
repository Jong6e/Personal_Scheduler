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

// get_line_input과 get_validated_input을 통합한 범용 입력 함수
static bool get_input_core(char *buffer, int buffer_size, const char *prompt, bool is_password, bool alphanumeric_only)
{
    printf("%s: ", prompt);
    int i = 0;
    int line_len = 0;
    line_len += strlen(prompt) + 2;
    buffer[0] = '\0';

    while (true)
    {
        int ch = _getch();

        // 방향키 등 특수키 입력(0xE0으로 시작) 무시
        if (ch == 0xE0)
        {
            _getch(); // 뒤따라오는 실제 키 코드 버림
            continue;
        }

        if (ch == KEY_ESC)
        {
            printf("\n[입력 취소]\n");
            Sleep(500);
            return false;
        }
        else if (ch == KEY_ENTER)
        {
            if (i > 0)
            {
                buffer[i] = '\0';
                printf("\n");
                break;
            }
        }
        else if (ch == KEY_BACKSPACE)
        {
            if (i > 0)
            {
                // 1. 버퍼에서 마지막 UTF-8 문자 삭제
                int last_char_start_pos = i - 1;
                while (last_char_start_pos > 0 && (buffer[last_char_start_pos] & 0xC0) == 0x80)
                {
                    last_char_start_pos--;
                }
                i = last_char_start_pos;
                buffer[i] = '\0';

                // 2. 화면을 다시 그림
                printf("\r"); // 줄의 시작으로 이동
                for (int k = 0; k < line_len; k++)
                    printf(" "); // 이전 줄 내용 지우기
                printf("\r");    // 다시 줄의 시작으로 이동

                // 3. 프롬프트와 수정된 버퍼 내용 출력
                printf("%s: %s", prompt, buffer);
                line_len = strlen(prompt) + 2 + strlen(buffer);
            }
        }
        else if (i < buffer_size - 1)
        {
            if (alphanumeric_only && !isalnum((unsigned char)ch))
            {
                continue;
            }

            buffer[i++] = (char)ch;
            buffer[i] = '\0';

            if (is_password)
            {
                printf("*");
            }
            else
            {
                printf("%c", ch);
            }
            line_len++;
        }
    }
    return true;
}

// ID/PW처럼 영문/숫자만 입력받는 함수
bool get_validated_input(char *buffer, int buffer_size, const char *prompt, bool is_password, bool alphanumeric_only)
{
    return get_input_core(buffer, buffer_size, prompt, is_password, alphanumeric_only);
}

// 메모처럼 모든 문자를 입력받는 함수
bool get_line_input(char *buffer, int buffer_size, const char *prompt)
{
    // is_password=false, alphanumeric_only=false
    return get_input_core(buffer, buffer_size, prompt, false, false);
}
