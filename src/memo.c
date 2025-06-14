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

// 메모 내용에서 제목을 추출합니다. (앞 10글자 + "...")
static void generate_title_from_content(const char *content, char *title)
{
    strncpy(title, content, 10);
    if (strlen(content) > 10)
    {
        strcpy(title + 10, "...");
    }
    else
    {
        title[strlen(content)] = '\0';
    }
    // 제목에 포함될 수 있는 개행문자 제거
    char *p = strchr(title, '\n');
    if (p)
        *p = '\0';
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
        snprintf(output, output_size, "메모를 찾을 수 없습니다.");
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
            char title[MEMO_TITLE_MAX], content[MEMO_CONTENT_MAX], created_at[DATETIME_MAX], updated_at[DATETIME_MAX];
            // 형식: id \t created_at \t updated_at \t title \t content
            sscanf(line, "%d\t%[^\t]\t%[^\t]\t%[^\t]\t%[^\n]", &id, created_at, updated_at, title, content);
            snprintf(output, output_size,
                     "ID: %d\n"
                     "제목: %s\n"
                     "작성일: %s\n"
                     "수정일: %s\n"
                     "---------------------------------\n"
                     "%s\n",
                     id, title, created_at, updated_at, content);
            found = true;
            break;
        }
    }
    fclose(file);
    if (!found)
    {
        snprintf(output, output_size, "메모 ID %d를 찾을 수 없습니다.", memo_id);
    }
    return found;
}

bool add_memo_for_user(const char *user_id, const char *title, const char *content)
{
    char filepath[MAX_MEMO_FILE_PATH];
    get_memo_filepath(user_id, filepath);

    FILE *file = fopen(filepath, "a");
    if (!file)
        return false;

    Memo new_memo;
    new_memo.id = get_next_memo_id(filepath);

    // 제목 처리: title이 제공되고 비어있지 않으면 사용, 아니면 content에서 생성
    if (title && strlen(title) > 0)
    {
        strncpy(new_memo.title, title, MEMO_TITLE_MAX - 1);
        new_memo.title[MEMO_TITLE_MAX - 1] = '\0';
    }
    else
    {
        generate_title_from_content(content, new_memo.title);
    }

    strncpy(new_memo.content, content, MEMO_CONTENT_MAX - 1);
    new_memo.content[MEMO_CONTENT_MAX - 1] = '\0';
    get_current_datetime(new_memo.created_at);
    strcpy(new_memo.updated_at, new_memo.created_at);

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
            generate_title_from_content(new_content, m.title);
            strncpy(m.content, new_content, MEMO_CONTENT_MAX - 1);
            m.content[MEMO_CONTENT_MAX - 1] = '\0';

            // 형식: id \t created_at \t ... -> created_at은 두 번째 필드
            sscanf(line, "%*d\t%[^\t]", m.created_at);
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
