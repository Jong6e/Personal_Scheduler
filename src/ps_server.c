// src/ps_server.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdbool.h>
#include "user.h"
#include "user_command.h"
#include "memo.h"
#include "memo_command.h"

#pragma comment(lib, "ws2_32.lib")

#define PORT 12345
#define BUF_SIZE 1024
#define MAX_CLIENTS 10

// 클라이언트 연결과 통신을 처리하는 함수
DWORD WINAPI handle_client(LPVOID client_socket);

int main()
{
    SetConsoleOutputCP(CP_UTF8);

    load_users_from_file();

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("WSAStartup 실패: %d\n", WSAGetLastError());
        return 1;
    }

    SOCKET serv_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);

    bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    listen(serv_sock, MAX_CLIENTS);

    printf("[서버] 클라이언트 연결 대기 중...\n");

    while (true)
    {
        struct sockaddr_in client_addr;
        int client_addr_size = sizeof(client_addr);
        SOCKET client_sock = accept(serv_sock, (struct sockaddr *)&client_addr, &client_addr_size);

        if (client_sock == INVALID_SOCKET)
        {
            printf("[서버] accept() 실패\n");
            continue;
        }

        CreateThread(NULL, 0, handle_client, (LPVOID)client_sock, 0, NULL);
    }

    closesocket(serv_sock);
    WSACleanup();
    return 0;
}

DWORD WINAPI handle_client(LPVOID client_socket)
{
    SOCKET sock = (SOCKET)(ULONG_PTR)client_socket;
    char buffer[1024 * 8], reply[1024 * 64]; // request 8KB, reply 64KB

    printf("[서버] 클라이언트 %llu 연결됨\n", sock);

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

        send(sock, reply, strlen(reply), 0);
    }

    closesocket(sock);
    return 0;
}
