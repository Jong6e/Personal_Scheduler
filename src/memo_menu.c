#include "memo_menu.h"
#include "common_input.h"
#include "memo.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <ctype.h>
#include <time.h>
#include <direct.h>   // for _mkdir
#include <sys/stat.h> // for stat
#include <windows.h>  // GetModuleFileName 사용

#define REQUEST_BUF_SIZE 2048
#define REPLY_BUF_SIZE 65536
#define ITEMS_PER_PAGE 10
#define CLIENT_MEMO_CACHE_SIZE 100
#define MAX_MEMOS_PER_PAGE 10

// 화면 모드 관리
typedef enum
{
    MODE_MONTHLY, // 월별 보기 모드
    MODE_SEARCH   // 검색 결과 보기 모드
} ViewMode;

//  전역 변수 및 구조체
static Memo g_memo_cache[CLIENT_MEMO_CACHE_SIZE];       // 서버에서 받아온 메모를 캐싱하는 배열
static int g_memo_count = 0;                            // 캐시된 메모의 수
static ViewMode g_view_mode = MODE_MONTHLY;             // 현재 화면 모드
static char g_search_keyword[MAX_MEMO_TITLE_LEN] = {0}; // 현재 검색어

// 함수 원형 선언
static void display_ui(int year, int month, int page);                              // 화면 관련 함수
static void fetch_and_display_memos(SOCKET sock, const char *user_id, int page);    // 메모 관련 함수
static void view_memo_details(SOCKET sock, const char *user_id);                    // 메모 상세보기 함수
static void add_new_memo(SOCKET sock, const char *user_id);                         // 메모 추가 함수
static void update_existing_memo(SOCKET sock, const char *user_id);                 // 메모 수정 함수
static void delete_existing_memo(SOCKET sock, const char *user_id);                 // 메모 삭제 함수
static bool prompt_and_execute_search(SOCKET sock, const char *user_id);            // 검색 관련 함수
static void handle_download_process(SOCKET sock, const char *user_id, int memo_id); // 다운로드 관련 함수

// 문자열의 앞뒤 공백을 제거하는 헬퍼 함수
static void trim_whitespace(char *str)
{
    if (!str)
        return;

    char *start = str;
    // 앞쪽 공백 건너뛰기
    while (isspace((unsigned char)*start))
    {
        start++;
    }

    // 문자열을 앞으로 이동
    memmove(str, start, strlen(start) + 1);

    // 뒤쪽 공백 제거
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
    {
        end--;
    }

    // 새 끝 설정
    *(end + 1) = '\0';
}

// 서버와 통신
static bool communicate_with_server(SOCKET sock, const char *request, char *reply)
{
    if (send(sock, request, strlen(request), 0) < 0)
        return false;
    int bytes = recv(sock, reply, REPLY_BUF_SIZE - 1, 0);
    if (bytes <= 0)
        return false;
    reply[bytes] = '\0';
    return true;
}

// 서버에서 받은 데이터를 파싱하여 g_memo_cache에 저장
static void parse_and_cache_memos(char *data)
{
    // 데이터 초기화
    g_memo_count = 0;
    if (strncmp(data, "OK", 2) == 0 || strncmp(data, "FAIL", 4) == 0 || data[0] == '\0')
    {
        return;
    }
    // 데이터 파싱
    char *context = NULL;
    char *line = strtok_s(data, "\n", &context);
    // 메모 캐시 초기화
    while (line != NULL && g_memo_count < CLIENT_MEMO_CACHE_SIZE)
    {
        // 메모 데이터 파싱
        char *inner_context = NULL;
        char *id_str = strtok_s(line, "\t", &inner_context);
        char *created_at_str = strtok_s(NULL, "\t", &inner_context);
        char *updated_at_str = strtok_s(NULL, "\t", &inner_context);
        char *title_str = strtok_s(NULL, "\n", &inner_context);

        if (id_str && created_at_str && updated_at_str && title_str)
        {
            // 메모 데이터 저장
            Memo *m = &g_memo_cache[g_memo_count];
            m->id = atoi(id_str);
            // 작성일시 저장
            strncpy(m->created_at, created_at_str, MAX_DATETIME_LEN - 1);
            m->created_at[MAX_DATETIME_LEN - 1] = '\0';
            // 수정일시 저장
            strncpy(m->updated_at, updated_at_str, MAX_DATETIME_LEN - 1);
            m->updated_at[MAX_DATETIME_LEN - 1] = '\0';
            // 제목 저장
            strncpy(m->title, title_str, MAX_MEMO_TITLE_LEN - 1);
            m->title[MAX_MEMO_TITLE_LEN - 1] = '\0';

            // 메모 카운트 증가
            g_memo_count++;
        }
        // 다음 라인 파싱
        line = strtok_s(NULL, "\n", &context);
    }
}

