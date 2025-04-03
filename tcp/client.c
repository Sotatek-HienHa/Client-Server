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
    int c = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKADDR_IN caddr, saddr;
    char buffer[1024];
    socklen_t sockLen = sizeof(SOCKADDR);

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(8888);
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    int error = connect(c, (SOCKADDR*)&saddr, sockLen);
    if(error == 0) {

        int recived = recv(c, buffer, sizeof(buffer), 0);
        buffer[recived] = 0;
        printf("recived from server: %s\n", buffer);

        char* sendToServer = "Hi Server\n";
        int sent = send(c, sendToServer, strlen(sendToServer), 0);
        printf("sent %d bytes to server\n", sent);


        close(c);
    }
}