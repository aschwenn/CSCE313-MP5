#include <cassert>
#include <cstring>
#include <sstream>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

#include "netreqchannel.H"

using namespace std;

static int nthreads = 0;
int MAX_MSG = 255;

string int2string(int number) {
   stringstream ss;
   ss << number;
   return ss.str();
}

string sread(int * fd) {
	char buf[MAX_MSG]; // buffer for the read
	read(*fd, buf, MAX_MSG);
	string read = buf;
	return read;
}

int swrite(int * fd, string s) {
	if (s.length() >= MAX_MSG) cerr << "Message is too long";
	if (write(*fd, s.c_str(), s.length() + 1) < 0) cerr << "Error writing with swrite()";
}

void process_hello(int * fd, const string & _request) {
	swrite(fd, "hello to you too");
}

void process_data(int * fd, const string &  _request) {
	usleep(1000 + (rand() % 5000));
	swrite(fd, int2string(rand() % 100));
}

void process_request(int * fd, const string & _request) {

	if (_request.compare(0, 5, "hello") == 0) {
		process_hello(fd, _request);
	}
	else if (_request.compare(0, 4, "data") == 0) {
		process_data(fd, _request);
	}
	else { // error handling
		swrite(fd, "unknown request");
	}

}

void* connection_handler(void * _fd) {
	int * fd = (int*)_fd; // passing in an int* gave me a ton of errors
	if (!fd) cerr << "Null file descriptor";
	for (;;) {
		string request = sread(fd);
		if (request.compare("quit") == 0) {
			swrite(fd, "bye"); // closing the connection
			usleep(10000);
			break;
		}
		process_request(fd, request);
	}
}

int main(int argc, char * argv[]) {
	unsigned short PORT_NO = 9009;
	int BACK_LOG = 150;

	/* Taking in command-line arguments */
	for (int i = 0; i < argc; i++) {
		if (argv[i][1] == 'b') BACK_LOG = atoi(argv[i + 1]);
		else if (argv[i][1] == 'p') PORT_NO = atoi(argv[i + 1]);
	}

	cout << "SERVER OPENED :: Port " << PORT_NO << endl;
	NetworkRequestChannel n(PORT_NO, connection_handler, BACK_LOG);
	cout << "SERVER OFFLINE" << endl;
}

