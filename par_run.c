#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h> //XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX e necessario aqui tambem

#include "par_run.h" // self
#include "main.h" // children_list, children_count, can_fork
#include "list.h" // insert_new_process


void par_run(char* argVector[]) 
{
	pthread_mutex_lock(&fork_mutex);					//XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
	while (can_fork == 0) pthread_cond_wait(&fork_cond,&fork_mutex);	//XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
	can_fork--;								//XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
	//sem_wait(&can_fork);							//XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX
	pthread_mutex_unlock(&fork_mutex);					//XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX XXX

	pid_t pid = fork();

	if (pid == -1)  //is parent; if child is created unsucessfully
		perror("par-shell: unable to fork");		

	else if (pid == 0) // is child
	{
		execv(argVector[0], argVector);

		if (argVector[0][0] != '/')		
			execvp(argVector[0], argVector);

		// next line only reached if execv fails.
		perror("par-shell: exec failed");
		exit(EXIT_FAILURE);
	}

	else // is parent; if child is created successfully
	{
		atomic_inc_children_count();
		atomic_insert_new_process(pid, time(NULL));
		sem_post(&can_wait);
	}

}

