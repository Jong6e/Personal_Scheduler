// src/export_util.c

#include "export_util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>

// 내부 헬퍼 함수 선언
static char *format_memo_as_md(const Memo *memo);
static char *format_memo_as_txt(const Memo *memo);
static char *format_memo_as_json(const Memo *memo);
static char *format_memo_as_xml(const Memo *memo);

// XML 특수문자 이스케이프 처리
static void escape_xml_chars(const char *input, char *output, int out_size)
{
    int i = 0, j = 0;
    while (input[i] != '\0' && j < out_size - 6)
    {
        switch (input[i])
        {
        case '<':
            strcpy_s(output + j, out_size - j, "&lt;");
            j += 4;
            break;
        case '>':
            strcpy_s(output + j, out_size - j, "&gt;");
            j += 4;
            break;
        case '&':
            strcpy_s(output + j, out_size - j, "&amp;");
            j += 5;
            break;
        case '\"':
            strcpy_s(output + j, out_size - j, "&quot;");
            j += 6;
            break;
        case '\'':
            strcpy_s(output + j, out_size - j, "&apos;");
            j += 6;
            break;
        default:
            output[j++] = input[i];
            break;
        }
        i++;
    }
    output[j] = '\0';
}

// JSON 특수문자 이스케이프 처리
static void escape_json_chars(const char *input, char *output, int out_size)
{
    int i = 0, j = 0;
    while (input[i] != '\0' && j < out_size - 2)
    {
        switch (input[i])
        {
        case '\"':
            output[j++] = '\\';
            output[j++] = '\"';
            break;
        case '\\':
            output[j++] = '\\';
            output[j++] = '\\';
            break;
        case '\b':
            output[j++] = '\\';
            output[j++] = 'b';
            break;
        case '\f':
            output[j++] = '\\';
            output[j++] = 'f';
            break;
        case '\n':
            output[j++] = '\\';
            output[j++] = 'n';
            break;
        case '\r':
            output[j++] = '\\';
            output[j++] = 'r';
            break;
        case '\t':
            output[j++] = '\\';
            output[j++] = 't';
            break;
        default:
            output[j++] = input[i];
            break;
        }
        i++;
    }
    output[j] = '\0';
}

// CSV 특수문자 이스케이프 처리
static void escape_csv_chars(const char *input, char *output, int out_size)
{
    // 필드에 쉼표, 큰따옴표, 또는 줄바꿈 문자가 포함되어 있는지 확인
    bool needs_quoting = (strchr(input, ',') != NULL || strchr(input, '\"') != NULL || strchr(input, '\n') != NULL);

    if (!needs_quoting)
    {
        // 특별한 처리가 필요 없으면 그대로 복사
        strcpy_s(output, out_size, input);
        return;
    }

    // 필드를 큰따옴표로 감싸고, 내부의 큰따옴표는 두 개로 만듦
    int i = 0, j = 0;
    output[j++] = '\"';
    while (input[i] != '\0' && j < out_size - 2)
    {
        if (input[i] == '\"')
        {
            output[j++] = '\"'; // 큰따옴표를 두 개로 이스케이프
        }
        output[j++] = input[i++];
    }
    output[j++] = '\"';
    output[j] = '\0';
}

