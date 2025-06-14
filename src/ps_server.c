// src/ps_server.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdbool.h>
#include "user.h"
#include "user_command.h"

#pragma comment(lib, "ws2_32.lib")

#define PORT 12345
#define BUF_SIZE 1024

// ✅ 클라이언트 연결과 통신을 처리하는 함수
void handle_client(SOCKET client_fd)
{
    printf("[서버] 클라이언트 연결됨 (소켓: %d)\n", (int)client_fd);
    char buffer[BUF_SIZE];

    while (true)
    {
        int bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0)
        {
            printf("[서버] 클라이언트 연결 해제됨 (소켓: %d)\n", (int)client_fd);
            break;
        }
        buffer[bytes] = '\0';
        printf("[서버] 수신: %s\n", buffer);

        char reply[BUF_SIZE] = {0};

        // TODO: 나중에 메모 관련 명령어가 추가되면 여기서 분기 처리
        handle_user_command(buffer, reply);

        printf("[서버] 응답: %s\n", reply);
        send(client_fd, reply, strlen(reply), 0);
    }
    closesocket(client_fd);
}

int main()
{
    SetConsoleOutputCP(CP_UTF8);

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
    {
        printf("WSAStartup 실패: %d\n", WSAGetLastError());
        return 1;
    }

    load_users_from_file();

    SOCKET server_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr = {0};
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    listen(server_fd, 5);
    printf("[서버] 클라이언트 접속 대기 중... (포트: %d)\n", PORT);

    while (true)
    {
        SOCKET client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == INVALID_SOCKET)
        {
            printf("[서버] 클라이언트 연결 수락 실패: %d\n", WSAGetLastError());
            continue;
        }
        // 실제 통신은 handle_client 함수에 위임
        handle_client(client_fd);
    }

    closesocket(server_fd);
    WSACleanup();
    return 0;
}
