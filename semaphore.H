#ifndef _semaphore_H_
#define _semaphore_H_

#include <pthread.h>
#include <iostream>

using namespace std;

class Semaphore {
private:
	int value;
	pthread_mutex_t m;
	pthread_cond_t  c;

public:
	Semaphore(int _val) {
		value = _val;
		pthread_cond_init(&c, NULL);
		pthread_mutex_init(&m, NULL);
	};

	~Semaphore() {
		pthread_mutex_destroy(&m);
		pthread_cond_destroy(&c);
	};

	int P() {
		pthread_mutex_lock(&m);
		value--;
		if (value < 0) {
			pthread_cond_wait(&c, &m);
		}
		pthread_mutex_unlock(&m);
		return 0;
	};
	int V() {
		pthread_mutex_lock(&m);
		value++;
		if (value <= 0) {
			pthread_cond_signal(&c);
		}
		pthread_mutex_unlock(&m);
		return 0;
	};
};


#endif


