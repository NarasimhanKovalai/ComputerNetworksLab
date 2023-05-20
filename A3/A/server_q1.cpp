/*Narasimhan Kovalai
ROLL NUMBER-20CS01075
ASSIGNMENT 3
COMPUTER NETWORK LAB*/

#define PORT 6000
#define LIMIT 2048

#include <iostream>
#include <string>
#include <cstring>
#include <queue>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <string>
#include <algorithm>
#include <chrono>

using namespace std;

class SERVER_CLASS
{
    int sockfd;
    struct sockaddr_in sin;
    int sin_len = sizeof(sin);

public:
    SERVER_CLASS(int server_port)
    {

        // Initialise the socket:
        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if (sockfd < 0)
        {
            cout << "Error creating the socket" << endl;
            exit(EXIT_FAILURE);
        }

        // Set up the address:
        memset(&sin, '\0', sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = htonl(INADDR_ANY); /*host to network format transalation*/
        sin.sin_port = htons(PORT);

        // Bind the socket to the local address:
        if (bind(sockfd, (sockaddr *)&sin, sizeof(sin)) < 0)
        {
            cout << "Cant bind to the address" << endl;
            exit(EXIT_FAILURE);
        }
    }

    void listenNow()
    {
        // Start Listening,here 5 is the client queue limit
        if (listen(sockfd, 5) < 0)
        {
            cout << "SERVER_CLASS not able to listen" << endl;
            exit(EXIT_FAILURE);
        }

        cout << "SERVER_CLASS Listening on port: " << PORT << endl;
    }

    int acceptNow()
    {
        int new_socket;
        if ((new_socket = accept(sockfd, (sockaddr *)&sin, (socklen_t *)&sin_len)) < 0)
        {
            cout << "Error Accepting" << endl;
            exit(EXIT_FAILURE);
        }
        return new_socket;
    }

    string read_signal(int client_socket)
    {
        char memory[LIMIT] = {0};
        read(client_socket, memory, LIMIT);
        string s(memory);
        return s;
    }

    void send_signal(int client_socket, string msg)
    {
        char *msg_pointer = &msg[0];
        send(client_socket, msg_pointer, sizeof(msg), 0);
    }

    void closeConnection(int client_socket)
    {
        close(client_socket);
    }
};

// Global State Controllers:
SERVER_CLASS myserver = SERVER_CLASS(PORT);
queue<int> waitingClient;

// Utility Functions:
vector<string> splitWord(string &s, char delimiter);

void *handleClient(void *arg)
{
    // Choose the earliest client to serve
    int client_socket = waitingClient.front();
    waitingClient.pop();

    cout << "Client ID: " << client_socket << " service started:" << endl;

    myserver.send_signal(client_socket, "Connected Now:\n");
    string request_string = myserver.read_signal(client_socket);
    while (request_string.size() != 0)
    {
        vector<string> parsed_version = splitWord(request_string, ' ');
        auto start = std::chrono::system_clock::now();
        // Some computation here
        auto end = std::chrono::system_clock::now();

        std::chrono::duration<double> elapsed_seconds = end - start;
        std::time_t end_time = std::chrono::system_clock::to_time_t(end);
        cout << "Client ID: " << client_socket << " requested at " << std::ctime(&end_time) << endl;
        float result = 0;

        if (parsed_version.size() > 3)
        {
            cout << "Cannot accept more than 3 arguments. Input should be of form OPERATOR OPERAND1 OPERAND2" << endl;
            exit(EXIT_FAILURE);
        }
        string in_operator = parsed_version[0];
        string op1 = parsed_version[1];
        string op2 = parsed_version[2];

        /*Transform input string to uppercase to accept any combination of uppercase and lowercase letters*/

        transform(in_operator.begin(), in_operator.end(), in_operator.begin(), ::toupper);

        if (in_operator == "ADD")
            result = stof(op1) + stof(op2);
        else if (in_operator == "SUB")
            result = stof(op1) - stof(op2);
        else if (in_operator == "MUL")
            result = stof(op1) * stof(op2);
        else if (in_operator == "DIV")
        {
            if (stof(parsed_version[2]) == 0.0)
            {
                std::chrono::duration<double> elapsed_seconds = end - start;
                std::time_t end_time = std::chrono::system_clock::to_time_t(end);
                cout << "Cannot divide by by 0....EXITING" << endl;
                myserver.closeConnection(client_socket);
                cout << "Client ID: " << client_socket << " disconnected at "<<std::ctime(&end_time) << endl;
                pthread_exit(NULL);
            }
            result = stof(parsed_version[1]) / stof(parsed_version[2]);
        }
        else{
            cout<<"Invalid command"<<endl;
            
        }

        myserver.send_signal(client_socket, to_string(result));
        request_string = myserver.read_signal(client_socket);
    }
    myserver.closeConnection(client_socket);
    auto start = std::chrono::system_clock::now();
    // Some computation here
    auto end = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsed_seconds = end - start;
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    cout << "Client ID: " << client_socket << " now inactive at " << std::ctime(&end_time) << endl;
    pthread_exit(NULL);
}

int main()
{

    cout << "TCP Server Calculator:\n";
    myserver.listenNow();

    // Accept connections in infinite loop
    while (true)
    {
        // Accept the connections and let the separate thread handle each client;
        int client_socket = myserver.acceptNow();
        waitingClient.push(client_socket);
        pthread_t tid;
        pthread_create(&tid, NULL, handleClient, NULL);
    }

    return 0;
}

// Utility functions
vector<string> splitWord(string &s, char delimiter)
{
    vector<string> res;
    string curr;
    for (auto x : s)
    {
        if (x == delimiter)
        {
            res.push_back(curr);
            curr = "";
        }
        else
            curr += x;
    }
    res.push_back(curr);
    return res;
}