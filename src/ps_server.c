// src/ps_server.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdbool.h>
#include <locale.h>
#include "user.h"
#include "user_command.h"
#include "memo.h"
#include "memo_command.h"
#include <signal.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 12345
#define BUF_SIZE 2048
#define MAX_CLIENTS 100

// 전역 서버 소켓
static SOCKET g_serv_sock = INVALID_SOCKET;

// 클라이언트 연결과 통신 처리
DWORD WINAPI handle_client(LPVOID client_socket);

// Ctrl+C 신호 처리
void signal_handler(int signum)
{
    if (signum == SIGINT)
    {
        printf("\n[서버] 종료 신호(Ctrl+C) 수신. 서버를 안전하게 종료합니다...\n");

        // 데이터 파일에 저장
        printf("[서버] 모든 사용자 정보를 파일에 저장 중...\n");
        user_save_to_file();
        printf("[서버] 모든 메모 정보를 파일에 저장 중...\n");
        memo_save_all_to_files();

        // 리소스 정리
        if (g_serv_sock != INVALID_SOCKET)
        {
            closesocket(g_serv_sock);
        }
        user_cleanup();
        memo_cleanup();
        WSACleanup();

        printf("[서버] 모든 리소스가 정리되었습니다. 프로그램을 종료합니다.\n");
        exit(0);
    }
}

int main()
{
    setlocale(LC_ALL, ".UTF8");
    SetConsoleOutputCP(CP_UTF8);

    // 시그널 핸들러 등록
    signal(SIGINT, signal_handler);

    user_init();
    memo_init();

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("WSAStartup 실패: %d\n", WSAGetLastError());
        return 1;
    }
    // 서버 소켓 생성
    g_serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);

    // 서버 소켓 바인딩
    bind(g_serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    listen(g_serv_sock, MAX_CLIENTS);

    printf("[서버] 클라이언트 연결 대기 중... (Ctrl+C로 종료)\n");

    // 클라이언트 연결 대기
    while (true)
    {
        struct sockaddr_in client_addr;
        int client_addr_size = sizeof(client_addr);
        SOCKET client_sock = accept(g_serv_sock, (struct sockaddr *)&client_addr, &client_addr_size);

        if (client_sock == INVALID_SOCKET)
        {
            // 시그널 핸들러에서 소켓이 닫힌 경우 accept가 실패할 수 있음
            if (WSAGetLastError() == WSAEINTR)
            {
                break; // 루프 종료
            }
            printf("[서버] accept() 실패\n");
            continue;
        }

        // 클라이언트 처리 스레드 생성
        CreateThread(NULL, 0, handle_client, (LPVOID)client_sock, 0, NULL);
    }

    // 이 코드는 시그널 핸들러가 정상 작동하면 도달하지 않지만,
    // 만약을 대비한 최종 정리 코드
    closesocket(g_serv_sock);
    user_cleanup();
    memo_cleanup();
    WSACleanup();
    return 0;
}

// 클라이언트 처리 스레드
DWORD WINAPI handle_client(LPVOID client_socket)
{
    SOCKET sock = (SOCKET)(ULONG_PTR)client_socket;
    char buffer[BUF_SIZE], reply[BUF_SIZE];

    printf("[서버] 클라이언트 %llu 연결됨\n", sock);

    // 클라이언트 연결 유지
    while (true)
    {
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0)
        {
            printf("[서버] 클라이언트 %llu 연결 해제됨\n", sock);
            break;
        }
        buffer[bytes] = '\0';
        printf("[서버] 클라이언트 %llu 수신: %s\n", sock, buffer);

        // 명령어에 따라 핸들러 분기
        if (strncmp(buffer, "MEMO_", 5) == 0)
        {
            handle_memo_command(buffer, reply, sizeof(reply));
        }
        else
        {
            handle_user_command(buffer, reply, sizeof(reply));
        }

        // 응답 전송
        send(sock, reply, strlen(reply), 0);
    }

    closesocket(sock);
    return 0;
}
