// src/user_menu.c

#include "user_menu.h"
#include "common_input.h"
#include "user.h"
#include "main_menu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <conio.h>
#include <ctype.h>

#define BUF_SIZE 2048

// 서버에 요청을 보내고 응답을 받아 출력하는 헬퍼 함수
static bool communicate_with_server(SOCKET sock, const char *request, char *reply)
{
    // 요청 전송
    if (send(sock, request, strlen(request), 0) < 0)
    {
        printf("[클라이언트] 서버에 요청 전송 실패\n");
        return false;
    }
    // 응답 수신
    int bytes = recv(sock, reply, BUF_SIZE - 1, 0);
    if (bytes <= 0)
    {
        printf("[클라이언트] 서버로부터 응답 수신 실패\n");
        return false;
    }
    reply[bytes] = '\0';
    return true;
}

// 프로그램 종료
void exit_program(SOCKET sock)
{
    char request[BUF_SIZE] = "EXIT";
    char reply[BUF_SIZE];

    printf("\n[클라이언트] 서버에 종료 요청을 보냅니다...\n");

    // 서버에 종료 요청 전송
    if (sock != INVALID_SOCKET)
    {
        if (send(sock, request, strlen(request), 0) < 0)
        {
            printf("[클라이언트] 서버에 종료 요청 전송 실패\n");
        }
        else
        {
            // 서버의 응답 대기
            int bytes = recv(sock, reply, BUF_SIZE - 1, 0);
            if (bytes > 0)
            {
                reply[bytes] = '\0';
                printf("[클라이언트] %s\n", reply + 3); // "OK:" 이후 메시지 출력
            }

            closesocket(sock);
        }
    }

    WSACleanup();
    printf("[클라이언트] 프로그램을 종료합니다.\n");
    Sleep(1000);
    exit(0);
}

// 로그인 이전 메뉴 (로그인, 회원가입, 종료)
void user_menu_loop(SOCKET sock)
{
    // 선택 변수
    char choice;
    // 아이디, 비밀번호, 비밀번호 확인 변수
    char id[MAX_ID_LEN], pw[MAX_PW_LEN], pw2[MAX_PW_LEN];
    // 요청, 응답 버퍼
    char request[BUF_SIZE], reply[BUF_SIZE];

    // 메뉴 루프
    while (true)
    {
        clear_screen();
        printf("┌────────────────────────────────────────────────────┐\n");
        printf("│                 개인 메모 관리 시스템              │\n");
        printf("└────────────────────────────────────────────────────┘\n\n");
        printf("======================== 메 뉴 =======================\n");
        printf("  1. 로그인\n");
        printf("  2. 회원가입\n");
        printf("  0. 종료\n");
        printf("======================================================\n");

        choice = get_single_choice_input(">> 선택", "120");

        // ESC 키 입력 시
        if (choice == KEY_ESC)
            continue;

        // 선택에 따라 분기
        switch (choice)
        {
        case '0': // 종료
            exit_program(sock);
            break;

        case '1': // 로그인
            // 아이디 입력
            if (!get_secure_input(id, sizeof(id), "  - 아이디", false, true))
                continue;
            // 비밀번호 입력
            if (!get_secure_input(pw, sizeof(pw), "  - 비밀번호", true, true))
                continue;

            // 로그인 요청 전송
            snprintf(request, sizeof(request), "LOGIN:%s:%s", id, pw);
            // 로그인 응답 처리
            if (communicate_with_server(sock, request, reply) && strncmp(reply, "OK", 2) == 0)
            {
                printf("[클라이언트] %s\n", reply + 3);
                Sleep(500);
                // 메인 메뉴 루프 진입
                main_menu_loop(sock, id);
            }
            else
            {
                printf("[클라이언트] %s\n", reply + 5);
            }
            break;

        case '2': // 회원가입
            // 아이디 입력
            printf("  * 아이디: 영문/숫자 조합으로 1~%d자 이내\n", MAX_ID_LEN);
            if (!get_secure_input(id, sizeof(id), "  - 아이디", false, true))
                continue;

            // 아이디 길이 검사
            if (strlen(id) < MIN_ID_LEN)
            {
                printf("[클라이언트] 아이디는 최소 %d자 이상이어야 합니다.\n", MIN_ID_LEN);
                break;
            }

            // 아이디 영문/숫자 조합 검사
            bool has_alpha = false, has_digit = false;
            for (int i = 0; id[i]; i++)
            {
                if (isalpha(id[i]))
                    has_alpha = true;
                if (isdigit(id[i]))
                    has_digit = true;
            }

            // 아이디 영문/숫자 조합 검사
            if (!has_alpha || !has_digit)
            {
                printf("[클라이언트] 아이디는 영문과 숫자를 모두 포함해야 합니다.\n");
                break;
            }

            // 비밀번호 입력
            printf("  * 비밀번호: 공백 없이 %d~%d자 이내\n", MIN_PW_LEN, MAX_PW_LEN);
            if (!get_secure_input(pw, sizeof(pw), "  - 비밀번호", true, true))
                continue;

            // 비밀번호 길이 검사
            if (strlen(pw) < MIN_PW_LEN)
            {
                printf("[클라이언트] 비밀번호는 최소 %d자 이상이어야 합니다.\n", MIN_PW_LEN);
                break;
            }

            // 비밀번호 확인 입력
            if (!get_secure_input(pw2, sizeof(pw2), "  - 비밀번호 확인", true, true))
                continue;

            // 비밀번호 확인 검사
            if (strcmp(pw, pw2) != 0)
            {
                printf("[클라이언트] 비밀번호가 일치하지 않습니다.\n");
                break;
            }

            // 회원가입 요청 전송
            snprintf(request, sizeof(request), "REGISTER:%s:%s", id, pw);
            // 회원가입 응답 처리
            communicate_with_server(sock, request, reply);
            // 회원가입 응답 처리
            if (strncmp(reply, "OK", 2) == 0)
            {
                printf("[클라이언트] %s\n", reply + 3);
            }
            else
            {
                printf("[클라이언트] %s\n", reply + 5);
            }
            break;

        default:
            // 예외처리
            printf("[클라이언트] 잘못된 선택입니다.\n");
            break;
        }
        printf("\n계속하려면 아무 키나 누르세요...");
        getch();
    }
}
