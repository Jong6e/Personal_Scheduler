// src/common_input.h

#ifndef COMMON_INPUT_H
#define COMMON_INPUT_H

#include <stdbool.h>
#include <winsock2.h>

// 콘솔 화면을 깨끗하게 지웁니다.
void clear_screen();

// ESC로 취소 가능한, 유효성이 검증된 단일 라인 입력을 받는 함수.
// is_password: true이면 입력 문자를 '*'로 마스킹합니다.
// alphanumeric_only: true이면 영문/숫자만 입력을 허용합니다.
bool get_validated_input(char *buffer, int buffer_size, const char *prompt, bool is_password, bool alphanumeric_only);

// 한글을 포함한 자유로운 텍스트 한 줄을 입력받는 함수입니다.
// (fgets 기반이므로 ESC 실시간 취소는 지원하지 않습니다)
void get_line_input(char *buffer, int buffer_size, const char *prompt);

#endif // COMMON_INPUT_H
