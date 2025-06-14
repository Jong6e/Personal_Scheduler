#include "memo.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_MEMO_FILE_PATH 100
#define TEMP_MEMO_FILE "data/memos.tmp"
#define DELIMITER " \t " // 탭과 공백을 구분자로 사용

// --- 내부 헬퍼 함수 ---

// 사용자 ID를 기반으로 메모 파일 경로를 생성합니다.
static void get_memo_filepath(const char *user_id, char *filepath)
{
    snprintf(filepath, MAX_MEMO_FILE_PATH, "data/%s_memos.txt", user_id);
}

// 현재 시간을 "YYYY-MM-DD HH:MM:SS" 형식의 문자열로 가져옵니다.
static void get_current_datetime(char *datetime_str)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(datetime_str, DATETIME_MAX, "%Y-%m-%d %H:%M:%S", t);
}

// 파일에서 다음 메모 ID를 결정합니다.
static int get_next_memo_id(const char *filepath)
{
    FILE *file = fopen(filepath, "r");
    if (!file)
        return 1;

    int max_id = 0;
    int id;
    while (fscanf(file, "%d", &id) == 1)
    {
        if (id > max_id)
            max_id = id;
        // 파일의 나머지 한 줄을 소비
        char buffer[MEMO_CONTENT_MAX + 100];
        fgets(buffer, sizeof(buffer), file);
    }
    fclose(file);
    return max_id + 1;
}

// --- 공개 API 함수 ---

bool list_memos_for_user(const char *user_id, char *output, int output_size)
{
    char filepath[MAX_MEMO_FILE_PATH];
    get_memo_filepath(user_id, filepath);

    FILE *file = fopen(filepath, "r");
    if (!file)
    {
        snprintf(output, output_size, "작성된 메모가 없습니다.");
        return true;
    }

    int offset = 0;
    char line[sizeof(Memo)];
    bool found = false;

    offset += snprintf(output + offset, output_size - offset, "[%s님의 메모 목록]\n", user_id);

    while (fgets(line, sizeof(line), file))
    {
        int id;
        char title[MEMO_TITLE_MAX];
        // 형식: id \t created_at \t updated_at \t title \t content
        sscanf(line, "%d\t%*[^\t]\t%*[^\t]\t%[^\t]", &id, title);

        offset += snprintf(output + offset, output_size - offset, "  - [%d] %s\n", id, title);
        found = true;
    }

    fclose(file);
    if (!found)
    {
        snprintf(output, output_size, "작성된 메모가 없습니다.");
    }
    return true;
}

bool get_memo_by_id(const char *user_id, int memo_id, char *output, int output_size)
{
    char filepath[MAX_MEMO_FILE_PATH];
    get_memo_filepath(user_id, filepath);
    FILE *file = fopen(filepath, "r");
    if (!file)
    {
        snprintf(output, output_size, "FAIL:사용자의 메모 파일을 찾을 수 없습니다.");
        return false;
    }

    char line[sizeof(Memo) + 100];
    bool found = false;
    while (fgets(line, sizeof(line), file))
    {
        int id;
        sscanf(line, "%d", &id);
        if (id == memo_id)
        {
            // OK:id\tcreated_at\tupdated_at\ttitle\tcontent 형식으로 전체 데이터 반환
            char created_at[DATETIME_MAX], updated_at[DATETIME_MAX], title[MEMO_TITLE_MAX], content[MEMO_CONTENT_MAX];
            sscanf(line, "%d\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\n]", &id, created_at, updated_at, title, content);
            snprintf(output, output_size, "OK:%d\t%s\t%s\t%s\t%s", id, created_at, updated_at, title, content);
            found = true;
            break;
        }
    }

    fclose(file);

    if (!found)
    {
        snprintf(output, output_size, "FAIL:메모 ID %d를 찾을 수 없습니다.", memo_id);
    }
    return found;
}

bool add_memo_for_user(const char *user_id, const char *title, const char *content)
{
    if (strlen(content) == 0)
    {
        return false; // 내용은 비워둘 수 없음
    }

    Memo new_memo;
    char filepath[MAX_MEMO_FILE_PATH];
    get_memo_filepath(user_id, filepath);
    new_memo.id = get_next_memo_id(filepath);

    // 클라이언트에서 받은 제목을 그대로 사용
    strncpy(new_memo.title, title, MEMO_TITLE_MAX - 1);
    new_memo.title[MEMO_TITLE_MAX - 1] = '\0';

    strncpy(new_memo.content, content, MEMO_CONTENT_MAX - 1);
    new_memo.content[MEMO_CONTENT_MAX - 1] = '\0';

    time_t t = time(NULL);
    get_current_datetime(new_memo.created_at);
    strcpy(new_memo.updated_at, new_memo.created_at);

    FILE *file = fopen(filepath, "a");
    if (!file)
        return false;

    // 형식: id \t created_at \t updated_at \t title \t content
    fprintf(file, "%d\t%s\t%s\t%s\t%s\n",
            new_memo.id, new_memo.created_at, new_memo.updated_at,
            new_memo.title, new_memo.content);

    fclose(file);
    return true;
}

