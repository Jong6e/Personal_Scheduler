// src/main_menu.c

#include "main_menu.h"
#include "memo_menu.h"
#include "common_input.h"
#include "user.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <conio.h>
#include <windows.h>
#include <time.h> // for time()

#define REQUEST_BUF_SIZE 2048
#define REPLY_BUF_SIZE 65536

// 함수 선언
static bool communicate_with_server(SOCKET sock, const char *request, char *reply);
static void handle_all_memos_download(SOCKET sock, const char *user_id);

// 서버와 통신하는 헬퍼 함수
static bool communicate_with_server(SOCKET sock, const char *request, char *reply)
{
    // 요청 전송
    if (send(sock, request, strlen(request), 0) < 0)
    {
        printf("[클라이언트] 서버에 요청 전송 실패\n");
        return false;
    }
    // 응답 수신
    int bytes = recv(sock, reply, REPLY_BUF_SIZE - 1, 0);
    if (bytes <= 0)
    {
        printf("[클라이언트] 서버로부터 응답 수신 실패\n");
        return false;
    }
    reply[bytes] = '\0';
    return true;
}

// 전체 메모 다운로드 과정을 처리하는 함수
static void handle_all_memos_download(SOCKET sock, const char *user_id)
{
    clear_screen();
    printf("--- 전체 메모 다운로드 ---\n\n");

    // 1. 포맷 선택
    printf("다운로드할 파일 형식을 선택하세요.\n");
    char choice = get_single_choice_input("1: Markdown (.md)\n2: 텍스트 (.txt)\n3: JSON (.json)\n4: XML (.xml)\n5: CSV (.csv)\n", "12345");
    if (choice == KEY_ESC)
        return;

    const char *format_str;
    const char *ext;
    if (choice == '1')
    {
        format_str = "MD";
        ext = "md";
    }
    else if (choice == '2')
    {
        format_str = "TXT";
        ext = "txt";
    }
    else if (choice == '3')
    {
        format_str = "JSON";
        ext = "json";
    }
    else if (choice == '4')
    {
        format_str = "XML";
        ext = "xml";
    }
    else
    { // choice == '5'
        format_str = "CSV";
        ext = "csv";
    }

    // 2. 파일명 생성
    char filepath[MAX_PATH];
    time_t t = time(NULL);
    struct tm now;
    localtime_s(&now, &t);
    snprintf(filepath, sizeof(filepath), "downloads/allmemo_%s_%d%02d%02d.%s", user_id, now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, ext);

    // 3. 파일 존재 여부 확인
    FILE *file_check;
    if (fopen_s(&file_check, filepath, "r") == 0)
    {
        fclose(file_check);
        printf("\n[경고] '%s' 파일이 이미 존재합니다.", filepath);
        char overwrite_choice = get_single_choice_input(" 덮어쓰시겠습니까? (Y/N)", "yYnN");
        if (overwrite_choice == 'n' || overwrite_choice == 'N' || overwrite_choice == KEY_ESC)
        {
            printf("\n[알림] 다운로드가 취소되었습니다.\n");
            Sleep(1000);
            return;
        }
    }

    // 4. 최종 확인
    printf("\n'downloads' 폴더에 '%s'(으)로 저장하시겠습니까?", filepath + strlen("downloads/"));
    choice = get_single_choice_input(" (Y/N)", "yYnN");
    if (choice == 'n' || choice == 'N' || choice == KEY_ESC)
    {
        printf("\n[알림] 다운로드가 취소되었습니다.\n");
        Sleep(1000);
        return;
    }

    // 5. 서버 요청
    char request[REQUEST_BUF_SIZE], reply[REPLY_BUF_SIZE];
    snprintf(request, sizeof(request), "DOWNLOAD_ALL:%s:%s", user_id, format_str);

    printf("\n다운로드 중...\n");
    if (!communicate_with_server(sock, request, reply) || strncmp(reply, "OK:", 3) != 0)
    {
        printf("\n[오류] 다운로드에 실패했습니다: %s\n", reply + 3);
        Sleep(1500);
        return;
    }

    // 6. 데이터 수신 및 파일 저장
    const char *data_to_save = reply + 3;
    FILE *file;
    if (fopen_s(&file, filepath, "w") != 0 || file == NULL)
    {
        printf("\n[오류] 파일을 생성할 수 없습니다.\n");
        Sleep(1000);
        return;
    }
    fprintf(file, "%s", data_to_save);
    fclose(file);

    // 7. 결과 안내
    printf("\n[성공] 다운로드가 완료되었습니다. (%s)\n", filepath);
    Sleep(1500);
}

// 로그인 후 메인 메뉴(사용자 메뉴) 루프
void main_menu_loop(SOCKET sock, const char *user_id)
{
    char choice;
    char request[REQUEST_BUF_SIZE], reply[REPLY_BUF_SIZE];

    while (true)
    {
        clear_screen();
        printf("┌────────────────────────────────────────────────────┐\n");
        printf("│                 개인 메모 관리 시스템              │\n");
        printf("└────────────────────────────────────────────────────┘\n\n");
        printf("===================== 메인 메뉴 ======================\n");
        printf("  1. 메모 관리\n");
        printf("  2. 전체 메모 다운로드\n");
        printf("  3. 비밀번호 변경\n");
        printf("  4. 회원 탈퇴\n");
        printf("  0. 로그아웃\n");
        printf("======================================================\n");

        choice = get_single_choice_input(">> 선택", "12340");
        if (choice == KEY_ESC)
            continue;

        switch (choice)
        {
        case '0': // 로그아웃
            printf("[클라이언트] 로그아웃합니다.\n");
            Sleep(500);
            return;

        case '1': // 메모 관리
            memo_menu_loop(sock, user_id);
            continue;

        case '2': // 전체 메모 다운로드
            handle_all_memos_download(sock, user_id);
            break;

        case '3': // 비밀번호 변경
        {
            char old_pw[MAX_PW_LEN], new_pw[MAX_PW_LEN], new_pw2[MAX_PW_LEN];
            clear_screen();
            printf("--- 비밀번호 변경 ---\n");
            if (!get_secure_input(old_pw, sizeof(old_pw), "기존 비밀번호", true, true))
                continue;
            if (!get_secure_input(new_pw, sizeof(new_pw), "새 비밀번호", true, true))
                continue;
            if (!get_secure_input(new_pw2, sizeof(new_pw2), "새 비밀번호 확인", true, true))
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

        case '4': // 회원 탈퇴
        {
            char pw[MAX_PW_LEN], confirm_choice;
            clear_screen();
            printf("--- 회원 탈퇴 ---\n[경고] 모든 데이터가 영구적으로 삭제됩니다.\n");

            confirm_choice = get_single_choice_input("정말 탈퇴하시겠습니까? (y/n)", "ynYN");

            if (confirm_choice == 'y' || confirm_choice == 'Y')
            {
                if (!get_secure_input(pw, sizeof(pw), "비밀번호 확인", true, true))
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
            else
            {
                printf("[클라이언트] 회원 탈퇴가 취소되었습니다.\n");
            }
        }
        break;

        default:
            printf("[클라이언트] 잘못된 선택입니다. (0-4)\n");
            break;
        }
        printf("\n계속하려면 아무 키나 누르세요...");
        getch();
    }
}
