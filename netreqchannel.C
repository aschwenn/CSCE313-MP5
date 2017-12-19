#include <cassert>
#include <cstring>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sstream>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

#include "netreqchannel.H"

using namespace std;

NetworkRequestChannel::NetworkRequestChannel(const string _server_host_name, const unsigned short _port_no) {
	/* CLIENT-SIDE CONSTRUCTOR */
	stringstream ss; // from int2str function
	ss << _port_no;
	string port = ss.str();

	// using Networking slides as reference
	struct sockaddr_in address, *servinfo;
	memset(&address, 0, sizeof(address));
	address.sin_family = AF_INET;

	if (struct servent * pse = getservbyname(port.c_str(), "tcp")) {
		address.sin_port = pse->s_port;
	}
	else if ((address.sin_port = htons((unsigned short)atoi(port.c_str()))) == 0) {
		cerr << "client side: bettati's code messed up: ";
		cout << strerror(errno) << endl;
		close(sfd); exit(0);
	}
	// map host name to IP
	if (struct hostent * phe = gethostbyname(_server_host_name.c_str())) {
		memcpy(&address.sin_addr, phe->h_addr, phe->h_length);
	}
	else if ((address.sin_addr.s_addr = inet_addr(_server_host_name.c_str())) == INADDR_NONE) {
		cerr << "can't get host entry: ";
		cout << strerror(errno) << endl;
		close(sfd); exit(0);
	}
	// allocate socket
	int s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		cerr << "can't create socket: ";
		cout << strerror(errno) << endl;
		close(sfd); exit(0);
	}
	// connect the socket
	if (connect(s, (struct sockaddr *)&address, (socklen_t)sizeof(address))) {
		cerr << "can't connect to host \"" << _server_host_name << "\" at port " << port << ": ";
		cout << strerror(errno) << endl;
		close(sfd); exit(0);
	}
	cfd = s; // save socket file descriptor
}

NetworkRequestChannel::NetworkRequestChannel(const unsigned short _port_no, void*(*connection_handler)(void*), int _backlog) {
	/* SERVER-SIDE CONSTRUCTOR */
	/* set up server side */
	backlog = _backlog;
	cout << "Using a backlog of " << backlog << endl;
	stringstream ss; // from int2str function
	ss << _port_no;
	string port = ss.str();

	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	// from the Networking slides
	if (struct servent * pse = getservbyname(port.c_str(), "tcp")) {
		address.sin_port = pse->s_port;
	}
	else if ((address.sin_port = htons((unsigned short)atoi(port.c_str()))) == 0) {
		cerr << "server side: bettati's code messed up: ";
		cout << strerror(errno) << endl;
		close(sfd); exit(0);
	}
	// set file descriptor for socket
	int sfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sfd < 0) {
		cerr << "socket()  failed: ";
		cout << strerror(errno) << endl;
		close(sfd); exit(0);
	}
	// bind the socket
	if (bind(sfd, (struct sockaddr *)&address, sizeof(address)) < 0) {
		cerr << "bind() failed: ";
		cout << strerror(errno) << endl;
		close(sfd); exit(0);
	}
	// listen on the socket
	if (listen(sfd, backlog) < 0) {
		cerr << "listen() failed: ";
		cout << strerror(errno) << endl;
		close(sfd); exit(0);
	}
	listen(sfd, backlog);

	/* handle incoming connections */
	cout << "SERVER AVAILABLE" << endl << endl;
	pthread_t th;
	pthread_attr_t ta;
	pthread_attr_init(&ta);
	int master_socket = sfd;
	int size = sizeof(address);
	for (;;) { // runs forever
		int *slave_socket = new int;
		*slave_socket = accept(master_socket, (struct sockaddr*)&address, (socklen_t*)&size);
		if (slave_socket < 0) {
			if (errno == EINTR) continue;
			else {
				cerr << "accept failed: ";
				cout << strerror(errno) << endl;
				close(sfd);
			}
		}
		pthread_create(&th, &ta, connection_handler, (void*)slave_socket);
	}
}

NetworkRequestChannel::~NetworkRequestChannel() {
	close(sfd);
}

const int MAX_MESSAGE = 255;

string NetworkRequestChannel::cread() {
	char buf[MAX_MESSAGE];
	if (read(cfd, buf, MAX_MESSAGE) < 0) {
		cerr << "READ ERROR";
	}

	string s = buf;
	return s;
}

int NetworkRequestChannel::cwrite(string _msg) {
	if (_msg.length() >= MAX_MESSAGE) {
		cerr << "Message too long for Channel!\n";
	return -1;
	}
	const char * s = _msg.c_str();

	if (write(cfd, s, strlen(s)+1) < 0) {
		cerr << "WRITE ERROR";
	}
}

int NetworkRequestChannel::read_fd() {
	return cfd;
}