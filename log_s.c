#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h> // For sockets
#include <signal.h> //for clean exit

#define PORT 9999          // Port number to listen on
#define LOG_FILE "echo.log" // File to save messages

void cleanExit() {
    printf("\nClean Exit - Removing Port Bindings...\n");
    exit(0);  // Exit immediately without cleaning up resources
}

int main() {
    int sock; // Our socket
    char buffer[1024]; // Where messages are stored
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    
    // Register signal handlers for SIGINT and SIGTERM
    signal(SIGINT, cleanExit);  // For Ctrl+C
    signal(SIGTERM, cleanExit); // For termination signals

    // 1. Create a UDP socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("Socket creation failed");
        return 1;
    }
    
    // Set socket option SO_REUSEADDR to allow binding even if it's in TIME_WAIT state
    int optval = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0) {
        perror("setsockopt failed for SO_REUSEPORT");
        return 1;
    }

    // 2. Set up the server's address and bind the socket to it
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return 1;
    }

    printf("Log Server is running on port %d...\n", PORT);

    // 3. Wait for messages forever
    while (1) {
        // Get a message from the Echo Server
        int bytes_received = recvfrom(sock, buffer, sizeof(buffer)-1, 0, (struct sockaddr *)&client_addr, &addr_len);
        if (bytes_received > 0) {
          buffer[bytes_received] = '\0'; // Null-terminate the string
        }

        // Open the log file and write the message
        FILE *file = fopen(LOG_FILE, "a"); // Append mode
        if (file) {
            fprintf(file, "Message: %s\n", buffer);
            fclose(file);
        }

        // Print the message to the screen
        printf("%s\n", buffer);
        memset(buffer, 0, sizeof(buffer));
    }
    memset(buffer, 0, sizeof(buffer));

    close(sock);
    return 0;
}
