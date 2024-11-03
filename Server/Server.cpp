#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <signal.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib") // Link with Ws2_32.lib

#define PORT 8080

using namespace std;

vector<SOCKET> clients; // To store connected client sockets
mutex clients_mutex;    // Mutex for thread-safe access to client list
SOCKET server_fd;       // Global server socket
int client_counter = 0; // To assign unique IDs to clients
bool running = true;    // Flag to control server execution

// Function to broadcast messages to all clients except the sender
void broadcast_message(const string& message, SOCKET sender_socket) {
    clients_mutex.lock();
    for (SOCKET client : clients) {
        if (client != sender_socket) { // Don't send back to the sender
            send(client, message.c_str(), message.size(), 0);
        }
    }
    clients_mutex.unlock();
}

// Signal handler for graceful shutdown
void signal_handler(int signal) {
    cout << "\nShutting down server..." << endl;
    running = false; // Stop accepting new clients

    // Notify all clients about the shutdown
    string shutdown_message = "Server is shutting down...";
    broadcast_message(shutdown_message, INVALID_SOCKET);

    // Close all client sockets
    clients_mutex.lock();
    for (SOCKET client : clients) {
        closesocket(client);
    }
    clients.clear();
    clients_mutex.unlock();

    // Close the server socket
    closesocket(server_fd);
    WSACleanup(); // Cleanup Winsock
    exit(0); // Exit the program
}

// Function to handle each client
void handle_client(SOCKET client_socket) {
    client_counter++;
    int client_id = client_counter;
    string welcome_message = "Client " + to_string(client_id) + " has joined the chat.";
    cout << welcome_message << endl;
    broadcast_message(welcome_message, client_socket);

    char buffer[1024] = { 0 };
    while (true) {
        memset(buffer, 0, sizeof(buffer)); // Clear the buffer
        int valread = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

        if (valread <= 0) {
            // Client disconnected
            string disconnect_message = "Client " + to_string(client_id) + " has left the chat.";
            cout << disconnect_message << endl;
            broadcast_message(disconnect_message, client_socket);

            closesocket(client_socket);
            clients_mutex.lock();
            clients.erase(remove(clients.begin(), clients.end(), client_socket), clients.end());
            clients_mutex.unlock();
            break;
        }

        // Prefix the message with the client ID and broadcast it
        string message = "Client " + to_string(client_id) + ": " + buffer;
        cout << message << endl;
        broadcast_message(message, client_socket);
    }
}

int main() {
    WSADATA wsaData;
    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSAStartup failed." << endl;
        return 1;
    }

    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        perror("Socket creation failed");
        WSACleanup();
        return 1;
    }

    // Set socket options to reuse address
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind the socket to the port
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        perror("Bind failed");
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }

    if (listen(server_fd, SOMAXCONN) == SOCKET_ERROR) {
        perror("Listen failed");
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }
    cout << "Server listening on port " << PORT << endl;

    // Register signal handler for graceful shutdown
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Accept multiple clients and spawn a thread for each
    while (running) {
        SOCKET client_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        if (client_socket == INVALID_SOCKET) {
            perror("Accept failed");
            continue;
        }

        // Add the new client to the list
        clients_mutex.lock();
        clients.push_back(client_socket);
        clients_mutex.unlock();

        // Spawn a new thread to handle this client
        thread(handle_client, client_socket).detach();
    }

    // Cleanup
    for (SOCKET client : clients) {
        closesocket(client);
    }
    closesocket(server_fd);
    WSACleanup();
    return 0;
}
