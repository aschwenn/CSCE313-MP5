#ifndef _netreqchannel_H_
#define _netreqchannel_H_

#include <iostream>
#include <fstream>

#include <string>

using namespace std;

class NetworkRequestChannel {

public:

private:

	int cfd; // socket file descriptor from client constructor
	int sfd; // master socket file descriptor from server constructor
	int backlog;

public:

	NetworkRequestChannel(const string _server_host_name, const unsigned short _port_no); // client-side constructor

	NetworkRequestChannel(const unsigned short _port_no, void*(*connection_handler)(void*), int _backlog); // server-side constructor
	
	~NetworkRequestChannel();

	string cread();

	int cwrite(string _msg);

	int read_fd();
};


#endif


