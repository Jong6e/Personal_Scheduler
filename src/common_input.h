// src/common_input.h

#ifndef COMMON_INPUT_H
#define COMMON_INPUT_H

#include <stddef.h>
#include <stdbool.h>

// ✅ 화면을 초기화하는 함수
void clear_screen();

// ✅ 사용자 입력을 안전하게 받는 함수 (ESC 키로 취소 가능)
// - buffer: 입력을 저장할 버퍼
// - size: 버퍼 크기
// - label: 입력 필드 이름 (예: "ID 입력")
// - masked: true일 경우 입력을 '*'로 마스킹
// - 반환값: 입력 성공 시 true, 취소 시 false
bool get_escapable_input(char *buffer, size_t size, const char *label, bool masked);

#endif // COMMON_INPUT_H
