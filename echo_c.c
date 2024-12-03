#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

// Enum to represent protocol type
typedef enum {
    TCP,
    UDP
} ProtocolType;

void print_usage(char *program_name) {
    fprintf(stderr, "Usage: %s <server_ip> <port> <protocol>\n", program_name);
    fprintf(stderr, "Protocol can be 'tcp' or 'udp'\n");
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        print_usage(argv[0]);
        return 1;
    }

    char *server_ip = argv[1];
    int port = atoi(argv[2]);
    ProtocolType protocol;

    // Parse protocol argument
    if (strcmp(argv[3], "tcp") == 0) {
        protocol = TCP;
    } else if (strcmp(argv[3], "udp") == 0) {
        protocol = UDP;
    } else {
        print_usage(argv[0]);
        return 1;
    }

    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // Create socket based on protocol
    if (protocol == TCP) {
        // 1. Create a TCP socket
        sock = socket(AF_INET, SOCK_STREAM, 0);
    } else {
        // 1. Create a UDP socket
        sock = socket(AF_INET, SOCK_DGRAM, 0);
    }

    if (sock < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // 2. Set up server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    // Connect for TCP, no connection needed for UDP
    if (protocol == TCP) {
        if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Connection failed");
            return 1;
        }
        printf("Connected to Server at %s: %d (TCP)\n", server_ip, port);
    }

    // 3. Send and receive messages
    while (1) {
        printf("Enter message (or 'exit' to quit): ");
        fgets(buffer, sizeof(buffer), stdin);

        // Remove newline character from the message
        buffer[strcspn(buffer, "\n")] = '\0';

        if (strcmp(buffer, "exit") == 0) {
            break;
        }

        if (protocol == TCP) {
            // TCP: use send and read
            send(sock, buffer, strlen(buffer), 0);
            memset(buffer, 0, sizeof(buffer));
            read(sock, buffer, sizeof(buffer));
            printf("Server: %s\n", buffer);
        } else {
            // UDP: use sendto and recvfrom
            socklen_t server_addr_len = sizeof(server_addr);
            sendto(sock, buffer, strlen(buffer), 0, 
                   (struct sockaddr *)&server_addr, server_addr_len);
            
            memset(buffer, 0, sizeof(buffer));
            recvfrom(sock, buffer, sizeof(buffer), 0, 
                     (struct sockaddr *)&server_addr, &server_addr_len);
            printf("Server: %s\n", buffer);
        }
    }

    close(sock);
    return 0;
}