// 단일 메모 변환 함수 구현
char *export_single_memo_to_string(const Memo *memo, const char *format)
{
    // MD 포맷
    if (strcmp(format, "MD") == 0)
    {
        return format_memo_as_md(memo);
    }
    // 텍스트 포맷
    if (strcmp(format, "TXT") == 0)
    {
        return format_memo_as_txt(memo);
    }
    // JSON 포맷
    if (strcmp(format, "JSON") == 0)
    {
        return format_memo_as_json(memo);
    }
    // CSV 포맷
    if (strcmp(format, "CSV") == 0)
    {
        char buffer[2048];
        char escaped_title[MAX_MEMO_TITLE_LEN * 2 + 3];
        char escaped_content[MAX_MEMO_CONTENT_LEN * 2 + 3];
        escape_csv_chars(memo->title, escaped_title, sizeof(escaped_title));
        escape_csv_chars(memo->content, escaped_content, sizeof(escaped_content));

        // 헤더와 데이터를 함께 생성 + BOM 추가
        snprintf(buffer, sizeof(buffer),
                 "\xEF\xBB\xBF" // UTF-8 BOM for Excel
                 "id,title,content,created_at,updated_at\n"
                 "%d,\"%s\",\"%s\",\"=\"\"%s\"\"\",\"=\"\"%s\"\"\"\n", // 날짜/시간을 텍스트로 강제
                 memo->id, escaped_title, escaped_content, memo->created_at, memo->updated_at);
        return strdup(buffer);
    }
    // XML 포맷
    if (strcmp(format, "XML") == 0)
    {
        return format_memo_as_xml(memo);
    }
    return NULL;
}

// 전체 메모 변환 함수 구현 (구조적 결함 수정)
char *export_all_memos_to_string(const Memo *memo_list, int memo_count, const char *format)
{
    size_t buffer_size = memo_count * (MAX_MEMO_CONTENT_LEN + 512) + 1024; // 여유있는 버퍼 크기
    char *result_buffer = (char *)malloc(buffer_size);
    if (result_buffer == NULL)
        return NULL;
    result_buffer[0] = '\0';
    size_t offset = 0;

    // CSV 포맷
    if (strcmp(format, "CSV") == 0)
    {
        // BOM 추가 (Excel 호환을 위해)
        offset += snprintf(result_buffer + offset, buffer_size - offset, "\xEF\xBB\xBF");
        // 헤더 추가
        offset += snprintf(result_buffer + offset, buffer_size - offset, "id,title,content,created_at,updated_at\n");
        for (int i = 0; i < memo_count; i++)
        {
            char escaped_title[MAX_MEMO_TITLE_LEN * 2 + 3];
            char escaped_content[MAX_MEMO_CONTENT_LEN * 2 + 3];
            escape_csv_chars(memo_list[i].title, escaped_title, sizeof(escaped_title));
            escape_csv_chars(memo_list[i].content, escaped_content, sizeof(escaped_content));
            offset += snprintf(result_buffer + offset, buffer_size - offset, "%d,\"%s\",\"%s\",\"=\"\"%s\"\"\",\"=\"\"%s\"\"\"\n", // 날짜/시간을 텍스트로 강제
                               memo_list[i].id, escaped_title, escaped_content, memo_list[i].created_at, memo_list[i].updated_at);
        }
    }
    // JSON 포맷
    else if (strcmp(format, "JSON") == 0)
    {
        offset += snprintf(result_buffer + offset, buffer_size - offset, "[\n");
        for (int i = 0; i < memo_count; i++)
        {
            char escaped_title[MAX_MEMO_TITLE_LEN * 6];
            char escaped_content[MAX_MEMO_CONTENT_LEN * 6];
            escape_json_chars(memo_list[i].title, escaped_title, sizeof(escaped_title));
            escape_json_chars(memo_list[i].content, escaped_content, sizeof(escaped_content));
            offset += snprintf(result_buffer + offset, buffer_size - offset, "  {\"id\":%d,\"title\":\"%s\",\"content\":\"%s\",\"created_at\":\"%s\",\"updated_at\":\"%s\"}",
                               memo_list[i].id, escaped_title, escaped_content, memo_list[i].created_at, memo_list[i].updated_at);
            if (i < memo_count - 1)
            {
                offset += snprintf(result_buffer + offset, buffer_size - offset, ",\n");
            }
        }
        offset += snprintf(result_buffer + offset, buffer_size - offset, "\n]");
    }
    // XML 포맷
    else if (strcmp(format, "XML") == 0)
    {
        offset += snprintf(result_buffer + offset, buffer_size - offset, "<memos>\n");
        for (int i = 0; i < memo_count; i++)
        {
            char escaped_title[MAX_MEMO_TITLE_LEN * 6];
            char escaped_content[MAX_MEMO_CONTENT_LEN * 6];
            escape_xml_chars(memo_list[i].title, escaped_title, sizeof(escaped_title));
            escape_xml_chars(memo_list[i].content, escaped_content, sizeof(escaped_content));
            offset += snprintf(result_buffer + offset, buffer_size - offset, "  <memo>\n    <id>%d</id>\n    <title>%s</title>\n    <content>%s</content>\n    <created_at>%s</created_at>\n    <updated_at>%s</updated_at>\n  </memo>\n",
                               memo_list[i].id, escaped_title, escaped_content, memo_list[i].created_at, memo_list[i].updated_at);
        }
        offset += snprintf(result_buffer + offset, buffer_size - offset, "</memos>");
    }
    // MD & TXT 포맷
    else
    {
        for (int i = 0; i < memo_count; i++)
        {
            char *single_memo_str = export_single_memo_to_string(&memo_list[i], format);
            if (single_memo_str)
            {
                offset += snprintf(result_buffer + offset, buffer_size - offset, "%s", single_memo_str);
                if (strcmp(format, "MD") == 0)
                {
                    offset += snprintf(result_buffer + offset, buffer_size - offset, "\n---\n\n");
                }
                else if (strcmp(format, "TXT") == 0)
                {
                    offset += snprintf(result_buffer + offset, buffer_size - offset, "====================\n\n");
                }
                free(single_memo_str);
            }
        }
    }

    return result_buffer;
}

