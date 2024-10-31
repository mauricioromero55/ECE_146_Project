/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")  // Link with Winsock library

void error(const char *msg)
{
    perror(msg);
    WSACleanup();  // Clean up Winsock resources on error
    exit(1);
}

int main(int argc, char *argv[])
{
    WSADATA wsaData;
    int iResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        fprintf(stderr, "WSAStartup failed with error: %d\n", iResult);
        exit(1);
    }

    SOCKET sockfd, newsockfd;
    int portno;
    struct sockaddr_in serv_addr, cli_addr;
    int n;
    socklen_t clilen;
    char buffer[256];

    if (argc < 2) {
        fprintf(stderr, "ERROR, no port provided\n");
        WSACleanup();
        exit(1);
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == INVALID_SOCKET)
        error("ERROR opening socket");

    memset(&serv_addr, 0, sizeof(serv_addr));  // Zero out the server address structure
    portno = atoi(argv[1]);  // Convert the port number from a string to an integer

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR)
        error("ERROR on binding");

    listen(sockfd, 5);  // Start listening with a backlog of 5 connections
    clilen = sizeof(cli_addr);

    newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
    if (newsockfd == INVALID_SOCKET)
        error("ERROR on accept");

    memset(buffer, 0, 256);  // Clear the buffer

    n = recv(newsockfd, buffer, 255, 0);  // Receive message from the client
    if (n == SOCKET_ERROR) error("ERROR reading from socket");

    printf("Here is the message: %s\n", buffer);

    n = send(newsockfd, "I got your message", 18, 0);  // Send response to client
    if (n == SOCKET_ERROR) error("ERROR writing to socket");

    closesocket(newsockfd);
    closesocket(sockfd);
    WSACleanup();  // Clean up Winsock

    return 0;
}

