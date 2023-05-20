#include <iostream>
#include <string>
#include <unordered_map>
#include <cstring>
#include <queue>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <signal.h>
#include <mutex>

using namespace std;
void *Client_manage(void *arg);
void PeerToPeer(string my_client_name, vector<string> delimited_str);
void sendCurrentStatus(string my_client_name);
vector<string> splitWord(string &s, char delimiter);
string MainMenu(string my_client_name);
void closeTheSession(string my_client_name);

class Server
{
	int file_desc;
	struct sockaddr_in sin;
	int sin_len = sizeof(sin);
	int port = 0;

public:
	void binder(int server_port)
	{

		// Initialise the socket:
		file_desc = socket(AF_INET, SOCK_STREAM, 0);

		if (file_desc < 0)
		{
			cout << "Error creating the socket" << endl;
			exit(EXIT_FAILURE);
		}

		// Set up the address:
		memset(&sin, '\0', sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = htonl(INADDR_ANY);
		sin.sin_port = htons(server_port);

		// Bind the socket to the local address:
		if (bind(file_desc, (sockaddr *)&sin, sizeof(sin)) < 0)
		{
			cout << "Error Binding to the address" << endl;
			exit(EXIT_FAILURE);
		}

		port = server_port;
	}

	void start_listening()
	{
		// Start Listening
		if (listen(file_desc, 7) < 0)
		{
			cout << "Error Listening" << endl;
			exit(EXIT_FAILURE);
		}

		cout << "Server Listening on port: " << port << endl;
	}

	int acceptNow()
	{
		int new_socket;
		if ((new_socket = accept(file_desc, (sockaddr *)&sin, (socklen_t *)&sin_len)) < 0)
		{
			cout << "Error Accepting" << endl;
			exit(EXIT_FAILURE);
		}
		return new_socket;
	}

	string readMsg(int cl_s_id)
	{
		string msg = "";
		char last_character = '-';

		// This is a part of my protocol
		do
		{
			char buffer[2048] = {0};
			read(cl_s_id, buffer, 2048);
			string s(buffer);
			msg += s;
			last_character = s[s.size() - 1];
		} while (last_character != '%');

		return msg.substr(0, msg.size() - 1);
	}

	bool sendMsg(int cl_s_id, string msg)
	{

		// The message is sent in parts:
		msg = msg + "%"; // Special Terminating Character:
		char buffer[msg.size()];
		strcpy(buffer, &msg[0]);

		int bytes_sent_total = 0;
		int bytes_sent_now = 0;
		int message_len = msg.length();
		while (bytes_sent_total < message_len)
		{
			bytes_sent_now = send(cl_s_id, &buffer[bytes_sent_total], message_len - bytes_sent_total, 0);
			if (bytes_sent_now == -1)
			{
				// Handle error
				cout << "Send Msg Error in Server Protocol" << endl;
				return false;
			}
			bytes_sent_total += bytes_sent_now;
		}
		return true;
	}

	void closeConnection(int cl_s_id)
	{
		close(cl_s_id);
	}
};

// Global State Controllers Variable:
Server serv_instance = Server();
queue<int> client_list;
unordered_map<string, string> partnerClient;
unordered_map<string, int> sock;
mutex mtx;

void *Client_manage(void *arg)
{
	// Choose the earliest client to serve
	mtx.lock();
	int cl_s_id = client_list.front();
	client_list.pop();
	mtx.unlock();
	string client_name = serv_instance.readMsg(cl_s_id);

	// Check if already there is a user with that name:
	if (sock.find(client_name) != sock.end())
	{
		string errMsg;
		errMsg += "Close: There is already a client with that name...Disconnecting...\n";
		serv_instance.sendMsg(cl_s_id, errMsg);
		cout << "Entered client_name already exists in the database..." << endl;
		return NULL;
	}

	sock[client_name] = cl_s_id;

	cout << "Client " << client_name << " service started" << endl;
	serv_instance.sendMsg(cl_s_id, MainMenu(client_name));

	string req = "";
	while (true)
	{

		req = serv_instance.readMsg(cl_s_id);
		vector<string> delimited_str = splitWord(req, ' ');
		string command = delimited_str[0];

		for (auto &c : command)
		{
			c = toupper(c);
		}

		// If client requests status:
		if (command == ("STATUS"))
			sendCurrentStatus(client_name);
		// If client wants to connect to other client:
		else if (command == ("CONNECT"))
			PeerToPeer(client_name, delimited_str);
		else if (command == ("CLOSE"))
		{
			closeTheSession(client_name);
			break;
		}
		// If this client is already connected to some client then send the read message to that client:
		else if (partnerClient.find(client_name) != partnerClient.end())
		{
			// Checkpoint: If the message is goodbye, close the session
			string msg = "  ";
			msg += "(" + client_name + "): " + req;
			serv_instance.sendMsg(sock[partnerClient[client_name]], msg);
			if (req.compare("GOODBYE") == 0)
				closeTheSession(client_name);
			continue;
		}
		else
		{
			string errMsg;
			errMsg += "(server): Command Not Found";
			serv_instance.sendMsg(cl_s_id, errMsg);
		}
	}
	sock.erase(client_name);
	serv_instance.closeConnection(cl_s_id);
	cout << "Client " << client_name << " disconnected.." << endl;
	pthread_exit(NULL);
}

// Helper Functions ----------------------------------------------------------------------------------
void sendCurrentStatus(string my_client_name)
{
	vector<string> names;
	string statusMessage;
	statusMessage += "  (server):\n";

	for (auto it : sock)
		names.push_back(it.first);

	for (int i = 0; i < names.size() - 1; i++)
	{
		string hasPartner = partnerClient.find(names[i]) == partnerClient.end() ? "AVAILABLE" : "ENGAGED";
		statusMessage += "  " + names[i] + " " + hasPartner + "\n";
	}

	string hasPartner = partnerClient.find(names[names.size() - 1]) == partnerClient.end() ? "AVAILABLE" : "ENGAGED";
	statusMessage += "  " + names[names.size() - 1] + " " + hasPartner + "\n";

	serv_instance.sendMsg(sock[my_client_name], statusMessage);
	return;
}

void PeerToPeer(string my_client_name, vector<string> delimited_str)
{

	string peer = delimited_str[1];
	cout << "Session request from " + my_client_name + " to " + peer << endl;

	if (partnerClient.find(my_client_name) != partnerClient.end())
	{
		string errMsg;
		errMsg += "  (server): You are already connected to someone.";
		serv_instance.sendMsg(sock[my_client_name], errMsg);
		cout << "Already connected, connect request rejected" << endl;
		return;
	}

	if (my_client_name.compare(peer) == 0)
	{
		string errMsg;
		errMsg += "  (server):   Cannot communicate with oneself";
		serv_instance.sendMsg(sock[my_client_name], errMsg);
		cout << "  Cannot communicate with oneself" << endl;
		return;
	}

	if (sock.find(peer) == sock.end())
	{
		string errMsg;
		errMsg += "  (server): No client named: " + peer;

		serv_instance.sendMsg(sock[my_client_name], errMsg);
		cout << "Couldn't connect: No client named: " << peer << endl;
		return;
	}

	if (partnerClient.find(peer) != partnerClient.end())
	{
		string errMsg;
		errMsg += "  (server): " + peer + " is BUSY";

		serv_instance.sendMsg(sock[my_client_name], errMsg);
		cout << "Couldn't connect: client " + peer + " is busy" << endl;
		return;
	}

	partnerClient[my_client_name] = peer;
	partnerClient[peer] = my_client_name;

	cout << "Connected: " + my_client_name << " and " << peer << endl;

	// Notify the clients that they are now connected:
	string successMsg;
	successMsg += "  (server): You are now connected to ";
	serv_instance.sendMsg(sock[peer], successMsg + my_client_name);
	serv_instance.sendMsg(sock[my_client_name], successMsg + peer);
	return;
}

void closeTheSession(string my_client_name)
{
	// Inform the partner about the session closure;
	if (partnerClient.find(my_client_name) == partnerClient.end())
		return;
	string peer = partnerClient[my_client_name];
	string msg;
	msg += "\n  (server): " + my_client_name + " closed the chat session";

	serv_instance.sendMsg(sock[peer], msg);

	// Update the data structures to close the connection:

	if (partnerClient.find(peer) != partnerClient.end())
		partnerClient.erase(peer);

	if (partnerClient.find(my_client_name) != partnerClient.end())
		partnerClient.erase(my_client_name);

	// Inform the requesting client about the closure:
	msg = "";

	msg += "  (server): Session closed";

	serv_instance.sendMsg(sock[my_client_name], msg);

	// Log the data to the server:
	cout << "Disconnected " + my_client_name + " and " + peer << endl;
}

string MainMenu(string my_client_name)
{
	string msg;
	msg =msg+"-----------------------------------\n " + " \n  (server): Hi, " + my_client_name + "\n";
	msg += "  The supported commands are:\n";
	msg += "  a-> STATUS             : TO SEE WHO ARE ENGAGED OR AVAILABLE FOR CHAT\n";
	msg += "  b-> CONNECT <username> : CONNECT TO PEER\n";
	msg += "  c-> GOODBYE            : TERMINATES CHAT SESSION\n";
	msg += "  d-> CLOSE              : SERVER FORGETS CLIENT\n";
	msg+="---------------------------------------\n";
	return msg;
}

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

void exit_handler(int sig)
{
	cout << "\nShutting down the server...\nDisconnecting Clients...\n";
	for (auto it : sock)
	{
		serv_instance.sendMsg(it.second, "close");
		serv_instance.closeConnection(it.second);
	}
	exit(0);
	return;
}

int main()
{

	int port_no;
	printf("Enter the port number of the sever : ");
	scanf("%d", &port_no);
	serv_instance.binder(port_no);

	// Register signal handlers:
	signal(SIGINT, exit_handler);

	serv_instance.start_listening();
	cout << "Server started successfully..." << endl;

	// Accept connections in infinite loop
	while (true)
	{
		// Accept the connections and let the separate thread handle each client;
		int cl_s_id = serv_instance.acceptNow();
		client_list.push(cl_s_id);
		pthread_t tid;
		pthread_create(&tid, NULL, Client_manage, NULL);
	}
	return 0;
}
