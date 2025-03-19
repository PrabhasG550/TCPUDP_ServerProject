TCP and UDP Socket Programming
Prabhas Gade

Build Commands:

1. Make 
    - compiles all programs.
    - without Makefile:
        gcc -std=c99 -o echo_s echo_s.c
        gcc -o echo_c echo_c.c
        gcc -o log_s log_s.c

Execution Commands:

2. ./log_s
    - runs the log server executable and binds to port 9999.
    - Ctrl-C or kill <pid> will prompt a clean exit, in which the port is unbinded.

3. ./echo_s <port>
    - runs the echo server executable and binds to port specified.
    - must be done on a new window.
    - Ctrl-C or kill <pid> will prompt a clean exit, in which the port is unbinded.

4. ./echo_c <serverIP> <port> <tcp|udp>
    - runs the echo client executable and binds to the port and server IP specified.
    - tcp or udp protocol must be specified by the user.
    - must also be done on a new window.
    - while there is an active connection, typing "exit" will end the connection

5. cat echo.log
    - displays all logs

Assignment Summary:

This assignment involves creating a system of clients and servers that communicate through a chain.
The echo_c client sends messages to the echo_s server, which sends a message back to the client as 
confirmation. The echo_s server then acts as a client and sends formatted log messages to the log_s server with
every new message. The log_s server inputs these messages into a file, echo.log, and prints it. 
