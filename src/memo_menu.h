// src/memo_menu.h

#ifndef MEMO_MENU_H
#define MEMO_MENU_H

#include <winsock2.h>

//  로그인 후 메모 관리 메뉴를 표시하고 상호작용하는 메인 루프
//  logged_in_id: 현재 로그인된 사용자의 ID
void memo_menu_loop(SOCKET sock, const char *logged_in_id);

#endif
