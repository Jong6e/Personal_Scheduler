#include "memo_menu.h"
#include "common_input.h"
#include "memo.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <conio.h>
#include <ctype.h>
#include <time.h>

#define REQUEST_BUF_SIZE 2048
#define REPLY_BUF_SIZE 65536
#define ITEMS_PER_PAGE 10

// --- 화면 모드 관리 ---
typedef enum
{
    MODE_MONTHLY, // 월별 보기 모드
    MODE_SEARCH   // 검색 결과 보기 모드
} ViewMode;

// --- 전역 변수 및 구조체 ---
static Memo g_memo_cache[MAX_MEMOS];                // 서버에서 받아온 메모를 캐싱하는 배열
static int g_memo_count = 0;                        // 캐시된 메모의 수
static ViewMode g_view_mode = MODE_MONTHLY;         // 현재 화면 모드
static char g_search_keyword[MEMO_TITLE_MAX] = {0}; // 현재 검색어

// --- 내부 헬퍼 함수 ---

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
    g_memo_count = 0;
    if (strncmp(data, "OK", 2) == 0 || strncmp(data, "FAIL", 4) == 0 || data[0] == '\0')
    {
        return;
    }

    char *context = NULL;
    char *line = strtok_s(data, "\n", &context);
    while (line != NULL && g_memo_count < MAX_MEMOS)
    {
        char *inner_context = NULL;
        char *id_str = strtok_s(line, "\t", &inner_context);
        char *created_at_str = strtok_s(NULL, "\t", &inner_context);
        char *updated_at_str = strtok_s(NULL, "\t", &inner_context);
        char *title_str = strtok_s(NULL, "\n", &inner_context);

        if (id_str && created_at_str && updated_at_str && title_str)
        {
            Memo *m = &g_memo_cache[g_memo_count];
            m->id = atoi(id_str);

            strncpy(m->created_at, created_at_str, DATETIME_MAX - 1);
            m->created_at[DATETIME_MAX - 1] = '\0';

            strncpy(m->updated_at, updated_at_str, DATETIME_MAX - 1);
            m->updated_at[DATETIME_MAX - 1] = '\0';

            strncpy(m->title, title_str, MEMO_TITLE_MAX - 1);
            m->title[MEMO_TITLE_MAX - 1] = '\0';

            g_memo_count++;
        }
        line = strtok_s(NULL, "\n", &context);
    }
}

