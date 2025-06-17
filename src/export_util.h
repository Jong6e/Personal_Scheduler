// src/export_util.h

#ifndef EXPORT_UTIL_H
#define EXPORT_UTIL_H

#include "memo.h"

// 단일 메모를 지정된 형식의 문자열로 변환
// 성공 시 동적으로 할당된 문자열 포인터 반환, 실패 시 NULL 반환
// 호출자는 반환된 문자열을 free() 해주어야 함
char *export_single_memo_to_string(const Memo *memo, const char *format);

// 사용자의 모든 메모 목록을 지정된 형식의 문자열로 변환
// 성공 시 동적으로 할당된 문자열 포인터 반환, 실패 시 NULL 반환
// 호출자는 반환된 문자열을 free() 해주어야 함
char *export_all_memos_to_string(const Memo *memo_list, int memo_count, const char *format);

#endif