bool update_memo_for_user(const char *user_id, int memo_id, const char *new_content)
{
    char filepath[MAX_MEMO_FILE_PATH];
    get_memo_filepath(user_id, filepath);

    FILE *original_file = fopen(filepath, "r");
    if (!original_file)
        return false;

    FILE *temp_file = fopen(TEMP_MEMO_FILE, "w");
    if (!temp_file)
    {
        fclose(original_file);
        return false;
    }

    char line[sizeof(Memo)];
    bool updated = false;

    while (fgets(line, sizeof(line), original_file))
    {
        int id;
        sscanf(line, "%d", &id);
        if (id == memo_id)
        {
            Memo m;
            m.id = id;

            // 원본 줄에서 제목과 생성일자를 읽어와 보존하고, 제목 자동 생성을 비활성화
            char old_title[MEMO_TITLE_MAX];
            sscanf(line, "%*d\t%[^\t]\t%*[^\t]\t%[^\t]", m.created_at, old_title);

            strncpy(m.title, old_title, MEMO_TITLE_MAX - 1);
            m.title[MEMO_TITLE_MAX - 1] = '\0';

            strncpy(m.content, new_content, MEMO_CONTENT_MAX - 1);
            m.content[MEMO_CONTENT_MAX - 1] = '\0';

            get_current_datetime(m.updated_at);

            // 형식: id \t created_at \t updated_at \t title \t content
            fprintf(temp_file, "%d\t%s\t%s\t%s\t%s\n", m.id, m.created_at, m.updated_at, m.title, m.content);
            updated = true;
        }
        else
        {
            fputs(line, temp_file);
        }
    }
    fclose(original_file);
    fclose(temp_file);

    if (updated)
    {
        remove(filepath);
        rename(TEMP_MEMO_FILE, filepath);
    }
    else
    {
        remove(TEMP_MEMO_FILE);
    }

    return updated;
}

bool delete_memo_for_user(const char *user_id, int memo_id)
{
    char filepath[MAX_MEMO_FILE_PATH];
    get_memo_filepath(user_id, filepath);

    FILE *original_file = fopen(filepath, "r");
    if (!original_file)
        return false;

    FILE *temp_file = fopen(TEMP_MEMO_FILE, "w");
    if (!temp_file)
    {
        fclose(original_file);
        return false;
    }

    char line[sizeof(Memo)];
    bool deleted = false;
    while (fgets(line, sizeof(line), original_file))
    {
        int id;
        sscanf(line, "%d", &id);

        if (id == memo_id)
        {
            deleted = true;
            continue; // 이 라인을 건너뛰어 임시 파일에 쓰지 않음
        }
        fputs(line, temp_file);
    }

    fclose(original_file);
    fclose(temp_file);

    if (deleted)
    {
        remove(filepath);
        rename(TEMP_MEMO_FILE, filepath);
    }
    else
    {
        remove(TEMP_MEMO_FILE);
    }

    return deleted;
}

// 특정 메모의 원본 내용을 가져옵니다. (수정용)
bool get_raw_memo_content(const char *user_id, int memo_id, char *content_output, int content_size)
{
    char filepath[MAX_MEMO_FILE_PATH];
    get_memo_filepath(user_id, filepath);
    FILE *file = fopen(filepath, "r");
    if (!file)
    {
        return false;
    }

    char line[sizeof(Memo)];
    bool found = false;
    while (fgets(line, sizeof(line), file))
    {
        int id;
        sscanf(line, "%d", &id);
        if (id == memo_id)
        {
            // 형식: id \t created_at \t updated_at \t title \t content
            // content는 5번째 필드
            sscanf(line, "%*d\t%*[^\t]\t%*[^\t]\t%*[^\t]\t%[^\n]", content_output);
            found = true;
            break;
        }
    }
    fclose(file);
    return found;
}

// 특정 연월의 메모 목록을 가져옵니다.
bool list_memos_by_month(const char *user_id, int year, int month, char *output, int output_size)
{
    char filepath[MAX_MEMO_FILE_PATH];
    get_memo_filepath(user_id, filepath);
    FILE *file = fopen(filepath, "r");
    if (!file)
    {
        output[0] = '\0'; // 파일이 없으면 빈 문자열 반환
        return true;
    }

    int offset = 0;
    char line[sizeof(Memo) + 100]; // 버퍼 여유 공간 추가
    bool found = false;

    output[0] = '\0'; // 출력 버퍼 초기화

    while (fgets(line, sizeof(line), file))
    {
        int memo_year, memo_month;
        // sscanf가 2개의 값을 성공적으로 읽었는지 확인
        if (sscanf(line, "%*d\t%d-%d", &memo_year, &memo_month) == 2)
        {
            if (memo_year == year && memo_month == month)
            {
                // sscanf로 인한 멀티바이트 문자 깨짐 방지
                // 라인 전체를 그대로 클라이언트에 전송하여 파싱을 클라이언트에 위임
                // content 필드는 매우 클 수 있으므로, 제목까지만 잘라서 보냄

                char *p_content = strstr(line, "\t"); // id
                if (p_content)
                    p_content = strstr(p_content + 1, "\t"); // created_at
                if (p_content)
                    p_content = strstr(p_content + 1, "\t"); // updated_at
                if (p_content)
                    p_content = strstr(p_content + 1, "\t"); // title

                if (p_content)
                {
                    int len_to_send = (p_content - line);
                    if (offset + len_to_send + 1 < output_size)
                    {
                        strncat(output + offset, line, len_to_send);
                        strcat(output, "\n");
                        offset += len_to_send + 1;
                        found = true;
                    }
                }
            }
        }
    }

    fclose(file);
    return true; // 데이터를 찾았든 못찾았든 연산 자체는 성공
}