// UTF-8 문자의 시작 바이트만 인식하여 너비를 계산하는 안정적인 함수
static void format_title_for_display(const char *original, char *buffer, int buffer_size, int visual_width)
{
    int current_width = 0;
    int original_byte_pos = 0;
    int buffer_byte_pos = 0;

    while (original[original_byte_pos] != '\0')
    {
        int char_width = 0;
        int char_bytes = 0;

        unsigned char first_byte = original[original_byte_pos];

        // UTF-8 시작 바이트를 기준으로 문자 바이트 수와 너비 결정
        if (first_byte < 0x80)
        { // 0xxxxxxx (ASCII)
            char_width = 1;
            char_bytes = 1;
        }
        else if ((first_byte & 0xE0) == 0xC0)
        { // 110xxxxx (2-byte)
            char_width = 2;
            char_bytes = 2;
        }
        else if ((first_byte & 0xF0) == 0xE0)
        { // 1110xxxx (3-byte, 한글 등)
            char_width = 2;
            char_bytes = 3;
        }
        else
        {
            // 잘못된 문자 또는 4바이트 이상의 문자는 1칸짜리 '?'로 표시 (안전장치)
            char_width = 1;
            char_bytes = 1; // 1바이트만 건너뛰어 다음 바이트부터 다시 파싱 시도
        }

        // 이번 문자를 포함하면 너비를 초과하는지 확인
        if (current_width + char_width > visual_width)
        {
            break;
        }

        // 원본 문자열에서 남은 바이트가 실제 문자 바이트 수보다 적거나,
        // 버퍼에 복사할 공간이 부족하면 중단 (잘못된 UTF-8 시퀀스로 인한 오버플로우 방지)
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
    if (g_view_mode == MODE_MONTHLY)
    {
        printf("[ %d년 %02d월 ]     |     이번 달의 메모 [ %d건 ]\n", year, month, g_memo_count);
    }
    else // MODE_SEARCH
    {
        printf("[ 검색 결과 ]     |     '%s'에 대한 메모 [ %d건 ]\n", g_search_keyword, g_memo_count);
    }
    printf("──────────────────────────────────────────────────────────────────────────────────────────────────\n");
    printf(" ID   | 제목                                       | 작성일시            | 수정일시\n");
    printf("──────────────────────────────────────────────────────────────────────────────────────────────────\n");

    if (g_memo_count == 0)
    {
        for (int i = 0; i < 4; i++)
        {
            printf("\n");
        }
        if (g_view_mode == MODE_MONTHLY)
        {
            printf("                                     이 달에 작성된 메모가 없습니다.\n");
        }
        else
        {
            printf("                                       검색 결과가 없습니다.\n");
        }
        for (int i = 0; i < ITEMS_PER_PAGE - 5; i++)
        {
            printf("\n");
        }
    }
    else
    {
        int start_index = page * ITEMS_PER_PAGE;
        for (int i = 0; i < ITEMS_PER_PAGE; i++)
        {
            int current_index = start_index + i;
            if (current_index < g_memo_count)
            {
                Memo *m = &g_memo_cache[current_index];
                char formatted_title[100];
                format_title_for_display(m->title, formatted_title, sizeof(formatted_title), 42);
                printf(" %-4d | %s | %-19s |", m->id, formatted_title, m->created_at);
                if (strcmp(m->created_at, m->updated_at) != 0)
                {
                    printf(" %s\n", m->updated_at);
                }
                else
                {
                    printf("\n");
                }
            }
            else
            {
                printf("\n");
            }
        }
    }

    int total_pages = (g_memo_count + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
    if (total_pages == 0)
        total_pages = 1;
    printf("──────────────────────────────────────────────────────────────────────────────────────────────────\n");

    if (g_view_mode == MODE_MONTHLY)
    {
        printf("Page %d/%d | ↑↓: 월 이동 | ←→: 페이지 이동 | Enter: 조회\n", page + 1, total_pages);
        printf("1: 추가 | 2: 수정 | 3: 삭제 | 4: 검색 | ESC: 뒤로가기\n");
    }
    else // MODE_SEARCH
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

static void add_new_memo(SOCKET sock, const char *user_id)
{
    char title[MEMO_TITLE_MAX], content[MEMO_CONTENT_MAX];
    char request[REQUEST_BUF_SIZE], reply[REPLY_BUF_SIZE];

    clear_screen();
    printf("──────────────────────────────────────────────────────────────────────────────────────────────────\n");
    printf(" 새 메모 작성 (ESC: 취소)\n");
    printf("──────────────────────────────────────────────────────────────────────────────────────────────────\n");

    if (!get_line_input(title, sizeof(title), "\n제목"))
    {
        return;
    }

    if (!get_line_input(content, sizeof(content), "\n내용"))
    {
        return;
    }

    snprintf(request, sizeof(request), "MEMO_ADD:%s:%s:%s", user_id, title, content);
    if (communicate_with_server(sock, request, reply))
    {
        printf("\n[성공] %s\n", reply);
    }
    else
    {
        printf("\n[실패] 서버와 통신하지 못했습니다.\n");
    }
    printf("계속하려면 아무 키나 누르세요...");
    _getch();
}

// 메모 상세보기 기능
static void view_memo_details(SOCKET sock, const char *user_id)
{
    char memo_id_str[10];
    char request[REQUEST_BUF_SIZE], reply[REPLY_BUF_SIZE];

    if (get_validated_input(memo_id_str, sizeof(memo_id_str), "\n조회할 메모 ID", false, false))
    {
        snprintf(request, sizeof(request), "MEMO_VIEW:%s:%s", user_id, memo_id_str);
        if (communicate_with_server(sock, request, reply) && strncmp(reply, "OK:", 3) == 0)
        {
            // 서버 응답 형식: OK:id\tcreated_at\tupdated_at\ttitle\tcontent
            char *p_reply = reply + 3;
            char created_at[DATETIME_MAX], updated_at[DATETIME_MAX], title[MEMO_TITLE_MAX], content[MEMO_CONTENT_MAX];

            sscanf(p_reply, "%*[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\n]", created_at, updated_at, title, content);

            display_detail_template(title, created_at, updated_at, content);
            printf("ESC: 뒤로가기\n");

            while (_getch() != 27)
                ; // ESC를 누를 때까지 대기
        }
        else
        {
            printf("%s\n", reply);
            printf("계속하려면 아무 키나 누르세요...");
            _getch();
        }
    }
}

static void update_existing_memo(SOCKET sock, const char *user_id)
{
    char memo_id_str[10];
    char request[REQUEST_BUF_SIZE], reply[REPLY_BUF_SIZE];

    if (!get_validated_input(memo_id_str, sizeof(memo_id_str), "\n수정할 메모 ID", false, false))
        return;

    snprintf(request, sizeof(request), "MEMO_VIEW:%s:%s", user_id, memo_id_str);
    if (communicate_with_server(sock, request, reply) && strncmp(reply, "OK:", 3) == 0)
    {
        char *p_reply = reply + 3;
        char created_at[DATETIME_MAX], updated_at[DATETIME_MAX], title[MEMO_TITLE_MAX], content[MEMO_CONTENT_MAX];
        sscanf(p_reply, "%*[^\t]\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\n]", created_at, updated_at, title, content);

        display_detail_template(title, created_at, updated_at, content);

        // ESC 취소 기능 안내 문구 추가
        printf("ESC: 수정 취소\n\n");

        char new_content[MEMO_CONTENT_MAX];
        if (!get_line_input(new_content, sizeof(new_content), "새 내용 입력"))
            return;

        snprintf(request, sizeof(request), "MEMO_UPDATE:%s:%s:%s", user_id, memo_id_str, new_content);
        communicate_with_server(sock, request, reply);
        printf("\n%s\n", reply);
    }
    else
    {
        printf("\n%s\n", reply);
    }
    printf("\n계속하려면 아무 키나 누르세요...");
    _getch();
}

// 삭제 시 확인 절차 추가
static void delete_existing_memo(SOCKET sock, const char *user_id)
{
    char memo_id_str[10];
    char request[REQUEST_BUF_SIZE], reply[REPLY_BUF_SIZE];

    if (!get_validated_input(memo_id_str, sizeof(memo_id_str), "\n삭제할 메모 ID", false, false))
    {
        return; // ESC 입력 시 함수 종료
    }

    printf("\n정말로 이 메모를 삭제하시겠습니까? (y/n) ");
    char ch = _getch();
    printf("\n");

    if (ch == 'y' || ch == 'Y')
    {
        snprintf(request, sizeof(request), "MEMO_DELETE:%s:%s", user_id, memo_id_str);
        communicate_with_server(sock, request, reply);
        printf("\n[서버 응답] %s\n", reply);
    }
    else
    {
        printf("\n삭제가 취소되었습니다.\n");
    }

    printf("\n계속하려면 아무 키나 누르세요...");
    _getch();
}

// 검색을 요청하고 실행하는 새로운 함수
static bool prompt_and_execute_search(SOCKET sock, const char *user_id)
{
    char field_char;
    char keyword[MEMO_TITLE_MAX];
    const char *field;

    printf("\n"); // 프롬프트와 푸터 사이에 한 줄 띄우기
    field_char = get_single_choice_input("검색 기준 (1:제목, 2:내용, 3:전체)", "123");

    if (field_char == 27) // ESC
        return false;

    int choice = field_char - '0'; // '1' -> 1
    if (choice == 1)
        field = "title";
    else if (choice == 2)
        field = "content";
    else if (choice == 3)
        field = "all";
    else
    {
        // get_single_choice_input에서 이미 필터링하므로 이 코드는 실행되지 않음
        return false;
    }

    if (!get_line_input(keyword, sizeof(keyword), "검색어"))
        return false;

    strncpy(g_search_keyword, keyword, sizeof(g_search_keyword) - 1);
    g_search_keyword[sizeof(g_search_keyword) - 1] = '\0';

    char request[REQUEST_BUF_SIZE], reply[REPLY_BUF_SIZE];
    snprintf(request, sizeof(request), "MEMO_SEARCH:%s:%s:%s", user_id, field, keyword);

    if (communicate_with_server(sock, request, reply))
    {
        parse_and_cache_memos(reply);
        g_view_mode = MODE_SEARCH;
        return true;
    }
    return false;
}

void memo_menu_loop(SOCKET sock, const char *logged_in_id)
{
    char request[REQUEST_BUF_SIZE], reply[REPLY_BUF_SIZE];
    time_t t;
    struct tm tm_now;

    t = time(NULL);
    localtime_s(&tm_now, &t);
    int current_year = tm_now.tm_year + 1900;
    int current_month = tm_now.tm_mon + 1;
    int current_page = 0;
    bool needs_update = true;
    g_view_mode = MODE_MONTHLY; // 함수 진입 시 항상 월별 모드로 초기화

    while (true)
    {
        if (needs_update)
        {
            // 월별 모드일 때만 서버에서 월별 데이터를 가져옴
            if (g_view_mode == MODE_MONTHLY)
            {
                snprintf(request, sizeof(request), "MEMO_LIST_BY_MONTH:%s:%d:%d", logged_in_id, current_year, current_month);
                if (communicate_with_server(sock, request, reply))
                {
                    parse_and_cache_memos(reply);
                    current_page = 0;
                }
            }
            needs_update = false;
        }

        display_ui(current_year, current_month, current_page);

        int ch = _getch();
        if (ch == 224 || ch == 0)
        {
            ch = _getch();
            int total_pages = (g_memo_count + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
            if (total_pages == 0)
                total_pages = 1;

            switch (ch)
            {
            case 72: // Up (월 이동은 월별 모드에서만)
                if (g_view_mode == MODE_MONTHLY)
                {
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
            case '1':
            case '2':
            case '3':
                if (ch == '1')
                    add_new_memo(sock, logged_in_id);
                else if (ch == '2')
                    update_existing_memo(sock, logged_in_id);
                else
                    delete_existing_memo(sock, logged_in_id);

                if (g_view_mode == MODE_MONTHLY)
                {
                    t = time(NULL);
                    localtime_s(&tm_now, &t);
                    current_year = tm_now.tm_year + 1900;
                    current_month = tm_now.tm_mon + 1;
                }
                needs_update = true;
                break;
            case '4': // 검색 (월별 모드에서만 활성화)
                if (g_view_mode == MODE_MONTHLY)
                {
                    if (prompt_and_execute_search(sock, logged_in_id))
                    {
                        current_page = 0;
                    }
                    // 검색 후에는 화면을 바로 다시 그리므로 needs_update는 false
                }
                break;
            }
        }
    }
}
