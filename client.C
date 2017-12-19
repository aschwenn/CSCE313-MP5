#include <cassert>
#include <cstring>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <string>
#include <pthread.h>
#include <map>
#include <time.h>
#include <sys/time.h>
#include <sstream>

#include "netreqchannel.H"
#include "bounded_buffer.H"

using namespace std;

/* DATA */

typedef struct foo {
	string patient_name;
	BoundedBuffer* WBB;
	int n;
} RTASTRUCT; // request threads

typedef struct foo2 {
	BoundedBuffer* WBB;
	NetworkRequestChannel* req;
} WTASTRUCT; // worker threads

typedef struct foo3 {
	BoundedBuffer* SBB; // stats buffer
	string patient_name;
} STASTRUCT; // stats threads

BoundedBuffer* buffer; // worked bounded buffer
BoundedBuffer* joeStats;
BoundedBuffer* janeStats;
BoundedBuffer* johnStats;

vector<int> joeHist(100);
vector<int> janeHist(100);
vector<int> johnHist(100);

// defaults (if not passed in via command-line arguments)
int NUM_REQUESTS = 1000;
int BB_SIZE = 50;
int WORKER_THREADS = 100; // this is the number of network request channels
string HOSTNAME = "localhost";
int PORT_NO = 9009;

/* FUNCTIONS */

BoundedBuffer* lookup(string req) {
	string test = req.substr(5);

	if (test == "Joe Smith") {
		return joeStats;
	}
	else if (test == "Jane Smith") {
		return janeStats;
	}
	else if (test == "John Doe") {
		return johnStats;
	}
}

void* event_handler(void* v) {
	fd_set readset;
	int max = 0; // need a max value for select()
	int workercount = 0;
	int responsecount = 0;

	map<NetworkRequestChannel*, BoundedBuffer*> STATMAP;
	NetworkRequestChannel* netchannels[WORKER_THREADS]; // for use in event handler

	// initialize request channels
	cout << endl << "Initializing network request channels..." << flush << endl;
	for (int i = 0; i < WORKER_THREADS; i++) {
		NetworkRequestChannel* newchannel = new NetworkRequestChannel(HOSTNAME, PORT_NO);
		netchannels[i] = newchannel;
	}
	cout << "Initialized." << flush;

	for (int i = 0; i < WORKER_THREADS; i++) {
		string reply = buffer->Remove();
		workercount++; // increment the workercount
		STATMAP.insert(pair<NetworkRequestChannel*, BoundedBuffer*>(netchannels[i], lookup(reply)));
		netchannels[i]->cwrite(reply);
	}

	for (;;) {
		FD_ZERO(&readset); // initialize readset
		for (int i = 0; i < WORKER_THREADS; i++) {
			if (netchannels[i]->read_fd() > max) max = netchannels[i]->read_fd();
			FD_SET(netchannels[i]->read_fd(),&readset);
		}

		if (select(max++, &readset, NULL, NULL, NULL)) {
			for (int i = 0; i < WORKER_THREADS; i++) {
				if (FD_ISSET(netchannels[i]->read_fd(), &readset)) {
					// reading from the netreqchannel
					string response = netchannels[i]->cread();
					responsecount++; // increment responses
					STATMAP[netchannels[i]]->Deposit(response); // send to statmap
					
					if (workercount < NUM_REQUESTS * 3) { // number of total requests for 3 people
						// writing to the netreqchannel
						string reply = buffer->Remove();
						STATMAP.erase(netchannels[i]);
						STATMAP.insert(pair<NetworkRequestChannel*, BoundedBuffer*>(netchannels[i], lookup(reply)));
						netchannels[i]->cwrite(reply);
						workercount++;
					}
				}
			}
		}
		if (responsecount == NUM_REQUESTS * 3) break;
	}
	
	// close all of the channels
	for (int i = 0; i < WORKER_THREADS; i++) {
		netchannels[i]->cwrite("quit");
	}
}

void* request_thread_func(void* rtastruct) {
	RTASTRUCT* myrtastruct = (RTASTRUCT*)rtastruct;
	for (int i = 0; i < NUM_REQUESTS; i++) {
		string request = "data " + myrtastruct->patient_name;
		myrtastruct->WBB->Deposit(request);
	}
}

void* statistics_thread_func(void* stastruct) {
	STASTRUCT* mystastruct = (STASTRUCT*)stastruct;
	string name = mystastruct->patient_name;
	for (int i = 0; i < NUM_REQUESTS; ++i) {
		if (name == "Joe Smith") {
			string response = mystastruct->SBB->Remove();
			joeHist[atoi(response.c_str())]++;
		}
		else if (name == "Jane Smith") {
			string response = mystastruct->SBB->Remove();
			janeHist[atoi(response.c_str())]++;
		}
		else if (name == "John Doe") {
			string response = mystastruct->SBB->Remove();
			johnHist[atoi(response.c_str())]++;
		}
		else { break; }
	}
}

