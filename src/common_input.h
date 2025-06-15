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

// 사용자로부터 한 줄을 입력받습니다. (ESC로 취소 가능)
// 반환 타입을 bool로 변경하여 입력 성공/취소 여부를 알림
bool get_line_input(char *buffer, int buffer_size, const char *prompt);

// 단일 키 입력을 받아 유효성을 검사하는 메뉴 선택 전용 함수
char get_single_choice_input(const char *prompt, const char *valid_choices);

#endif // COMMON_INPUT_H
