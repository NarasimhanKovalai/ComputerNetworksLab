#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <bitset>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>
#define PORT 4400
#define checksumsize 8
#define windowsize 5
#define LIMIT 2048
using namespace std;
float in_prob = 0.0;
int payloadsize = 0;

void getbits(vector<int> &arr)
{
    // Open the input file
    ifstream inputFile("q.txt");

    // Open the output file
    ofstream outputFile("bits.txt");

    // Read the input file line by line
    string line;
    while (getline(inputFile, line))
    {
        // Convert each character to its binary representation (8 bits)
        string binaryData = "";
        for (char c : line)
        {
            binaryData += bitset<8>(c).to_string();
        }
        for (auto elem : binaryData)
        {
            arr.push_back(atoi(&elem));
        }

        // Write the binary data to the output file
        outputFile << binaryData << endl;
    }

    // Close the input and output files
    inputFile.close();
    outputFile.close();
}

long int findSize(char file_name[])
{
    // opening the file in read mode
    FILE *fp = fopen(file_name, "r");

    // checking if the file exist or not
    if (fp == NULL)
    {
        printf("File Not Found!\n");
        return -1;
    }

    fseek(fp, 0L, SEEK_END);

    // calculating the size of the file
    long int res = ftell(fp);

    // closing the file
    fclose(fp);

    return res;
}

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

class SERVER_CLASS
{
    int sockfd;
    struct sockaddr_in sin;
    int sin_len = sizeof(sin);
    int new_socket;

public:
    SERVER_CLASS(int server_port)
    {
        // Initialise the socket:
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        int bufsize=100;
        setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
        if (sockfd < 0)
        {
            cout << "Error creating the socket" << endl;
            exit(EXIT_FAILURE);
        }

        // Set up the address:
        memset(&sin, '\0', sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr =
            htonl(INADDR_ANY); /*host to network format transalation*/
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
        if ((new_socket =
                 accept(sockfd, (sockaddr *)&sin, (socklen_t *)&sin_len)) < 0)
        {
            cout << "Error Accepting" << endl;
            exit(EXIT_FAILURE);
        }
        return new_socket;
    }

    string read_signal()
    {
        char memory[LIMIT] = {0};
        read(new_socket, memory, LIMIT);
        string s(memory);
        return s;
    }

    void send_signal(string msg)
    {
        auto msg_pointer = &msg[0];
        // send(new_socket, msg_pointer, sizeof(msg), 0);
        cout<<"Inside function msg,msgsize  : "<<msg<<strlen(msg_pointer)<<endl;
        int c=write(new_socket, msg_pointer,LIMIT);
        cout<<"Write length is "<<c<<endl;
    }

    void closeConnection(int client_socket) { close(client_socket); }
};

SERVER_CLASS serv_instance = SERVER_CLASS(PORT);

void introduce_error(string &s)
{
    // /choose either 1 or 2 bits to be flipped/
    int x = 1 + rand() % 2;

    while (x--)
    {
        // /choose which index(or indices) to be flipped/
        int y = rand() % (payloadsize * 8);
        s[y] == '1' ? s[y] = '0' : s[y] = '1';
    }
    cout<<"Eroor introduced"<<endl;
}

int transmission(int &i, long &M, int &payloadsize, vector<int> &arr)
{
    auto x = M / payloadsize;
    int times_transmitted = 0;
    while (i <= x)
    {
        int z = 0;
        for (int k = i; k < i + windowsize && k <= x; k++)
        {
            cout << "Sending Frame " << k << "..." << endl;
            times_transmitted++;
        }
        for (int k = i; k < i + windowsize && k <= x; k++)
        {
            
            vector<int> pack_arr;
            string s;
            for (int j = 0; j < payloadsize * 8; j++)
            {
                pack_arr.push_back(arr[(i - 1) * payloadsize * 8 + j]);
                s += to_string(arr[(i - 1) * payloadsize * 8 + j]);
            }
            cout << s << "\n";
            vector<int> cs(checksumsize);
            perform_checksum(pack_arr, cs, payloadsize * 8, checksumsize);
            for (int j = 0; j < checksumsize; j++)
            {
                s += to_string(cs[j]);
            }
            // for (int x : cs) {
            //     cout << x<<" ";
            // }
            // cout << "\n";
            // s = "hello";
            float gen = (float)(rand() % 100) / 100;
            cout << "GEN " << gen << " Input_p " << in_prob << "\n";


            if (gen < in_prob)
            {
                introduce_error(s);
            }
            cout << "Length of string "<<s<<" to be sent is : " << s.length() << endl;
            serv_instance.send_signal(s);

            string t = serv_instance.read_signal();
            cout << t << "\n";

            int f = atoi(&t[0]);

            if (!f)
            {
                cout << "Acknowledgment for Frame " << k << "..." << endl;
                z++;
            }
            else
            {
                cout << "Timeout!! Frame Number : " << k << " Not Received"
                     << endl;
                cout << "Retransmitting Window..." << endl;
                break;
            }
        }
        cout << "\n";
        i = i + z;
    }
    return times_transmitted;
}

int main(int argc, char **argv)
{
    srand(time(0));
    char file_name[] = {"q.txt"};
    auto M = findSize(file_name);
    cout << M << endl;

    if (argc < 3)
    {
        cout << "Provide both x(bytes) and p as CLI arguments" << endl;
        exit(1);
    }
    else
    {
        payloadsize = atoi(argv[1]);
        in_prob = atof(argv[2]);
        // in_prob = (float)(*argv[2]);
        cout << "calc : " << in_prob << endl;
    }
    // int port_no;
    // printf("Enter the port number of the sever : ");
    // scanf("%d", &port_no);

    // Register signal handlers:
    // signal(SIGINT, exit_handler);

    serv_instance.listenNow();
    cout << "Server started successfully..." << endl;
    serv_instance.acceptNow();
    int i = 1;
    long x = M;
    x /= payloadsize;
    vector<int> arr;
    getbits(arr);

    int tras = transmission(i, M, payloadsize, arr);
    cout << "Total number of packet sent(original+retransmission) : " << tras << "\n";
    return 0;
}