
/*Narasimhan Kovalai
ROLL NUMBER-20CS01075
ASSIGNMENT 3
COMPUTER NETWORK LAB*/

#define PORT 6000
#define LIMIT 1024
#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

using namespace std;
string localhost = "127.0.0.1";

class CLIENT_CLASS
{
    int sockfd;
    struct sockaddr_in saddr;

public:
    CLIENT_CLASS(string server_address, int server_port)
    {
        // Initialise the socket:
        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if (sockfd < 0)
        {
            cout << "Socket file descriptor cant be created." << endl;
            exit(EXIT_FAILURE);
        }

        // Set the address:
        memset(&saddr, '\0', sizeof(sockaddr));
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(server_port);

        if (inet_pton(AF_INET, "127.0.0.1", &saddr.sin_addr) <= 0)
        {
            cout << "Invalid server address, enter local host server once again" << endl;
            exit(EXIT_FAILURE);
        }
    }

    void CONN_SERV_CLI()
    {
        // Start Connection:
        if (connect(sockfd, (sockaddr *)&saddr, sizeof(sockaddr)) < 0)
        {
            cout << "Connection Failed due to sytem call fault" << endl;
            exit(EXIT_FAILURE);
        }
    }

    string read_signal()
    {
        char memory[LIMIT] = {0};
        read(sockfd, memory, LIMIT);
        string s(memory);
        return s;
    }

    void send_signal(string msg)
    {
        char *msg_pointer = &msg[0];
        send(sockfd, msg_pointer, sizeof(msg), 0);
        return;
    }

    void closenow()
    {
        close(sockfd);
    }
};

int main()
{

    CLIENT_CLASS myclient = CLIENT_CLASS("127.0.0.1", PORT);
    myclient.CONN_SERV_CLI();
    cout << myclient.read_signal();

    string request;
    getline(cin, request);

    while (true)
    {
        myclient.send_signal(request);
        if (request.size() == 0)
        {
            cout << "Exiting the program in 3 seconds...";
            sleep(3);
            break;
        }
        cout << myclient.read_signal() << endl;
        getline(cin, request);
    }

    myclient.closenow();
    return 0;
}