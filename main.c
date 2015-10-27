#include <stdio.h> // getline()
#include <string.h> // strcmp()
#include <pthread.h> // all
#include <semaphore.h> // sem_init()

#include "commandlinereader.h"
#include "par_run.h"
#include "monitor.h"
#include "list.h"
#include "main.h"

#define CHILD_ARGV_SIZE 20
#define MAXPAR 8

//*********** BEGIN GLOBAL VARIABLES ***********/

static unsigned int children_count = 0; // counts all children successfully forked
static unsigned int waited_children = 0; // counts all children succesffully waited on

static list_t* children_list; // initialized in main

static bool exit_called = false; // true after user inputs enter

static pthread_mutex_t list_mutex; // initialized in main
static pthread_mutex_t exit_called_mutex;
static pthread_mutex_t waited_children_mutex;
static pthread_mutex_t children_count_mutex;

static pthread_t thread_monitor;

sem_t can_wait; // semaphore for being able to wait on children; initialized in main
sem_t can_fork; // semaphore for being able to fork children; initialized in main

//*********** END GLOBAL VARIABLES ***********/

inline void atomic_insert_new_process(int pid, time_t starttime)
{ 
	pthread_mutex_lock(&list_mutex);	
	insert_new_process(children_list, pid, starttime); 
	pthread_mutex_unlock(&list_mutex);	
}

inline void atomic_update_terminated_process(int pid, time_t endtime)
{ 
	
	pthread_mutex_lock(&list_mutex);
	update_terminated_process(children_list, pid, endtime);
	pthread_mutex_unlock(&list_mutex);
}

inline void atomic_inc_children_count(void)
{
	pthread_mutex_lock(&children_count_mutex);	
	++children_count;
	pthread_mutex_unlock(&children_count_mutex);
}

inline void atomic_inc_waited_children(void)
{
	pthread_mutex_lock(&waited_children_mutex);
	++waited_children;
	pthread_mutex_unlock(&waited_children_mutex);
}

inline bool atomic_get_exit_called(void)
{
	pthread_mutex_lock(&exit_called_mutex);
	bool exit_called_l = exit_called;
	pthread_mutex_unlock(&exit_called_mutex);
	return exit_called_l;
}

inline void atomic_set_exit_called(bool b)
{
	pthread_mutex_lock(&exit_called_mutex);	
	exit_called = b;
	pthread_mutex_unlock(&exit_called_mutex);	
}


int main(int argc, char* argv[]) 
{	
	children_list = lst_new(); // see above

	pthread_mutex_init(&children_count_mutex, NULL);
	pthread_mutex_init(&waited_children_mutex, NULL);
	pthread_mutex_init(&exit_called_mutex, NULL);
	pthread_mutex_init(&list_mutex, NULL);

	char* argv_child[CHILD_ARGV_SIZE]; // argv passed to forked child. 

	sem_init(&can_fork, 0, MAXPAR);	// semaphore, mode, count 

	sem_init(&can_wait, 0, 0); //ditto

	if (pthread_create(&thread_monitor, NULL, monitor, NULL)) // multi-threading starts here
		perror("par-shell: Couldn't create monitoring thread. Will not be able to monitor and wait for children.");

	printf("Par-shell now ready. Does not wait for jobs to exit!\n>>> "); 

	for(;;) // breaks upon "exit" input
	{

		switch (readLineArguments(argv_child, CHILD_ARGV_SIZE)) 
		// switch mais lindo do mundo. Nao me interessa que diz o Rafael!
		{
			case -1: perror("par-shell: couldn't read input");
			case 0: continue;
		}

		if (!strcmp(argv_child[0], "exit")) // user asks to exit
		{		
			sem_post(&can_wait);			
			atomic_set_exit_called(true);

			if (pthread_join(thread_monitor, NULL))
				perror("par-shell: couldn't join with monitor thread");

			break;
		}

		else par_run(argv_child);
	}

	lst_print(children_list); 
	lst_destroy(children_list);		
	pthread_mutex_destroy(&children_count_mutex);
	pthread_mutex_destroy(&waited_children_mutex);
	pthread_mutex_destroy(&exit_called_mutex);
	pthread_mutex_destroy(&list_mutex);
	sem_destroy(&can_fork);
	sem_destroy(&can_wait);

	return EXIT_SUCCESS;
	
}
