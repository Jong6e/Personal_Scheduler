// src/common_input.h

#ifndef COMMON_INPUT_H
#define COMMON_INPUT_H

#include <stdbool.h>
#include <winsock2.h>

#define KEY_ESC 27
#define KEY_ENTER 13
#define KEY_BACKSPACE 8

// 콘솔 화면을 깨끗하게 지우는 함수
void clear_screen();

// ESC/백스페이스 지원, 유효성 검증이 포함된 입력을 받는 핵심 함수
// is_password: true이면 입력 문자를 '*'로 마스킹
// alphanumeric_only: true이면 영문/숫자만 입력을 허용
bool get_secure_input(char *buffer, int buffer_size, const char *prompt, bool is_password, bool alphanumeric_only);

// 사용자로부터 한 줄의 텍스트를 입력받는 함수 (ESC로 취소 가능, 백스페이스 지원)
bool get_line_input(char *buffer, int buffer_size, const char *prompt);

// 메뉴 선택을 위한 단일 문자 입력을 받는 함수 (ESC로 취소 가능)
char get_single_choice_input(const char *prompt, const char *valid_choices);

#endif
