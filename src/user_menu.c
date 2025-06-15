// src/user_menu.c

#include "user_menu.h"
#include "common_input.h"
#include "user.h"      // USER_ID_MAX, USER_PW_MAX
#include "main_menu.h" // 메모 메뉴 진입을 위해 추가
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h> // Sleep
#include <conio.h>   // getch 함수 사용을 위해 추가

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

// 프로그램을 종료합니다.
void exit_program(SOCKET sock)
{
    printf("\n[클라이언트] 프로그램을 종료합니다.\n");
    if (sock != INVALID_SOCKET)
    {
        closesocket(sock);
    }
    WSACleanup();
    Sleep(500);
    exit(0);
}

// 로그인 이전 사용자를 위한 메뉴입니다. (로그인, 회원가입, 종료)
void user_menu_loop(SOCKET sock)
{
    char choice;
    char id[MAX_ID_LEN], pw[MAX_PW_LEN], pw2[MAX_PW_LEN];
    char request[BUF_SIZE], reply[BUF_SIZE];

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

        if (choice == KEY_ESC)
            continue;

        switch (choice)
        {
        case '0': // 종료
            exit_program(sock);
            break;

        case '1': // 로그인
            if (!get_secure_input(id, sizeof(id), "  - 아이디", false, true))
                continue;
            if (!get_secure_input(pw, sizeof(pw), "  - 비밀번호", true, true))
                continue;

            snprintf(request, sizeof(request), "LOGIN:%s:%s", id, pw);
            if (communicate_with_server(sock, request, reply) && strncmp(reply, "OK", 2) == 0)
            {
                printf("[클라이언트] %s\n", reply + 3); // "OK:" 다음부터 출력
                Sleep(500);
                main_menu_loop(sock, id);
            }
            else
            {
                printf("[클라이언트] %s\n", reply + 5); // "FAIL:" 다음부터 출력
            }
            break;

        case '2': // 회원가입
            if (!get_secure_input(id, sizeof(id), "  - 아이디", false, true))
                continue;
            if (!get_secure_input(pw, sizeof(pw), "  - 비밀번호", true, true))
                continue;
            if (!get_secure_input(pw2, sizeof(pw2), "  - 비밀번호 확인", true, true))
                continue;

            if (strcmp(pw, pw2) != 0)
            {
                printf("[클라이언트] 비밀번호가 일치하지 않습니다.\n");
                break;
            }

            snprintf(request, sizeof(request), "REGISTER:%s:%s", id, pw);
            communicate_with_server(sock, request, reply);
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
            // get_single_choice_input에서 유효하지 않은 입력은 걸러주므로
            // 이 케이스는 사실상 발생하지 않습니다.
            printf("[클라이언트] 잘못된 선택입니다.\n");
            break;
        }
        printf("\n계속하려면 아무 키나 누르세요...");
        getch();
    }
}