// UTF-8 문자의 시작 바이트만 인식하여 너비를 계산하는 안정적인 함수
static void format_title_for_display(const char *original, char *buffer, int buffer_size, int visual_width)
{
    // 현재 너비 초기화
    int current_width = 0;
    int original_byte_pos = 0;
    int buffer_byte_pos = 0;

    // 원본 문자열 파싱
    while (original[original_byte_pos] != '\0')
    {
        // 문자 너비 초기화
        int char_width = 0;
        int char_bytes = 0;

        // 첫 바이트 파싱
        unsigned char first_byte = original[original_byte_pos];

        // UTF-8 시작 바이트를 기준으로 문자 바이트 수와 너비 결정
        if (first_byte < 0x80)
        { // 0xxxxxxx (ASCII)
            char_width = 1;
            char_bytes = 1;
        }
        // 2바이트 문자
        else if ((first_byte & 0xE0) == 0xC0)
        { // 110xxxxx (2-byte)
            char_width = 2;
            char_bytes = 2;
        }
        // 3바이트 문자
        else if ((first_byte & 0xF0) == 0xE0)
        { // 1110xxxx (3-byte, 한글 등)
            char_width = 2;
            char_bytes = 3;
        }
        // 잘못된 문자
        else
        {
            char_width = 1;
            char_bytes = 1;
        }

        // 이번 문자를 포함하면 너비를 초과하는지 확인
        if (current_width + char_width > visual_width)
        {
            break;
        }

        if (strlen(original + original_byte_pos) < char_bytes || buffer_byte_pos + char_bytes >= buffer_size)
        {
            break;
        }

        // 문자를 버퍼로 복사
        memcpy(buffer + buffer_byte_pos, original + original_byte_pos, char_bytes);

        current_width += char_width;
        original_byte_pos += char_bytes;
        buffer_byte_pos += char_bytes;
    }

    // 나머지 공간을 공백으로 채워 정렬
    for (int i = current_width; i < visual_width; i++)
    {
        if (buffer_byte_pos < buffer_size - 1)
        {
            buffer[buffer_byte_pos++] = ' ';
        }
    }
    buffer[buffer_byte_pos] = '\0';
}

