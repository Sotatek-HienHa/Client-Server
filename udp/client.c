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

    SOCKADDR_IN saddr;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(8888);
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    SOCKADDR_IN caddr;
    caddr.sin_family = AF_INET;
    caddr.sin_port = htons(9999);  
    caddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (bind(d, (SOCKADDR*)&caddr, sizeof(caddr)) < 0) {
        close(d);
        return 1;
    }

    char* message = "Xin chao server\n";
    if (sendto(d, message, strlen(message), 0, (SOCKADDR*)&saddr, sizeof(saddr)) < 0) {
        close(d);
        return 1;
    }

    char buffer[1024] = {0};
    socklen_t addr_len = sizeof(saddr);
    int ret = recvfrom(d, buffer, sizeof(buffer) - 1, 0, (SOCKADDR*)&saddr, &addr_len);
    if (ret < 0) {
        close(d);
        return 1;
    }
    buffer[ret] = '\0';
    printf("Received from server: %s\n", buffer);

    close(d);
    return 0;
}
