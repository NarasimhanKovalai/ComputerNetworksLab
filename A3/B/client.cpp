#define PORT 4000

#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <vector>

using namespace std;

void check_for_close(string msgRead);
vector<string> splitWord(string &s, char delim);

// The CLIENT_CLASS class
class CLIENT_CLASS
{
	int sockfd;
	struct sockaddr_in saddr;

public:
	bool constructNow(string server_address, int server_port)
	{
		// Initialise the socket:
		sockfd = socket(AF_INET, SOCK_STREAM, 0);

		if (sockfd < 0)
		{
			cout << "Error : file desciptor value negative" << endl;
			exit(EXIT_FAILURE);
		}

		// Set the address:
		memset(&saddr, '\0', sizeof(sockaddr));
		saddr.sin_family = AF_INET;
		saddr.sin_port = htons(server_port);

		if (inet_pton(AF_INET, &server_address[0], &saddr.sin_addr) <= 0)
		{
			cout << "Invalid Address" << endl;
			exit(EXIT_FAILURE);
		}

		return true;
	}

	void Start_Connection()
	{
		// Start Connection:
		if (connect(sockfd, (sockaddr *)&saddr, sizeof(sockaddr)) < 0)
		{
			cout << "Connection Failed" << endl;
			exit(EXIT_FAILURE);
		}
	}

	string readMsg()
	{
		string msg = "";
		char last_character = '-';
		do
		{
			char buffer[2048] = {0};
			read(sockfd, buffer, 2048);
			string s(buffer);
			msg += s;
			last_character = s[s.size() - 1];
		} while (last_character != '%');

		return msg.substr(0, msg.size() - 1);
	}

	bool sendMsg(string msg)
	{
		
		// The message is sent in parts:
		msg += "%"; // Special Terminating Character:
		char buffer[msg.size() + 1];
		strcpy(buffer, &msg[0]);

		int bytes_sent_total = 0;
		int bytes_sent_now = 0;
		int message_len = msg.length();
		while (bytes_sent_total < message_len)
		{
			bytes_sent_now = send(sockfd, &buffer[bytes_sent_total], message_len - bytes_sent_total, 0);
			if (bytes_sent_now == -1)
			{
				// Handle error:
				cout << "Send Msg Error in Sending Protocol" << endl;
				return false;
			}
			bytes_sent_total += bytes_sent_now;
		}
		return true;
	}

	void closenow()
	{
		close(sockfd);
	}
};

// Global controller variables:
CLIENT_CLASS myclient = CLIENT_CLASS();
pthread_t recv_t, send_t;

// Main client logic:
void *receive_chat(void *arg)
{
	while (true)
	{
		string msgRead = myclient.readMsg();
		cout << msgRead << endl;
		check_for_close(msgRead);
	}
}

void *send_chat(void *arg)
{
	while (true)
	{
		string msg;
		getline(cin, msg);
		myclient.sendMsg(msg);
		check_for_close(msg);
	}
}

// Helper Functions:
void check_for_close(string msgRead)
{
	vector<string> parsedCommand = splitWord(msgRead, ' ');
	if (parsedCommand.size() > 0 && (parsedCommand[0].compare("CLOSE") == 0))
		exit(0);
}

// Utility Functions:
vector<string> splitWord(string &s, char delim)
{
	vector<string> temp;
	string curr;
	for (auto x : s)
	{
		if (x == delim)
		{
			temp.push_back(curr);
			curr = "";
		}
		else
			curr += x;
	}
	temp.push_back(curr);
	return temp;
}

// Signal Handlers:
void exit_handler(int sig)
{
	myclient.sendMsg("CLOSE");
	myclient.closenow();
	exit(0);
}

int main(int argc, char **argv)
{

	try
	{
		string ip_address(argv[1]);

		// Construct the client:
		myclient.constructNow(ip_address, stoi(argv[2]));
	}
	catch (exception e)
	{
		cout << "Incorrect format. Please provide ip address and port number as first two arguments\n";
		exit(0);
	}

	// Register signal handlers:
	signal(SIGINT, exit_handler);

	myclient.Start_Connection();
	cout << "Connected to server..." << endl;
	cout << "Enter your username: ";

	string myName;
	getline(cin, myName);
	myclient.sendMsg(myName);

	// Just to confirm that the connection is established:

	pthread_create(&recv_t, NULL, receive_chat, NULL);
	pthread_create(&send_t, NULL, send_chat, NULL);

	pthread_join(recv_t, NULL);
	pthread_join(send_t, NULL);

	myclient.closenow();
	return 0;
}