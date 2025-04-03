#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

int main() {
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKADDR_IN saddr, caddr;
    socklen_t sockLen = sizeof(SOCKADDR);
    char buffer[1024];

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(8888);
    saddr.sin_addr.s_addr = inet_addr("0.0.0.0");

    int error = bind(s, (SOCKADDR*)&saddr, sockLen);

    if(error == 0) {
        listen(s, 10);
        int c = accept(s, (SOCKADDR*)&caddr, &sockLen);

        if(c >= 0) {
            char* welcome = "Hello\n";
            int sent = send(c, welcome, strlen(welcome), 0);
            printf("%d bytes sent\n", sent);

            memset(buffer, 0, sizeof(buffer));
            int recived = recv(c, buffer, sizeof(buffer) - 1, 0);
            buffer[recived] = 0;
            printf("Recived: %s\n", buffer);

            close(s);
            close(c);
        }
    }
}