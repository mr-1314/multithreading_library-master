#include<bits/stdc++.h>
#include "thread.h"
#include <unistd.h>
using namespace std;

void* sum(void *args) {
	cout << "Hello, Sanket here!" << endl;

	for(int i = 0; i < 1e9; i++) {
		;
	}
	cout << "Hello, hrishikesh here!" << endl;
}

int main() {

	thread_init(1);

	thread_t t1, t2;

	thread_create(&t1, sum, NULL);
	thread_create(&t2, sum, NULL);
	void **stat;
	thread_join(&t1, stat);
	thread_join(&t2, stat);
	return 0;
}