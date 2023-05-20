
/*Narasimhan Kovalai
ROLL NUMBER-20CS01075
ASSIGNMENT 4
COMPUTER NETWORK LAB*/

#define PORT 4400
#define LIMIT 2048
#define checksumsize 8
#define payloadsize 4
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include <cstring>
#include <iostream>
#include <string>

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
            cout << "Invalid server address, enter local host server once again"
                 << endl;
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
        int n = read(sockfd, memory, LIMIT);
       // cout << "Value of n is " <<n<< endl;
        // cout << n<< "\n";
        string s(memory);
        return s;
    }

    void send_signal(string msg)
    {
        auto msg_pointer = &msg[0];
        // send(sockfd, msg_pointer, sizeof(msg), 0);
        // cout << "SENDING...\n";
        int n = write(sockfd, msg_pointer, LIMIT);
        //  cout <<"Value of n whle write"<< n<< "\n";
        // return;
    }

    void closenow() { close(sockfd); }
};

void perform_checksum(vector<int> &arr, vector<int> &checksum, int x, int y)
{
    /*taking XOR followed by 1's complement */
    int n = x / y;
    int table[n][y];
    for (int i = 0; i < n; i++)
        for (int j = 0; j < y; j++)
            table[i][j] = arr[i * (y) + j];

    for (int i = 1; i < n; i++)
        for (int j = 0; j < y; j++)
            table[i][j] ^= table[i - 1][j];

    for (int i = 0; i < y; i++)
        checksum[i] = table[n - 1][i] ^ 1;
}

int main()
{
    CLIENT_CLASS myclient = CLIENT_CLASS("127.0.0.1", PORT);
    myclient.CONN_SERV_CLI();

    while (1)
    {
        string s = myclient.read_signal();
        vector<int> cs(checksumsize);
        vector<int> pak_arr(strlen(&s[0]), 0);
        cout << "Received packet :" << s << endl;
        int y = 0;
        for(int i=0;i<s.length();i++){
            pak_arr[i]=s[i]-'0';
        }
        cout << "pak array is : ";
        for (auto elem : pak_arr)
        {
            cout << elem;
        }
        cout << endl;
        // for (int x : pak_arr) {
        //     cout << x<< " ";
        // }
        // cout << "\n";
        perform_checksum(pak_arr, cs, s.length(), checksumsize);
        // for (int x : cs) {
        //     cout << x<<" ";
        // }
        // cout << "\n";
        int flag = 0;
        for (int x : cs)
        {
            if (x != 0)
            {
                flag = 1;
                break;
            }
        }
        // cout << s<< "\n";
        myclient.send_signal(to_string(flag));
    }

    myclient.closenow();
    return 0;
}