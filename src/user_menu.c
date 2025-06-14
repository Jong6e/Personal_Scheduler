#include "user_menu.h"
#include "common_input.h"
#include "user.h" // USER_ID_MAX, USER_PW_MAX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h> // Sleep
#include <conio.h>   // getch 함수 사용을 위해 추가

#define BUF_SIZE 1024

// --- 내부 함수 ---

// 서버에 요청을 보내고 응답을 받아 출력하는 헬퍼 함수
static bool communicate_with_server(SOCKET sock, const char *request, char *reply)
{
    if (send(sock, request, strlen(request), 0) < 0)
    {
        printf("[클라이언트] 서버에 요청 전송 실패\n");
        return false;
    }

    int bytes = recv(sock, reply, BUF_SIZE - 1, 0);
    if (bytes <= 0)
    {
        printf("[클라이언트] 서버로부터 응답 수신 실패\n");
        return false;
    }
    reply[bytes] = '\0';
    return true;
}

// --- 외부 공개 함수 ---

// ✅ 프로그램의 안전한 종료를 처리
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

// ✅ 로그인 전 사용자 메뉴 메인 루프
void user_menu_loop(SOCKET sock)
{
    char option[10];
    char id[USER_ID_MAX], pw[USER_PW_MAX], pw2[USER_PW_MAX];
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

        if (!get_escapable_input(option, sizeof(option), ">> 선택", false))
            continue;

        int choice = atoi(option);

        switch (choice)
        {
        case 0: // 종료
            exit_program(sock);
            break;

        case 1: // 로그인
            if (!get_escapable_input(id, sizeof(id), "  - 아이디", false))
                continue;
            if (!get_escapable_input(pw, sizeof(pw), "  - 비밀번호", true))
                continue;

            snprintf(request, sizeof(request), "LOGIN:%s:%s", id, pw);
            if (communicate_with_server(sock, request, reply) && strncmp(reply, "OK", 2) == 0)
            {
                printf("[클라이언트] %s\n", reply + 3); // "OK:" 다음부터 출력
                // TODO: 로그인 성공 시 메모 관리 메뉴로 전환
                printf("메모 관리 기능은 아직 구현되지 않았습니다. 로그아웃합니다.\n");
            }
            else
            {
                printf("[클라이언트] %s\n", reply + 5); // "FAIL:" 다음부터 출력
            }
            break;

        case 2: // 회원가입
            if (!get_escapable_input(id, sizeof(id), "  - 아이디", false))
                continue;
            if (!get_escapable_input(pw, sizeof(pw), "  - 비밀번호", true))
                continue;
            if (!get_escapable_input(pw2, sizeof(pw2), "  - 비밀번호 확인", true))
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
            printf("[클라이언트] 잘못된 선택입니다. (0-2)\n");
            break;
        }
        printf("\n계속하려면 아무 키나 누르세요...");
        getch();
    }
}
