#include <bits/stdc++.h>
#include <sys/time.h>
#include "thread.h"
#include <list>
using namespace std;
// ----------------------------------------------------------------------------------
#define RUNNING 1
#define CANCEL 2
#define COMPLETE 3
struct my_thread
{
    thread_t pid;
    int status;
    void *(*function)(void *);
    void *arg;
    void *return_value;
    ucontext_t *context;
    thread_t joining_thread;
};
thread_t cnt = 1;
sigset_t alarm_set;
static struct itimerval timer;
struct sigaction act;
void alarm_handler(int sig);
my_thread *get_mythread(thread_t pid);
my_thread *curr;
void wrapper_function(void *(*routine)(void *), void *args);
list<my_thread *> ready_queue, zombie_queue;
// ----------------------------------------------------------------------------------

void thread_init(int period)
{

    my_thread *t = (my_thread *)malloc(sizeof(my_thread));
    t->pid = cnt++;
    t->context = (ucontext_t *)malloc(sizeof(ucontext_t));
    t->status = RUNNING;
    t->joining_thread = -1;
    t->arg = NULL;
    t->return_value = NULL;

    int p = getcontext(t->context);
    if (p == -1)
    {
        perror("failure in getcontext");
        exit(EXIT_FAILURE);
    }
    //setting timer
    /*
        struct itimerval {
               struct timeval it_interval;
               struct timeval it_value;    
           };

           struct timeval {
               time_t      tv_sec;         
               suseconds_t tv_usec;        
           };
    */

    timer.it_interval.tv_sec = period;
    timer.it_interval.tv_usec = 0;
    timer.it_value.tv_sec = period;
    timer.it_value.tv_usec = 0;

    int d = setitimer(ITIMER_VIRTUAL, &timer, NULL);
    if (d == -1)
    {
        perror("setitimer");
        exit(EXIT_FAILURE);
    }

    //signal handling
    sigemptyset(&alarm_set);
    sigaddset(&alarm_set, SIGVTALRM);
    sigprocmask(SIG_UNBLOCK, &alarm_set, NULL); //unblock if it is already blocked
    // signal handler on expiring timer
    act.sa_handler = &alarm_handler;

    int s = sigaction(SIGVTALRM, &act, NULL);

    if (s < -1)
    {
        perror("error in sigaction");
        exit(EXIT_FAILURE);
    }

    curr = t;
}

int thread_create(thread_t *pid, void *(*routine)(void *), void *args)
{
    //disable signal
    sigprocmask(SIG_BLOCK, &alarm_set, NULL);

    my_thread *t = (my_thread *)malloc(sizeof(my_thread));
    *pid = cnt++;
    t->pid = *pid;
    t->status = RUNNING;
    t->function = routine;
    t->arg = args;
    t->context = (ucontext_t *)malloc(sizeof(ucontext_t));
    t->joining_thread = -1;

    int p = getcontext(t->context);
    if (p == -1)
    {
        perror("failure in getcontext");
        exit(EXIT_FAILURE);
    }

    t->context->uc_stack.ss_sp = malloc(SIGSTKSZ);
    t->context->uc_stack.ss_size = SIGSTKSZ;
    t->context->uc_stack.ss_flags = 0;
    t->context->uc_link = NULL;

    makecontext(t->context, (void (*)(void))wrapper_function, 2, routine, args);
    ready_queue.push_back(t);
    //enable signal
    sigprocmask(SIG_UNBLOCK, &alarm_set, NULL);

    return 0; //on success return 0
}

int thread_join(thread_t *pid, void **stat)
{
    if (curr->pid == *pid)
        return -1;

    my_thread *t = get_mythread(*pid);
    if (!t)
        return -1;

    if (t->joining_thread == curr->pid)
        return -1;

    curr->joining_thread = t->pid;

    while (t->status == RUNNING)
    {
        sigprocmask(SIG_UNBLOCK, &alarm_set, NULL);
        alarm_handler(SIGVTALRM);
        sigprocmask(SIG_BLOCK, &alarm_set, NULL);
    }

    if (stat == NULL)
        return 0;

    if (t->status == CANCEL)
        *stat = (void *)CANCEL;
    else if (t->status == COMPLETE)
        *stat = t->return_value;

    return 0;
}

