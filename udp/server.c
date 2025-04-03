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
    int d = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (d < 0) {
        return 1;
    }

    SOCKADDR_IN saddr, caddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(8888);
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(d, (SOCKADDR*)&saddr, sizeof(saddr)) < 0) {
        close(d);
        return 1;
    }

    char buffer[1024] = {0};
    socklen_t addr_len = sizeof(caddr);
    int recived = recvfrom(d, buffer, sizeof(buffer) - 1, 0, (SOCKADDR*)&caddr, &addr_len);
    if (recived < 0) {
        close(d);
        return 1;
    }
    buffer[recived] = '\0';
    printf("Received: %s\n", buffer);

    char* welcome = "Hello\n";
    if (sendto(d, welcome, strlen(welcome), 0, (SOCKADDR*)&caddr, addr_len) < 0) {
        printf("sendto failed");
    }

    close(d);
    return 0;
}
