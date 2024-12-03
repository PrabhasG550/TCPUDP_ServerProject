#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <signal.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 30


typedef struct {
    int tcp_sock;
    int udp_sock;
} ServerSockets;

void send_log_message(const struct sockaddr_in *log_server_addr, 
                     const char *buffer, const char *client_ip) {
    // Get the current time
    time_t rawtime;
    struct tm *timeinfo;
    char timestamp[20]; // Format: YYYY-MM-DD HH:MM:SS
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);

    // Format the message
    char log_message[2048];
    snprintf(log_message, sizeof(log_message), "%s \"%s\" was received from %s", 
             timestamp, buffer, client_ip);

    // Send the formatted log message to the Log Server
    int log_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (log_sock >= 0) {
        sendto(log_sock, log_message, strlen(log_message), 0,
               (struct sockaddr *)log_server_addr, sizeof(struct sockaddr_in));
        close(log_sock);
    }
}

void cleanExit() {
    printf("\nClean Exit - Removing Port Bindings...\n");
    exit(0);  // Exit immediately without cleaning up resources
}

int setup_tcp_socket(int port) {
    int tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sock < 0) {
        perror("TCP Socket creation failed");
        return -1;
    }

    // Set socket option to reuse address
    int optval = 1;
    if (setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("setsockopt failed");
        close(tcp_sock);
        return -1;
    }

    // Bind the socket
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(tcp_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("TCP Bind failed");
        close(tcp_sock);
        return -1;
    }

    // Start listening
    if (listen(tcp_sock, 5) < 0) {
        perror("TCP Listen failed");
        close(tcp_sock);
        return -1;
    }

    return tcp_sock;
}

int setup_udp_socket(int port) {
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        perror("UDP Socket creation failed");
        return -1;
    }

    // Set socket option to reuse address
    int optval = 1;
    if (setsockopt(udp_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
        perror("setsockopt failed");
        close(udp_sock);
        return -1;
    }

    // Bind the socket
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(udp_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("UDP Bind failed");
        close(udp_sock);
        return -1;
    }

    return udp_sock;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    
    // Setup TCP and UDP sockets
    ServerSockets server_socks;
    server_socks.tcp_sock = setup_tcp_socket(port);
    server_socks.udp_sock = setup_udp_socket(port);

    if (server_socks.tcp_sock < 0 || server_socks.udp_sock < 0) {
        printf("Failed to setup server sockets\n");
        return 1;
    }

    // Prepare log server address (using same port for simplicity)
    struct sockaddr_in log_server_addr;
    log_server_addr.sin_family = AF_INET;
    log_server_addr.sin_addr.s_addr = INADDR_ANY; 
    log_server_addr.sin_port = htons(9949); // Log server port from previous implementation

    // Client sockets array
    int client_sockets[MAX_CLIENTS] = {0};
    fd_set readfds;
    int max_sd, activity, new_socket, sd;
    char buffer[BUFFER_SIZE];
    
    // Register signal handlers for SIGINT and SIGTERM
    signal(SIGINT, cleanExit);  // For Ctrl+C
    signal(SIGTERM, cleanExit); // For termination signals

    printf("Echo Server is running on port %d...\n", port);

    while (1) {
        // Clear the socket set
        FD_ZERO(&readfds);

        // Add TCP listening socket to set
        FD_SET(server_socks.tcp_sock, &readfds);
        max_sd = server_socks.tcp_sock;

        // Add UDP socket to set
        FD_SET(server_socks.udp_sock, &readfds);
        if (server_socks.udp_sock > max_sd)
            max_sd = server_socks.udp_sock;

        // Add child sockets to set
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];
            if (sd > 0)
                FD_SET(sd, &readfds);
            if (sd > max_sd)
                max_sd = sd;
        }

        // Wait for activity on one of the sockets
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0) {
            perror("select error");
            continue;
        }

        // UDP socket activity
        if (FD_ISSET(server_socks.udp_sock, &readfds)) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            memset(buffer, 0, sizeof(buffer));

            // Receive UDP message
            int bytes_received = recvfrom(server_socks.udp_sock, buffer, sizeof(buffer), 0, 
                                          (struct sockaddr *)&client_addr, &addr_len);
            
            if (bytes_received > 0) {
                // Get client IP
                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

                // Send log message
                send_log_message(&log_server_addr, buffer, client_ip);

                // Echo back to client
                sendto(server_socks.udp_sock, buffer, bytes_received, 0, 
                       (struct sockaddr *)&client_addr, addr_len);
            }
        }

        // TCP listening socket activity (new connection)
        if (FD_ISSET(server_socks.tcp_sock, &readfds)) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);

            // Accept new connection
            new_socket = accept(server_socks.tcp_sock, 
                                (struct sockaddr *)&client_addr, &addr_len);

            if (new_socket < 0) {
                perror("Accept failed");
                continue;
            }

            // Add new socket to array of sockets
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }

        // Handle existing TCP client sockets
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sd = client_sockets[i];
            
            if (FD_ISSET(sd, &readfds)) {
                // Receive message from client
                memset(buffer, 0, sizeof(buffer));
                int bytes_received = recv(sd, buffer, sizeof(buffer) - 1, 0);

                if (bytes_received <= 0) {
                    // Client disconnected
                    close(sd);
                    client_sockets[i] = 0;
                } else {
                    // Get client IP
                    struct sockaddr_in client_info;
                    socklen_t client_info_len = sizeof(client_info);
                    getpeername(sd, (struct sockaddr *)&client_info, &client_info_len);
                    char client_ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &client_info.sin_addr, client_ip, sizeof(client_ip));

                    // Send log message
                    send_log_message(&log_server_addr, buffer, client_ip);

                    // Echo back to client
                    send(sd, buffer, bytes_received, 0);
                }
            }
        }
    }
    

    // Close sockets (this part will never be reached in the current implementation)
    close(server_socks.tcp_sock);
    close(server_socks.udp_sock);
    return 0;
}