void wrapper_function(void *(*routine)(void *), void *args)
{
    sigprocmask(SIG_UNBLOCK, &alarm_set, NULL);

    curr->return_value = (*routine)(args);

    thread_exit(curr->return_value);
}

my_thread *get_mythread(thread_t pid)
{

    for (auto t : ready_queue)
    {
        if (t->pid == pid)
        {
            return t;
        }
    }

    for (auto t : zombie_queue)
    {
        if (t->pid == pid)
        {
            return t;
        }
    }

    return NULL;
}

void thread_exit(void* return_value){
	//block signal
	sigprocmask(SIG_BLOCK , &alarm_set ,NULL);

	if(ready_queue.empty()){
		sigprocmask(SIG_UNBLOCK , &alarm_set , NULL);
		exit(0);
	}

	if(curr -> pid == 1){	//main thread calls exit
		while(!ready_queue.empty()){
			sigprocmask(SIG_UNBLOCK , &alarm_set , NULL);
			alarm_handler(SIGVTALRM);
			sigprocmask(SIG_BLOCK , &alarm_set , NULL);
		}
		sigprocmask(SIG_UNBLOCK , &alarm_set , NULL);
		exit(0);
	}

	my_thread* previous = curr;
	curr = (my_thread*) (ready_queue.front());
    ready_queue.pop_front();
	curr -> status = RUNNING;

	free(previous->context->uc_stack.ss_sp);
	free(previous->context);
	previous->context = NULL;

	//mark previous as done
	previous -> status = COMPLETE;
	previous -> return_value = return_value;
	previous -> joining_thread = 0;

	zombie_queue.push_back(previous);

	//unblock signal
	sigprocmask(SIG_UNBLOCK , &alarm_set , NULL);
	setcontext(curr -> context);	
}

int thread_kill(thread_t thread){
	
	//check if thread is killing itself  in such case block
	if(thread == curr -> pid){
		thread_exit(0);
	}

	//block signal
	sigprocmask(SIG_BLOCK , &alarm_set , NULL);

	my_thread *t = get_mythread(thread);

	if(t == NULL){//not found
		sigprocmask(SIG_UNBLOCK , &alarm_set , NULL);
		return -1;
	}

	if(t -> status == COMPLETE){
		sigprocmask(SIG_UNBLOCK , &alarm_set , NULL);
		return -1;
	}

	if(t -> status == CANCEL){//already cancelled thread
		sigprocmask(SIG_UNBLOCK , &alarm_set , NULL);
		return -1;
	}
	else t -> status = CANCEL;

	//free memory
	free(t->context->uc_stack.ss_sp);
	free(t->context);
	t->context = NULL;
	t->joining_thread = -1;
	zombie_queue.push_back(t);

	//unblock signal
	sigprocmask(SIG_UNBLOCK , &alarm_set , NULL);
	return 0;
}


//alarm handler is nothing but scheduler
void alarm_handler(int signal){
	//block signal
	sigprocmask(SIG_BLOCK , &alarm_set , NULL);


	if(ready_queue.empty()){
		return;
	}
	my_thread* previous = curr;
	ready_queue.push_back(curr);

    my_thread* next =(my_thread*) ready_queue.front();
	ready_queue.pop_front();

	while(next -> status == CANCEL){
		zombie_queue.push_back(next);

		if(ready_queue.empty())
			return;
		
        next =(my_thread*) ready_queue.front();
		ready_queue.pop_front();
	}

	next -> status = RUNNING;
	curr = next;


	//unblock signal
	sigprocmask(SIG_UNBLOCK , &alarm_set , NULL);
	swapcontext(previous -> context , curr -> context);
}