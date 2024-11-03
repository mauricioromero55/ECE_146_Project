
#include <iostream>
#include <string>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>

#pragma comment(lib, "ws2_32.lib") // Link with Ws2_32.lib

#define PORT 8080

using namespace std;

SOCKET client_fd;
bool running = true; // Control flag for receiving thread

void receive_messages() {
    char buffer[1024] = { 0 };
    while (running) {
        int valread = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (valread > 0) {
            buffer[valread] = '\0'; // Null-terminate the buffer to print correctly
            cout << "Message from server: " << buffer << endl;
        }
        else {
            // If recv returns <= 0, we assume the server has closed the connection
            running = false; // Stop receiving
        }
    }
}

int main() {
    WSADATA wsaData;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed\n";
        return -1;
    }

    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        cerr << "Socket creation error\n";
        WSACleanup();
        return -1;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Connect to the server (update IP address as needed)
    if (inet_pton(AF_INET, "192.168.1.13", &serv_addr.sin_addr) <= 0) {
        cerr << "Invalid address / Address not supported\n";
        closesocket(client_fd);
        WSACleanup();
        return -1;
    }

    if (connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        cerr << "Connection Failed\n";
        closesocket(client_fd);
        WSACleanup();
        return -1;
    }

    cout << "Connected to server. Type 'exit' to quit.\n";

    // Start a new thread for receiving messages
    thread receiver(receive_messages);

    string message;
    while (true) {
        cout << "Enter message to server: ";
        getline(cin, message);

        if (message == "exit") {
            running = false; // Stop the receiving thread
            break;
        }

        send(client_fd, message.c_str(), message.size(), 0);
        cout << "Message sent" << endl;
    }

    // Clean up
    receiver.join(); // Wait for the receiver thread to finish
    closesocket(client_fd);
    WSACleanup(); // Cleanup Winsock
    return 0;
}
