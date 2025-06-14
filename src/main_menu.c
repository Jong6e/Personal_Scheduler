#include "main_menu.h"
#include "memo_menu.h"
#include "common_input.h"
#include "user.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <conio.h>
#include <windows.h> // For Sleep

#define REQUEST_BUF_SIZE 1024
#define REPLY_BUF_SIZE 1024

// 서버와 통신하는 헬퍼 함수
static bool communicate_with_server(SOCKET sock, const char *request, char *reply)
{
    if (send(sock, request, strlen(request), 0) < 0)
    {
        printf("[클라이언트] 서버에 요청 전송 실패\n");
        return false;
    }
    int bytes = recv(sock, reply, REPLY_BUF_SIZE - 1, 0);
    if (bytes <= 0)
    {
        printf("[클라이언트] 서버로부터 응답 수신 실패\n");
        return false;
    }
    reply[bytes] = '\0';
    return true;
}

// 로그인 후 메인 메뉴(사용자 메뉴) 루프
void main_menu_loop(SOCKET sock, const char *user_id)
{
    char option[10], request[REQUEST_BUF_SIZE], reply[REPLY_BUF_SIZE];

    while (true)
    {
        clear_screen();
        printf("┌────────────────────────────────────────────────────┐\n");
        printf("│                 개인 메모 관리 시스템              │\n");
        printf("└────────────────────────────────────────────────────┘\n\n");
        printf("===================== 메인 메뉴 ======================\n");
        printf("  1. 메모 관리\n");
        printf("  2. 비밀번호 변경\n");
        printf("  3. 회원 탈퇴\n");
        printf("  0. 로그아웃\n");
        printf("======================================================\n");

        if (!get_validated_input(option, sizeof(option), ">> 선택", false, false))
            continue;

        int choice = -1;
        if (strlen(option) == 1 && isdigit(option[0]))
            choice = option[0] - '0';

        switch (choice)
        {
        case 0: // 로그아웃
            printf("[클라이언트] 로그아웃합니다.\n");
            Sleep(500);
            return;

        case 1: // 메모 관리
            memo_menu_loop(sock, user_id);
            continue;

        case 2: // 비밀번호 변경
        {
            char old_pw[USER_PW_MAX], new_pw[USER_PW_MAX], new_pw2[USER_PW_MAX];
            clear_screen();
            printf("--- 비밀번호 변경 ---\n");
            if (!get_validated_input(old_pw, sizeof(old_pw), "기존 비밀번호", true, false))
                continue;
            if (!get_validated_input(new_pw, sizeof(new_pw), "새 비밀번호", true, false))
                continue;
            if (!get_validated_input(new_pw2, sizeof(new_pw2), "새 비밀번호 확인", true, false))
                continue;

            if (strcmp(new_pw, new_pw2) != 0)
            {
                printf("[클라이언트] 새 비밀번호가 일치하지 않습니다.\n");
            }
            else
            {
                snprintf(request, sizeof(request), "UPDATE_PW:%s:%s:%s", user_id, old_pw, new_pw);
                communicate_with_server(sock, request, reply);
                printf("[클라이언트] %s\n", strncmp(reply, "OK", 2) == 0 ? reply + 3 : reply + 5);
            }
        }
        break;

        case 3: // 회원 탈퇴
        {
            char pw[USER_PW_MAX], confirm[10];
            clear_screen();
            printf("--- 회원 탈퇴 ---\n[경고] 모든 데이터가 영구적으로 삭제됩니다.\n");
            if (!get_validated_input(confirm, sizeof(confirm), "정말 탈퇴하시겠습니까? (Y/N)", false, false) || (confirm[0] != 'Y' && confirm[0] != 'y'))
            {
                printf("[클라이언트] 회원 탈퇴가 취소되었습니다.\n");
            }
            else
            {
                if (!get_validated_input(pw, sizeof(pw), "비밀번호 확인", true, false))
                    continue;
                snprintf(request, sizeof(request), "DELETE_USER:%s:%s", user_id, pw);
                if (communicate_with_server(sock, request, reply) && strncmp(reply, "OK", 2) == 0)
                {
                    printf("[클라이언트] %s\n", reply + 3);
                    printf("계정 삭제가 완료되었습니다. 초기 메뉴로 돌아갑니다.\n");
                    Sleep(1000);
                    return; // 성공 시 루프 탈출하여 로그아웃
                }
                else
                {
                    printf("[클라이언트] %s\n", reply + 5);
                }
            }
        }
        break;

        default:
            printf("[클라이언트] 잘못된 선택입니다. (0-3)\n");
            break;
        }
        printf("\n계속하려면 아무 키나 누르세요...");
        getch();
    }
}
