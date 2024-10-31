#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib") // Link Winsock library

using namespace std;

#define PORT 9090
#define MAX_CLIENTS 10

vector<SOCKET> clients; // To store client sockets
mutex client_mutex;      // Mutex for thread-safe access to client list

// Function to handle each client
void handle_client(SOCKET client_socket) {
    char buffer[1024] = { 0 };
    while (true) {
        // Clear the buffer
        memset(buffer, 0, sizeof(buffer));

        // Read message from client
        int valread = recv(client_socket, buffer, sizeof(buffer), 0);

        if (valread <= 0) {
            // Client has disconnected
            cout << "Client disconnected.\n";
            closesocket(client_socket);

            // Remove client from list
            client_mutex.lock();
            clients.erase(remove(clients.begin(), clients.end(), client_socket), clients.end());
            client_mutex.unlock();
            break;
        }

        // Print received message
        cout << "Message from client: " << buffer << endl;

        // Forward the message to all other clients
        client_mutex.lock();
        for (SOCKET client : clients) {
            if (client != client_socket) {
                send(client, buffer, strlen(buffer), 0);
            }
        }
        client_mutex.unlock();
    }
}

int main() {
    WSADATA wsaData;
    int iResult;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        cerr << "WSAStartup failed: " << iResult << endl;
        return 1;
    }

    SOCKET server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create server socket
    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd == INVALID_SOCKET) {
        cerr << "Socket creation failed: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }

    // Set socket options to allow address reuse
    iResult = setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    if (iResult == SOCKET_ERROR) {
        cerr << "Setsockopt failed: " << WSAGetLastError() << endl;
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // Bind the socket to the IP and port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    iResult = bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    if (iResult == SOCKET_ERROR) {
        cerr << "Bind failed: " << WSAGetLastError() << endl;
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections
    iResult = listen(server_fd, MAX_CLIENTS);
    if (iResult == SOCKET_ERROR) {
        cerr << "Listen failed: " << WSAGetLastError() << endl;
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }
    cout << "Server listening on port " << PORT << endl;

    // Accept connections in a loop
    while (true) {
        client_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if (client_socket == INVALID_SOCKET) {
            cerr << "Accept failed: " << WSAGetLastError() << endl;
            continue;
        }
        cout << "New client connected.\n";

        // Add new client to the list and create a new thread to handle it
        client_mutex.lock();
        clients.push_back(client_socket);
        client_mutex.unlock();
        thread(handle_client, client_socket).detach();  // Detach thread to allow independent execution
    }

    // Clean up Winsock resources
    closesocket(server_fd);
    WSACleanup();
    return 0;
}
