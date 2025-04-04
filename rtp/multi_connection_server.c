#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

#define TCP_PORT 8888
#define UDP_PORT 9999
#define CLIENT_RTP_PORT 5004
#define CLIENT_RCTP_PORT 5005
#define RTP_VERSION 2
#define RTP_PAYLOAD_TYPE 96


typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;

typedef struct {
    unsigned int timestamp;
    unsigned int packet_count;
    unsigned int byte_count;
    pthread_mutex_t lock;
} rtp_stats;

typedef struct {
    int udp_sock;
    SOCKADDR_IN dest_addr;
    rtp_stats *stats;
} args;

typedef struct {
    unsigned int version:2;         
    unsigned int padding:1;
    unsigned int extension:1;
    unsigned int csrc_count:4;
    unsigned int marker:1;
    unsigned int payload_type:7;
    unsigned short sequence_number;
    unsigned int timestamp;
    unsigned int ssrc;
} rtp_header;

void *rtp_send(void* arg) {
    args *thread_arg = (args*) arg;
    int sock = thread_arg->udp_sock;
    SOCKADDR_IN addr = thread_arg->dest_addr;
    unsigned int sockaddr_size = sizeof(SOCKADDR);
    rtp_header header;
    memset(&header, 0, sizeof(header));
    header.version = RTP_VERSION;
    header.payload_type = RTP_PAYLOAD_TYPE;
    header.ssrc = htonl(0x12345678);
    unsigned short sequence = 0;
    unsigned int timestamp = 0;
    while(1) {
        header.sequence_number = htons(sequence);
        header.timestamp = htonl(timestamp);
        char *payload = "Hello from server\n";
        int package_size = strlen(payload) + sizeof(header);

        char *package = malloc(package_size);
        memcpy(package, &header, sizeof(header));
        memcpy(package + sizeof(header), payload, strlen(payload));

        int sent = sendto(sock, package, package_size, 0, (SOCKADDR*)&addr, sizeof(SOCKADDR));
        printf("send from pid %d\n", getpid());
        free(package);

        pthread_mutex_lock(&thread_arg->stats->lock);
        thread_arg->stats->timestamp = timestamp;
        thread_arg->stats->packet_count += 1;
        thread_arg->stats->byte_count += sent;
        pthread_mutex_unlock(&thread_arg->stats->lock);

        sequence++;
        timestamp++;
        sleep(1);
    }
    pthread_exit(NULL);
    
}

void *rctp_send(void* arg) {
    args *thread_args = (args*)arg;
    int sock = thread_args->udp_sock;
    SOCKADDR_IN addr = thread_args->dest_addr;

    while(1) {
        pthread_mutex_lock(&thread_args->stats->lock);
        unsigned int current_timestamp = thread_args->stats->timestamp;
        unsigned int packet_count = thread_args->stats->packet_count;
        unsigned int byte_count = thread_args->stats->byte_count;
        pthread_mutex_unlock(&thread_args->stats->lock);

        unsigned char rctp_package[28];
        memset(rctp_package, 0, sizeof(rctp_package));

        rctp_package[0] =  0x80;

        rctp_package[1] = 200;

        unsigned short length = htons(6);
        memcpy(&rctp_package[2], &length, sizeof(length));

        unsigned int ssrc = htonl(0x12345678);
        memcpy(&rctp_package[4], &ssrc, sizeof(ssrc));

        unsigned int ntp_dummy = htonl(0x00000001);
        memcpy(&rctp_package[8], &ntp_dummy, sizeof(ntp_dummy));    
        memcpy(&rctp_package[12], &ntp_dummy, sizeof(ntp_dummy)); 
        
        unsigned int rtp_timestamp_net = htonl(current_timestamp);
        memcpy(&rctp_package[16], &rtp_timestamp_net, sizeof(rtp_timestamp_net));

        unsigned int packet_count_net = htonl(packet_count);
        memcpy(&rctp_package[20], &packet_count_net, sizeof(packet_count_net));

        unsigned int byte_count_net = htonl(byte_count);
        memcpy(&rctp_package[24], &byte_count_net, sizeof(byte_count_net));

        int sent = sendto(sock, rctp_package, sizeof(rctp_package), 0, (SOCKADDR*)&addr, sizeof(SOCKADDR));

        printf("RCTP send from pid %d\n", getpid());
        sleep(5);
    }
    pthread_exit(NULL);
}



int main() {
    int s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    SOCKADDR_IN saddr, caddr;
    socklen_t addr_len = sizeof(SOCKADDR);

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(TCP_PORT);
    saddr.sin_addr.s_addr = inet_addr("0.0.0.0");

    int error = bind(s, (SOCKADDR*)&saddr, addr_len);
    if(error != 0) {
        return 1;
    }

    listen(s, 10);
    while(1) {
        int c = accept(s, (SOCKADDR*)&caddr, &addr_len);
        if(c < 0) {
            return 1;
        }
        
        int pid = fork();
        if(pid < 0) {
            return 1;
        }

        if(pid == 0) {
            close(s);
            rtp_stats stats;
            stats.timestamp = 0;
            stats.packet_count = 0;
            stats.byte_count = 0;
            pthread_mutex_init(&stats.lock, NULL);

            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &caddr.sin_addr.s_addr, client_ip, INET_ADDRSTRLEN);
        
            SOCKADDR_IN rtp_addr, rctp_addr;
        
            int rtp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            memset(&rtp_addr, 0, sizeof(SOCKADDR));
            rtp_addr.sin_family = AF_INET;
            rtp_addr.sin_port = htons(CLIENT_RTP_PORT);
            inet_pton(AF_INET, client_ip, &rtp_addr.sin_addr.s_addr) ;
        
            int rctp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            memset(&rctp_addr, 0, sizeof(SOCKADDR));
            rctp_addr.sin_family = AF_INET;
            rctp_addr.sin_port = htons(CLIENT_RCTP_PORT);
            inet_pton(AF_INET, client_ip, &rctp_addr.sin_addr.s_addr);
        
            pthread_t rtp_thread, rctp_thread;
            args rtp_args, rctp_args;
        
            rtp_args.udp_sock = rtp_socket;
            rtp_args.dest_addr = rtp_addr;
            rtp_args.stats = &stats;
            pthread_create(&rtp_thread, NULL, rtp_send, (void*)&rtp_args);
        
            rctp_args.udp_sock = rctp_socket;
            rctp_args.dest_addr = rctp_addr;
            rctp_args.stats = &stats;
            pthread_create(&rctp_thread, NULL, rctp_send, (void*)&rctp_args);
        
            while(1) {
                char buffer[16] = {0};
                int recived = recv(c, buffer, sizeof(buffer), 0);
                if(recived == 0) break;
                buffer[recived] = 0;
                printf("%s\n", buffer);
                if(strcmp(buffer, "exit") == 0) {
                    break;
                }
            }
            close(c);
            pthread_mutex_destroy(&stats.lock);
            pthread_cancel(rtp_thread);
            pthread_cancel(rctp_thread);
            pthread_join(rtp_thread, NULL);
            pthread_join(rctp_thread, NULL);
            close(rtp_socket);
            close(rctp_socket);
            printf("pid %d huy\n", getpid());
            return 0;
        } else {
            printf("pid goc\n");
            close(c);
        }
    }
}