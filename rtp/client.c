#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SERVER_IP "127.0.0.1" 
#define SERVER_TCP_PORT  8888
#define CLIENT_RTP_PORT  5004
#define CLIENT_RTCP_PORT 5005

typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;

typedef struct {
    int sock;
    SOCKADDR_IN addr;
} args;


void* rtp_recv_thread(void* arg) {
    args *thread_args = (args*) arg;
    char buffer[2048]; 

    while (1) {
        SOCKADDR_IN saddr;
        int addr_len = sizeof(saddr);
        int received = recvfrom(thread_args->sock, buffer, sizeof(buffer), 0, (SOCKADDR*)&saddr, &addr_len);
        buffer[received] = 0;
        printf("RTP package from server: ");
        for (int i = 0; i < received; i++) {
            printf("%02X ", (unsigned char)buffer[i]);
        }
        printf("\n");
    } 
    return NULL;
}

void* rctp_recv_thread(void* arg) {
    args *thread_args = (args*) arg;
    unsigned char buffer[1500]; 

    while (1) {
        SOCKADDR_IN saddr;
        socklen_t addr_len = sizeof(saddr);
        int received = recvfrom(thread_args->sock, buffer, sizeof(buffer), 0, (SOCKADDR*)&saddr, &addr_len);
        buffer[received] = 0;
        printf("RCTP package from server: ");
        for (int i = 0; i < received; i++) {
            printf("%02X ", (unsigned char)buffer[i]);
        }
        printf("\n");
    }
    return NULL;
}

int main() {
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    SOCKADDR_IN saddr;
    memset(&saddr, 0, sizeof(saddr));

    saddr.sin_family = AF_INET;
    saddr.sin_port   = htons(SERVER_TCP_PORT);
    saddr.sin_addr.s_addr = inet_addr(SERVER_IP);


    int error = connect(s, (SOCKADDR*)&saddr, sizeof(saddr));
    if(error != 0) {
        return 1;
    }


    int rtp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);


    SOCKADDR_IN rtp_addr;
    memset(&rtp_addr, 0, sizeof(rtp_addr));
    rtp_addr.sin_family = AF_INET;
    rtp_addr.sin_addr.s_addr = inet_addr("0.0.0.0");  
    rtp_addr.sin_port = htons(CLIENT_RTP_PORT);

    error = bind(rtp_sock, (SOCKADDR*)&rtp_addr, sizeof(rtp_addr));
    if(error != 0) {
        return 1;
    }

    int rctp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    SOCKADDR_IN rctp_addr;
    memset(&rctp_addr, 0, sizeof(rctp_addr));
    rctp_addr.sin_family = AF_INET;
    rctp_addr.sin_addr.s_addr = INADDR_ANY;
    rctp_addr.sin_port = htons(CLIENT_RTCP_PORT);

    error = bind(rctp_sock, (SOCKADDR*)&rctp_addr, sizeof(rctp_addr));
    if(error != 0) {
        return 1;
    }
    

    pthread_t rtp_thread, rctp_thread;
    args rtp_args, rctp_args;

    rtp_args.sock = rtp_sock;
    rtp_args.addr = rtp_addr;

    rctp_args.sock = rctp_sock;
    rctp_args.addr = rctp_addr;

    pthread_create(&rtp_thread, NULL, rtp_recv_thread, (void*)&rtp_args);
    pthread_create(&rctp_thread, NULL, rctp_recv_thread, (void*)&rctp_args);


    while (1) {
        char input[64];
        printf("Enter command (\"exit\" to quit): ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }

        input[strlen(input) - 1] = 0;
        if(strlen(input) == 0) continue;

        send(s, input, strlen(input), 0);

        if (strcmp(input, "exit") == 0) {
            printf("Exiting...\n");
            break;
        }
    }

    close(s);
    pthread_cancel(rtp_thread);
    pthread_cancel(rctp_thread);
    pthread_join(rtp_thread, NULL);
    pthread_join(rctp_thread, NULL);
    close(rtp_sock);
    close(rctp_sock);

    return 0;
}
