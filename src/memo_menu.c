// src/memo_menu.c

#include "memo_menu.h"
#include "common_input.h"
#include "memo.h"      // For CONTENT_MAX
#include "user_menu.h" // For exit_program
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <ctype.h> // isdigit 함수 사용을 위해 추가

#define REQUEST_BUF_SIZE 2048
#define REPLY_BUF_SIZE 65536 // 64KB, 메모 목록이 길어질 수 있음

// 서버에 요청을 보내고 응답을 받는 내부 함수
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

// 로그인 후 메모 관리 메뉴 메인 루프
void memo_menu_loop(SOCKET sock, const char *logged_in_id)
{
    char option[10];
    char request[REQUEST_BUF_SIZE], reply[REPLY_BUF_SIZE];
    char memo_id_str[10], content[MEMO_CONTENT_MAX];

    while (true)
    {
        clear_screen();
        printf("┌────────────────────────────────────────────────────┐\n");
        printf("│                 개인 메모 관리 시스템              │\n");
        printf("└────────────────────────────────────────────────────┘\n\n");
        printf("====================== 메모 관리 =====================\n");
        printf("  1. 메모 목록 보기\n");
        printf("  2. 메모 상세보기\n");
        printf("  3. 새 메모 작성\n");
        printf("  4. 메모 수정\n");
        printf("  5. 메모 삭제\n");
        printf("  0. 뒤로가기\n");
        printf("======================================================\n");

        // 메모 목록을 항상 표시
        snprintf(request, sizeof(request), "MEMO_LIST:%s", logged_in_id);
        if (communicate_with_server(sock, request, reply))
        {
            printf("%s\n", reply);
        }
        printf("======================================================\n");

        if (!get_validated_input(option, sizeof(option), ">> 선택", false, false))
            continue;

        int choice = -1;
        if (strlen(option) == 1 && isdigit(option[0]))
            choice = option[0] - '0';

        switch (choice)
        {
        case 0:
            return;

        case 1: // 목록 보기는 이미 위에서 처리했으므로, 잠시 대기
            printf("\n목록을 새로고침하려면 아무 키나 누르세요...");
            getch();
            continue;

        case 2: // 메모 상세보기
            if (!get_validated_input(memo_id_str, sizeof(memo_id_str), "상세보기할 메모 ID", false, false))
                continue;
            snprintf(request, sizeof(request), "MEMO_VIEW:%s:%s", logged_in_id, memo_id_str);
            if (communicate_with_server(sock, request, reply))
            {
                printf("\n--- 메모 상세보기 ---\n%s\n", reply);
            }
            break;

        case 3: // 새 메모 작성
        {
            char title[MEMO_TITLE_MAX];
            get_line_input(title, sizeof(title), "제목 (미입력 시 자동생성)");
            get_line_input(content, sizeof(content), "새 메모 내용");

            snprintf(request, sizeof(request), "MEMO_ADD:%s:%s:%s", logged_in_id, title, content);
            communicate_with_server(sock, request, reply);
            printf("[클라이언트] %s\n", reply);
            break;
        }

        case 4: // 메모 수정
        {
            if (!get_validated_input(memo_id_str, sizeof(memo_id_str), "수정할 메모 ID", false, false))
                continue;

            // 1. 기존 내용을 서버에서 가져온다.
            snprintf(request, sizeof(request), "MEMO_GET_CONTENT:%s:%s", logged_in_id, memo_id_str);
            if (communicate_with_server(sock, request, reply) && strncmp(reply, "OK:", 3) == 0)
            {
                // 2. 기존 내용을 보여준다.
                printf("\n---[ 기존 내용 ]---\n%s\n--------------------\n", reply + 3);

                // 3. 새 내용을 입력받는다.
                get_line_input(content, sizeof(content), "새 내용");

                // 4. 서버에 업데이트를 요청한다.
                snprintf(request, sizeof(request), "MEMO_UPDATE:%s:%s:%s", logged_in_id, memo_id_str, content);
                communicate_with_server(sock, request, reply);
                printf("[클라이언트] %s\n", reply);
            }
            else
            {
                // MEMO_GET_CONTENT 실패 처리
                printf("[클라이언트] %s\n", reply);
            }
            break;
        }

        case 5: // 메모 삭제
            if (!get_validated_input(memo_id_str, sizeof(memo_id_str), "삭제할 메모 ID", false, false))
                continue;
            snprintf(request, sizeof(request), "MEMO_DELETE:%s:%s", logged_in_id, memo_id_str);
            communicate_with_server(sock, request, reply);
            printf("[클라이언트] %s\n", reply);
            break;

        default:
            printf("[클라이언트] 잘못된 선택입니다. (0-5)\n");
            break;
        }
        printf("\n계속하려면 아무 키나 누르세요...");
        getch();
    }
}
