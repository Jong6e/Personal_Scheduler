// src/common_input.c

#include "common_input.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <windows.h>
#include <ctype.h> // isalnum

// 콘솔 화면을 깨끗하게 지우는 함수
void clear_screen()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// ID/PW, 일반 텍스트 등 ESC/백스페이스 처리가 필요한 모든 입력을 처리하는 핵심 함수 (비밀번호 입력 시 마스킹 처리)
bool get_secure_input(char *buffer, int buffer_size, const char *prompt, bool is_password, bool alphanumeric_only)
{
    printf("%s (ESC:취소):\n", prompt);
    // 입력 버퍼 초기화
    int i = 0;
    buffer[0] = '\0';
    // 입력 버퍼 크기 초기화
    while (i < buffer_size - 1)
    {
        // 키 입력 받기
        int ch = _getch();
        // ESC 키 입력 시
        if (ch == KEY_ESC)
        {
            printf("\n[입력 취소]\n");
            Sleep(500);
            return false;
        }
        // Enter 키 입력 시
        if (ch == KEY_ENTER)
        {
            // 입력된 내용이 없으면 Enter 키를 무시, 사용자의 빈 값 제출 방지
            if (i == 0)
                continue;
            printf("\n");
            break;
        }
        // 백스페이스 키 입력 시
        if (ch == KEY_BACKSPACE)
        {
            if (i > 0)
            {
                // UTF-8 문자를 글자 단위로 올바르게 지우기 위한 로직
                int bytes_to_erase = 1;
                char *ptr = &buffer[i - 1]; // 버퍼의 마지막 바이트를 가리킴

                // 마지막 바이트부터 거꾸로 탐색하여 문자의 시작점을 찾음
                // UTF-8 연속 바이트는 10xxxxxx 형식
                while (ptr > buffer && (*ptr & 0xC0) == 0x80)
                {
                    ptr--;
                    bytes_to_erase++;
                }

                // 버퍼에서 해당 글자의 바이트 수만큼 제거
                i -= bytes_to_erase;
                buffer[i] = '\0';

                // 콘솔 화면에서도 지움 (한글 등 멀티바이트 문자는 2칸 차지)
                if (bytes_to_erase > 1)
                {
                    printf("\b\b  \b\b");
                }
                else
                {
                    printf("\b \b");
                }
            }
        }
        // 그 외 키 입력 시
        else
        {
            // 특수 키는 무시
            if (ch == 0xE0 || ch == 0x00)
            {
                _getch();
                continue;
            }

            // 첫 글자로 공백 입력을 방지 (ID/PW가 아닌 일반 텍스트 입력 시)
            if (i == 0 && !alphanumeric_only && isspace((unsigned char)ch))
            {
                continue;
            }

            // Enter, ESC, Backspace를 제외한 모든 제어 문자를 무시
            // Shift+Enter 등으로 입력되는 비정상적인 문자를 차단
            if (iscntrl((unsigned char)ch))
            {
                continue;
            }

            // ID/PW 입력 모드일 때, isalnum()으로 영문/숫자만 허용하고,
            // 그 외의 경우는 isprint()가 false를 반환하는 한글 등 비-ASCII 문자 입력을 허용
            if (alphanumeric_only && !isalnum((unsigned char)ch))
            {
                continue;
            }

            buffer[i++] = (char)ch;
            printf(is_password ? "*" : "%c", ch);
        }
    }
    // 입력 버퍼 종료
    buffer[i] = '\0';
    return true;
}

// 메모의 제목/내용처럼 일반 텍스트 입력을 받는 함수 (ESC/백스페이스 지원)
bool get_line_input(char *buffer, int buffer_size, const char *prompt)
{
    // 내부적으로 get_secure_input을 호출하여 기능 재사용 및 일관성 확보 (비밀번호 입력 시 마스킹 처리)
    return get_secure_input(buffer, buffer_size, prompt, false, false);
}

// 단일 키 입력을 받아 유효성을 검사하는 메뉴 선택 전용 함수
char get_single_choice_input(const char *prompt, const char *valid_choices)
{
    printf("%s (ESC:취소) ", prompt);

    while (true)
    {
        // 키 입력 받기
        int ch = _getch();
        // ESC 키 입력 시
        if (ch == KEY_ESC)
        {
            printf("\n[입력 취소]\n");
            Sleep(500);
            return KEY_ESC;
        }
        // 유효한 선택 문자 입력 시
        if (valid_choices != NULL && strchr(valid_choices, ch) != NULL)
        {
            printf("%c\n", ch);
            return (char)ch;
        }
    }
}
