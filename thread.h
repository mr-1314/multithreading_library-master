#include <ucontext.h>

typedef long int thread_t;

void thread_init(int period);
int thread_create(thread_t *pid, void *(*routine)(void *), void *args);
int thread_join(thread_t *pid, void **stat);
void thread_exit(void* return_value);
int thread_kill(thread_t thread);