// 각 포맷별 실제 변환 로직

// MD 포맷
static char *format_memo_as_md(const Memo *memo)
{
    char buffer[2048];
    snprintf(buffer, sizeof(buffer),
             "---\nid: %d\ncreated_at: %s\nupdated_at: %s\n---\n\n# %s\n\n%s\n\n",
             memo->id, memo->created_at, memo->updated_at, memo->title, memo->content);
    return strdup(buffer);
}

// TXT 포맷
static char *format_memo_as_txt(const Memo *memo)
{
    char buffer[2048];
    snprintf(buffer, sizeof(buffer),
             "ID: %d\n작성일시: %s\n수정일시: %s\n제목: %s\n\n--------------------\n%s\n\n",
             memo->id, memo->created_at, memo->updated_at, memo->title, memo->content);
    return strdup(buffer);
}

// JSON 포맷
static char *format_memo_as_json(const Memo *memo)
{
    char escaped_title[MAX_MEMO_TITLE_LEN * 6];
    char escaped_content[MAX_MEMO_CONTENT_LEN * 6];
    escape_json_chars(memo->title, escaped_title, sizeof(escaped_title));
    escape_json_chars(memo->content, escaped_content, sizeof(escaped_content));

    char buffer[2048];
    snprintf(buffer, sizeof(buffer),
             "[\n"
             "  {\n"
             "    \"id\": %d,\n"
             "    \"title\": \"%s\",\n"
             "    \"content\": \"%s\",\n"
             "    \"created_at\": \"%s\",\n"
             "    \"updated_at\": \"%s\"\n"
             "  }\n"
             "]",
             memo->id, escaped_title, escaped_content, memo->created_at, memo->updated_at);
    return strdup(buffer);
}

// XML 포맷
static char *format_memo_as_xml(const Memo *memo)
{
    char escaped_title[MAX_MEMO_TITLE_LEN * 6];
    char escaped_content[MAX_MEMO_CONTENT_LEN * 6];
    escape_xml_chars(memo->title, escaped_title, sizeof(escaped_title));
    escape_xml_chars(memo->content, escaped_content, sizeof(escaped_content));

    char buffer[2048];
    snprintf(buffer, sizeof(buffer),
             "<memos>\n"
             "  <memo>\n"
             "    <id>%d</id>\n"
             "    <title>%s</title>\n"
             "    <content>%s</content>\n"
             "    <created_at>%s</created_at>\n"
             "    <updated_at>%s</updated_at>\n"
             "  </memo>\n"
             "</memos>",
             memo->id, escaped_title, escaped_content, memo->created_at, memo->updated_at);
    return strdup(buffer);
}