// UI를 그리는 함수를 ViewMode에 따라 다르게 그리도록 수정
static void display_ui(int year, int month, int page)
{
    clear_screen();
    // 월별 모드일 때
    if (g_view_mode == MODE_MONTHLY)
    {
        // 월별 모드 표시
        printf("[ %d년 %02d월 ]     |     이번 달의 메모 [ %d건 ]\n", year, month, g_memo_count);
    }
    // 검색 모드일 때
    else // MODE_SEARCH
    {
        // 검색 모드 표시
        printf("[ 검색 결과 ]     |     '%s'에 대한 메모 [ %d건 ]\n", g_search_keyword, g_memo_count);
    }
    printf("──────────────────────────────────────────────────────────────────────────────────────────────────\n");
    printf(" ID   | 제목                                       | 작성일시            | 수정일시\n");
    printf("──────────────────────────────────────────────────────────────────────────────────────────────────\n");

    // 메모가 없을 때
    if (g_memo_count == 0)
    {
        for (int i = 0; i < 4; i++)
        {
            printf("\n");
        }
        // 월별 모드일 때
        if (g_view_mode == MODE_MONTHLY)
        {
            printf("                                     이 달에 작성된 메모가 없습니다.\n");
        }
        // 검색 모드일 때
        else
        {
            printf("                                       검색 결과가 없습니다.\n");
        }
        // 페이지 빈 공간 추가
        for (int i = 0; i < ITEMS_PER_PAGE - 5; i++)
        {
            printf("\n");
        }
    }
    else
    {
        // 페이지 시작 인덱스 계산
        int start_index = page * ITEMS_PER_PAGE;
        for (int i = 0; i < ITEMS_PER_PAGE; i++)
        {
            // 현재 인덱스 계산
            int current_index = start_index + i;
            if (current_index < g_memo_count)
            {
                // 메모 데이터 가져오기
                Memo *m = &g_memo_cache[current_index];
                char formatted_title[100];
                format_title_for_display(m->title, formatted_title, sizeof(formatted_title), 42);
                printf(" %-4d | %s | %-19s |", m->id, formatted_title, m->created_at);
                // 수정일시가 있으면 출력
                if (strcmp(m->created_at, m->updated_at) != 0)
                {
                    // 수정일시 출력
                    printf(" %s\n", m->updated_at);
                }
                else
                {
                    // 수정일시 없으면 줄바꿈
                    printf("\n");
                }
            }
            else
            {
                // 메모가 없으면 줄바꿈
                printf("\n");
            }
        }
    }

    // 총 페이지 수 계산
    int total_pages = (g_memo_count + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
    if (total_pages == 0)
    {
        total_pages = 1;
    }
    printf("──────────────────────────────────────────────────────────────────────────────────────────────────\n");

    // 월별 모드일 때
    if (g_view_mode == MODE_MONTHLY)
    {
        printf("Page %d/%d | ↑↓: 월 이동 | ←→: 페이지 이동 | Enter: 조회\n", page + 1, total_pages);
        printf("1: 추가 | 2: 수정 | 3: 삭제 | 4: 검색 | ESC: 뒤로가기\n");
    }
    // 검색 모드일 때
    else
    {
        printf("Page %d/%d | ←→: 페이지 이동 | Enter: 조회 | ESC: 검색 종료\n", page + 1, total_pages);
        printf("1: 추가 | 2: 수정 | 3: 삭제\n");
    }
}

// 상세보기 및 수정용 UI 템플릿을 그리는 함수
static void display_detail_template(const char *title, const char *created_at, const char *updated_at, const char *content)
{
    clear_screen();
    printf("──────────────────────────────────────────────────────────────────────────────────────────────────\n");
    printf(" 제목: %s\n", title);
    printf(" 작성: %s", created_at);
    if (strcmp(created_at, updated_at) != 0)
    {
        printf(" | 수정: %s", updated_at);
    }
    printf("\n");
    printf("──────────────────────────────────────────────────────────────────────────────────────────────────\n\n");
    printf("%s\n\n", content);
    printf("──────────────────────────────────────────────────────────────────────────────────────────────────\n");
}

// 새 메모 추가 기능
static void add_new_memo(SOCKET sock, const char *user_id)
{
    char title[MAX_MEMO_TITLE_LEN], content[MAX_MEMO_CONTENT_LEN];
    char request[REQUEST_BUF_SIZE], reply[REPLY_BUF_SIZE];

    clear_screen();
    printf("──────────────────────────────────────────────────────────────────────────────────────────────────\n");
    printf(" 새 메모 작성\n");
    printf("──────────────────────────────────────────────────────────────────────────────────────────────────\n");
    if (!get_line_input(title, sizeof(title), "제목"))
        return;
    if (!get_line_input(content, sizeof(content), "내용"))
        return;

    if (strlen(title) == 0 || strlen(content) == 0)
    {
        printf("[클라이언트] 제목과 내용은 비워둘 수 없습니다.\n");
        Sleep(1000);
        return;
    }

    snprintf(request, sizeof(request), "MEMO_ADD:%s:%s:%s", user_id, title, content);
    if (communicate_with_server(sock, request, reply) && strncmp(reply, "OK", 2) == 0)
    {
        printf("[클라이언트] %s\n", reply + 3);
    }
    else
    {
        printf("[클라이언트] %s\n", reply + 5);
    }
    Sleep(1000); // 1초 대기 후 자동으로 메뉴로 복귀
}

// 메모 상세보기 기능
static void view_memo_details(SOCKET sock, const char *user_id)
{
    // 메모 ID 버퍼 선언
    char memo_id_str[10];
    char request[REQUEST_BUF_SIZE], reply[REPLY_BUF_SIZE];
    // 메모 ID 입력받기
    if (get_secure_input(memo_id_str, sizeof(memo_id_str), "\n조회할 메모 ID", false, true))
    {
        // 서버에 메모 상세 요청
        snprintf(request, sizeof(request), "MEMO_VIEW:%s:%s", user_id, memo_id_str);
        if (communicate_with_server(sock, request, reply) && strncmp(reply, "OK:", 3) == 0)
        {
            // 서버 응답 형식: OK:id\tcreated_at\tupdated_at\ttitle\tcontent
            char *p_reply = reply + 3;
            char created_at[MAX_DATETIME_LEN], updated_at[MAX_DATETIME_LEN], title[MAX_MEMO_TITLE_LEN], content[MAX_MEMO_CONTENT_LEN];
            // 서버 응답 파싱
            sscanf(p_reply, "%*[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\n]", created_at, updated_at, title, content);
            // 상세보기 템플릿 표시
            display_detail_template(title, created_at, updated_at, content);
            printf("D: 메모 다운로드 | Enter: 뒤로가기\n");
            // 메모 다운로드 또는 뒤로가기 선택
            int ch = _getch();
            if (ch == 'd' || ch == 'D')
            {
                // 메모 다운로드 처리
                int memo_id = atoi(memo_id_str);
                handle_download_process(sock, user_id, memo_id);
            }
        }
        else
        {
            // 서버 응답 출력
            printf("%s\n", reply);
            printf("계속하려면 아무 키나 누르세요...");
            _getch();
        }
    }
}

// 메모 수정
static void update_existing_memo(SOCKET sock, const char *user_id)
{
    char request[REQUEST_BUF_SIZE];
    char reply[REPLY_BUF_SIZE];
    char id_str[10];

    // 1. 수정할 메모 ID 입력 받기
    if (!get_line_input(id_str, sizeof(id_str), "수정할 메모의 ID를 입력하세요"))
    {
        return; // ESC 입력 시 복귀
    }
    // 메모 ID 변환
    int memo_id = atoi(id_str);
    // 메모 ID 유효성 검사
    if (memo_id == 0)
    {
        printf("\n[오류] 유효하지 않은 ID입니다.\n");
        Sleep(1000);
        return;
    }
    // 서버에 기존 메모 내용 요청 (MEMO_VIEW)
    snprintf(request, sizeof(request), "MEMO_VIEW:%s:%d", user_id, memo_id);
    // 서버 응답 확인
    if (!communicate_with_server(sock, request, reply) || strncmp(reply, "OK:", 3) != 0)
    {
        printf("\n[오류] 메모를 불러오는 데 실패했습니다: %s\n", reply);
        Sleep(1000);
        return;
    }
    // 응답 파싱하여 기존 내용 추출
    char *context = NULL;
    strtok_s(reply, ":", &context);                 // "OK" 부분 무시
    char *memo_data = strtok_s(NULL, "", &context); // 나머지 전체 데이터
    if (memo_data == NULL)
    {
        printf("\n[오류] 서버 응답이 올바르지 않습니다.\n");
        Sleep(1000);
        return;
    }
    // 응답 파싱하여 기존 내용 추출
    char *inner_context = NULL;
    strtok_s(memo_data, "\t", &inner_context); // id
    char *created_at = strtok_s(NULL, "\t", &inner_context);
    char *updated_at = strtok_s(NULL, "\t", &inner_context);
    char *original_title = strtok_s(NULL, "\t", &inner_context);
    char *original_content = strtok_s(NULL, "\n", &inner_context);

    if (original_content == NULL)
    {
        original_content = original_title; // 제목만 있는 경우
    }
    // 화면 정리 및 기존 내용 표시
    clear_screen();
    printf("--- 메모 수정 ---\n");
    printf("기존 제목: %s (제목은 변경되지 않습니다)\n", original_title);
    printf("---------------------------------\n");
    printf("기존 내용:\n%s\n", original_content);
    printf("---------------------------------\n\n");
    // 새로운 내용 입력 받기 (add_new_memo와 유사한 방식)
    char new_content[MAX_MEMO_CONTENT_LEN];
    if (!get_line_input(new_content, sizeof(new_content), "새로운 내용을 입력하세요"))
    {
        return; // ESC 입력 시 복귀
    }
    // 내용 유효성 검사
    if (strlen(new_content) == 0)
    {
        printf("\n[오류] 내용은 비워둘 수 없습니다.\n");
        Sleep(1000);
        return;
    }
    // 서버에 수정 요청 (MEMO_UPDATE)
    snprintf(request, sizeof(request), "MEMO_UPDATE:%s:%d:%s", user_id, memo_id, new_content);
    if (communicate_with_server(sock, request, reply) && strncmp(reply, "OK", 2) == 0)
    {
        printf("\n[성공] 메모가 수정되었습니다.\n");
    }
    else
    {
        printf("\n[오류] 메모 수정에 실패했습니다: %s\n", reply);
    }
    Sleep(1000);
}

// 삭제 시 확인 절차 추가
static void delete_existing_memo(SOCKET sock, const char *user_id)
{
    char memo_id_str[10];
    char request[REQUEST_BUF_SIZE], reply[REPLY_BUF_SIZE];
    int memo_id;
    // 메모 ID 입력받기
    if (!get_secure_input(memo_id_str, sizeof(memo_id_str), "삭제할 메모 ID", false, true))
        return;
    memo_id = atoi(memo_id_str);

    // 서버에 메모 삭제 요청
    snprintf(request, sizeof(request), "MEMO_DELETE:%s:%d", user_id, memo_id);
    if (communicate_with_server(sock, request, reply) && strncmp(reply, "OK", 2) == 0)
    {
        printf("[클라이언트] %s\n", reply + 3);
    }
    else
    {
        printf("[클라이언트] %s\n", reply + 5);
    }
    Sleep(1000); // 1초 대기 후 자동으로 메뉴로 복귀
}

// 검색을 요청하고 실행하는 새로운 함수
static bool prompt_and_execute_search(SOCKET sock, const char *user_id)
{
    // 변수 선언
    char choice;
    char keyword[MAX_MEMO_TITLE_LEN];
    const char *field;
    // 검색 기준 입력받기
    printf("\n");
    choice = get_single_choice_input("검색 기준 (1:제목, 2:내용, 3:전체)", "123");
    // 검색 기준 유효성 검사
    if (choice == KEY_ESC)
    {
        printf("\n[알림] 검색이 취소되었습니다.\n");
        Sleep(1000);
        return false;
    }
    // 검색 기준 설정
    if (choice == '1')
        field = "title";
    else if (choice == '2')
        field = "content";
    else // '3'
        field = "all";
    // 검색어 입력받기
    if (!get_line_input(keyword, sizeof(keyword), "검색어"))
    {
        printf("\n[알림] 검색이 취소되었습니다.\n");
        Sleep(1000);
        return false;
    }
    // 입력된 키워드의 앞뒤 공백 제거
    trim_whitespace(keyword);
    // 검색어 유효성 검사
    if (strlen(keyword) == 0)
    {
        printf("\n[오류] 검색어는 비워둘 수 없습니다. 공백만 입력할 수 없습니다.\n");
        Sleep(1000);
        return false;
    }
    // 검색어 복사
    strncpy(g_search_keyword, keyword, sizeof(g_search_keyword) - 1);
    g_search_keyword[sizeof(g_search_keyword) - 1] = '\0';
    // 요청 전송
    char request[REQUEST_BUF_SIZE], reply[REPLY_BUF_SIZE];
    // 요청 전송
    snprintf(request, sizeof(request), "MEMO_SEARCH:%s:%s:%s", user_id, field, keyword);
    // 서버 응답 확인
    if (communicate_with_server(sock, request, reply))
    {
        // 서버 응답 파싱
        parse_and_cache_memos(reply);
        // 검색 모드 설정
        g_view_mode = MODE_SEARCH;
        return true;
    }
    return false;
}

// 다운로드 과정을 처리하는 핵심 핸들러 함수
static void handle_download_process(SOCKET sock, const char *user_id, int memo_id)
{
    clear_screen();
    printf("--- 메모 다운로드 ---\n\n");

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

    // 실행 파일 위치 기반 절대 경로 생성 (안정성 강화)
    char exe_path[MAX_PATH];
    GetModuleFileName(NULL, exe_path, MAX_PATH); // ps_client.exe의 전체 경로 획득

    // 경로에서 파일 이름(ps_client.exe) 제거 -> 프로젝트 루트 경로가 됨
    char *last_slash = strrchr(exe_path, '\\');
    if (last_slash != NULL)
    {
        *last_slash = '\0';
    }
    // 절대 경로 생성 완료
    // downloads 디렉터리 확인 및 생성
    char download_dir[MAX_PATH];
    snprintf(download_dir, sizeof(download_dir), "%s\\downloads", exe_path);
    struct stat st = {0};
    if (stat(download_dir, &st) == -1)
    {
        _mkdir(download_dir);
    }
    // 파일명 생성 (절대 경로 사용)
    char filepath[MAX_PATH];
    time_t t = time(NULL);
    struct tm now;
    localtime_s(&now, &t);
    // 개별 메모 다운로드
    if (memo_id != -1)
    {
        snprintf(filepath, sizeof(filepath), "%s\\memo_%s_%d_%d%02d%02d.%s", download_dir, user_id, memo_id, now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, ext);
    }
    // 전체 메모 다운로드
    else
    {
        snprintf(filepath, sizeof(filepath), "%s\\allmemo_%s_%d%02d%02d.%s", download_dir, user_id, now.tm_year + 1900, now.tm_mon + 1, now.tm_mday, ext);
    }
    // 파일 존재 여부 확인
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
    // 최종 확인 (사용자에게는 상대 경로처럼 보여주기)
    char *display_path = strstr(filepath, "downloads\\");
    if (display_path == NULL)
    {
        display_path = filepath; // 예외처리: 못찾으면 전체 경로 표시
    }
    printf("\n'downloads' 폴더에 '%s'(으)로 저장하시겠습니까?", display_path + strlen("downloads\\"));
    choice = get_single_choice_input(" (Y/N)", "yYnN");
    if (choice == 'n' || choice == 'N' || choice == KEY_ESC)
    {
        printf("\n[알림] 다운로드가 취소되었습니다.\n");
        Sleep(1000);
        return;
    }
    // 서버 요청
    char request[REQUEST_BUF_SIZE], reply[REPLY_BUF_SIZE];
    if (memo_id == -1)
    {
        snprintf(request, sizeof(request), "DOWNLOAD_ALL:%s:%s", user_id, format_str);
    }
    else
    {
        snprintf(request, sizeof(request), "DOWNLOAD_SINGLE:%s:%d:%s", user_id, memo_id, format_str);
    }

    printf("\n다운로드 중...\n");
    if (!communicate_with_server(sock, request, reply) || strncmp(reply, "OK:", 3) != 0)
    {
        printf("\n[오류] 다운로드에 실패했습니다: %s\n", reply);
        Sleep(1500);
        return;
    }
    // 데이터 수신 및 파일 저장
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
    // 결과 안내
    printf("\n[성공] 다운로드가 완료되었습니다. (%s)\n", filepath);
    Sleep(1500);
}

void memo_menu_loop(SOCKET sock, const char *logged_in_id)
{
    // 요청 버퍼 선언
    char request[REQUEST_BUF_SIZE], reply[REPLY_BUF_SIZE];
    // 현재 시간 가져오기
    time_t t;
    struct tm tm_now;
    // 현재 년도와 월 가져오기
    t = time(NULL);
    localtime_s(&tm_now, &t);
    int current_year = tm_now.tm_year + 1900;
    int current_month = tm_now.tm_mon + 1;
    int current_page = 0;
    bool needs_update = true;
    // 초기 뷰 모드 설정
    g_view_mode = MODE_MONTHLY;

    // 메모 메뉴 루프
    while (true)
    {
        // 데이터 업데이트 필요 시
        if (needs_update)
        {
            // 월별 모드일 때만 서버에서 월별 데이터를 가져옴 (검색 모드에서는 사용하지 않음)
            if (g_view_mode == MODE_MONTHLY)
            {
                snprintf(request, sizeof(request), "MEMO_LIST_BY_MONTH:%s:%d:%d", logged_in_id, current_year, current_month);
                if (communicate_with_server(sock, request, reply))
                {
                    // 서버 응답 파싱
                    parse_and_cache_memos(reply);
                    current_page = 0;
                }
            }
            needs_update = false;
        }

        // 화면 표시
        display_ui(current_year, current_month, current_page);

        // 키 입력 처리
        int ch = _getch();
        if (ch == 224 || ch == 0)
        {
            ch = _getch();
            // 총 페이지 수 계산
            int total_pages = (g_memo_count + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
            if (total_pages == 0)
                total_pages = 1;

            // 키 입력에 따른 동작 분기
            switch (ch)
            {
            case 72: // Up (월 이동은 월별 모드에서만)
                if (g_view_mode == MODE_MONTHLY)
                {
                    // 현재 시간 가져오기
                    t = time(NULL);
                    localtime_s(&tm_now, &t);
                    int real_year = tm_now.tm_year + 1900;
                    int real_month = tm_now.tm_mon + 1;
                    if (current_year * 12 + current_month < real_year * 12 + real_month)
                    {
                        current_month++;
                        if (current_month > 12)
                        {
                            current_month = 1;
                            current_year++;
                        }
                        needs_update = true;
                    }
                }
                break;
            case 80: // Down (월 이동은 월별 모드에서만)
                if (g_view_mode == MODE_MONTHLY)
                {
                    current_month--;
                    if (current_month < 1)
                    {
                        current_month = 12;
                        current_year--;
                    }
                    needs_update = true;
                }
                break;
            case 75: // Left
                if (current_page > 0)
                    current_page--;
                break;
            case 77: // Right
                if (current_page < total_pages - 1)
                    current_page++;
                break;
            }
        }
        else
        {
            switch (ch)
            {
            case 27: // ESC
                if (g_view_mode == MODE_SEARCH)
                {
                    g_view_mode = MODE_MONTHLY;
                    needs_update = true; // 월별 목록으로 갱신
                }
                else
                {
                    return; // 월별 모드에서 ESC는 뒤로가기
                }
                break;
            case '\r': // Enter
                view_memo_details(sock, logged_in_id);
                needs_update = true; // 상세보기 후, 화면을 새로고침
                break;
            // 메모 추가
            case '1':
                add_new_memo(sock, logged_in_id);
                needs_update = true;
                break;
            // 메모 수정
            case '2':
                update_existing_memo(sock, logged_in_id);
                needs_update = true;
                break;
            // 메모 삭제
            case '3':
                delete_existing_memo(sock, logged_in_id);
                needs_update = true;
                break;
            // 검색
            case '4': // 월별 모드에서만 '검색'으로 동작
                if (g_view_mode == MODE_MONTHLY)
                {
                    // 검색 요청 전송
                    if (prompt_and_execute_search(sock, logged_in_id))
                    {
                        current_page = 0;
                    }
                    needs_update = true;
                }
                break;
            }
        }
    }
}
