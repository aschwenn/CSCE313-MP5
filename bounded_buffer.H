#include <string>
#include <queue>
#include <iostream>
#include "semaphore.H"

using namespace std;

class BoundedBuffer {
private:
	Semaphore* full;
	Semaphore* empty; // counts the number of slots in buffer
	pthread_mutex_t lock;
	queue<string> buffer;
	int size;
public:
	BoundedBuffer(int sz) { // initialize/allocate
		size = sz;
		full = new Semaphore(0);
		empty = new Semaphore(size);
		pthread_mutex_init(&lock, NULL);
	};
	~BoundedBuffer() { // deallocate
		delete full;
		delete empty;
		pthread_mutex_destroy(&lock);
	};
	void Deposit(string r) {
		empty->P(); // decrement empty
		pthread_mutex_lock(&lock); // CRITICAL SECTION
		buffer.push(r);
		pthread_mutex_unlock(&lock); // END CRITICAL SECTION
		full->V();
	};
	string Remove() {
		full->P();
		pthread_mutex_lock(&lock); // CRITICAL SECTION
		string r = buffer.front();
		buffer.pop(); // remove first
		pthread_mutex_unlock(&lock); // END CRITICAL SECTION
		empty->V();
		return r; // return removed RTASTRUCT
	};
};