int main(int argc, char * argv[]) {

	/* Taking in command-line arguments */
	for (int i = 0; i < argc; i++) {
		if (argv[i][1] == 'n') NUM_REQUESTS = atoi(argv[i + 1]);
		else if (argv[i][1] == 'b') BB_SIZE = atoi(argv[i + 1]);
		else if (argv[i][1] == 'w') WORKER_THREADS = atoi(argv[i + 1]);
		else if (argv[i][1] == 'h') HOSTNAME = argv[i + 1];
		else if (argv[i][1] == 'p') PORT_NO = atoi(argv[i + 1]);
	}

	/* Initializing BoundedBuffers */
	buffer = new BoundedBuffer(BB_SIZE);
	joeStats = new BoundedBuffer(BB_SIZE);
	janeStats = new BoundedBuffer(BB_SIZE);
	johnStats = new BoundedBuffer(BB_SIZE);

	/* Initializing request structs */
	RTASTRUCT* joeStruct = new RTASTRUCT;
	RTASTRUCT* janeStruct = new RTASTRUCT;
	RTASTRUCT* johnStruct = new RTASTRUCT;
	
	joeStruct->patient_name = "Joe Smith";
	joeStruct->WBB = buffer;
	joeStruct->n = NUM_REQUESTS;
	
	janeStruct->patient_name = "Jane Smith";
	janeStruct->WBB = buffer;
	janeStruct->n = NUM_REQUESTS;

	johnStruct->patient_name = "John Doe";
	johnStruct->WBB = buffer;
	johnStruct->n = NUM_REQUESTS;


	/* Create threads */
	pthread_t request_t[3];
	pthread_t event_t;
	pthread_t stat_t[3];

	cout << "--- CLIENT STARTED ---" << endl;
	cout << "NUMBER OF REQUESTS: " << NUM_REQUESTS << endl;
	cout << "BOUNDED BUFFER SIZE: " << BB_SIZE << endl;
	cout << "NETWORK REQUEST CHANNELS: " << WORKER_THREADS << endl;


	struct timeval begin;
	struct timeval end;
	gettimeofday(&begin, NULL); // get before time

	/* Create threads */
	// request
	pthread_create(&request_t[0], NULL, request_thread_func, (void*)joeStruct);
	pthread_create(&request_t[1], NULL, request_thread_func, (void*)janeStruct);
	pthread_create(&request_t[2], NULL, request_thread_func, (void*)johnStruct);
		
	cout << "Connecting to port " << PORT_NO << " and hostname " << HOSTNAME << endl;
	pthread_create(&event_t, NULL, event_handler, NULL);
	cout << "Processing requests... " << endl;

	/* Initializing stats structs */
	STASTRUCT* joeSTASTRUCT = new STASTRUCT;
	joeSTASTRUCT->patient_name = "Joe Smith";
	joeSTASTRUCT->SBB = joeStats;
	STASTRUCT* janeSTASTRUCT = new STASTRUCT;
	janeSTASTRUCT->patient_name = "Jane Smith";
	janeSTASTRUCT->SBB = janeStats;
	STASTRUCT* johnSTASTRUCT = new STASTRUCT;
	johnSTASTRUCT->patient_name = "John Doe";
	johnSTASTRUCT->SBB = johnStats;

	// stats
	pthread_create(&stat_t[0], NULL, statistics_thread_func, (void*)joeSTASTRUCT);
	pthread_create(&stat_t[1], NULL, statistics_thread_func, (void*)janeSTASTRUCT);
	pthread_create(&stat_t[2], NULL, statistics_thread_func, (void*)johnSTASTRUCT);
	

	// wait for threads to finish
	usleep(1000000);

	/* Printing Histograms */
	int histWidth = 5; // number of columns that appear per histogram
	cout << endl << endl << "Joe Smith Histogram -- Frequency of Responses" << endl;
	for (int i = 0; i < 10; ++i) {
		int sum = 0;
		int start = i * 10;
		int end = start + 9;
		for (int j = 0; j < 10; ++j) {
			sum += joeHist[start + j];
		}
		cout << start << "-" << end << ": " << sum;
		if ((i+1) % histWidth == 0) {
			cout << endl;
		}
		else {
			cout << " \t";
		}
	}

	cout << endl << endl << "Jane Smith Histogram -- Frequency of Responses" << endl;
	for (int i = 0; i < 10; ++i) {
		int sum = 0;
		int start = i * 10;
		int end = start + 9;
		for (int j = 0; j < 10; ++j) {
			sum += janeHist[start + j];
		}
		cout << start << "-" << end << ": " << sum;
		if ((i + 1) % histWidth == 0) {
			cout << endl;
		}
		else {
			cout << " \t";
		}
	}

	cout << endl << endl << "John Doe Histogram -- Frequency of Responses" << endl;
	for (int i = 0; i < 10; ++i) {
		int sum = 0;
		int start = i * 10;
		int end = start + 9;
		for (int j = 0; j < 10; ++j) {
			sum += johnHist[start + j];
		}
		cout << start << "-" << end << ": " << sum;
		if ((i + 1) % histWidth == 0) {
			cout << endl;
		}
		else {
			cout << " \t";
		}
	}

	gettimeofday(&end, NULL); // get after time
	unsigned int tusec = end.tv_usec - begin.tv_usec;
	unsigned int tsec = end.tv_sec - begin.tv_sec;
	cout << endl << "Total request time was " << tusec << " microseconds, or around " << tsec << " seconds." << endl;

	usleep(100000);
}
