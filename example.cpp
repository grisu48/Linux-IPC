/* =============================================================================
 * 
 * Title:         Example usage of the IPC modules (SharedMemory & Semaphore)
 * Author:        Felix Niederwanger
 * License:       Copyright (c), 2019 Felix Niederwanger
 *                MIT license (http://opensource.org/licenses/MIT)
 * 
 * =============================================================================
 */

#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#include "ipc.hpp"

// SharedMemory segments and semaphores use keys to identify them.
// Each process attaches to a given key, so it needs to be known to everyone
// and the key needs to be free. Check `ipcs` before.
#define IPC_KEY 0x823		// Use unique key

#define CHILDREN 8 			// Number of child processes


using namespace std;


/** Forks this process n times */
int fork_n(const int n) {
	for(int i=0;i<n;i++) {
		pid_t pid = fork();
		if (pid < 0) {
			cerr << "Fork failed" << endl;
			exit(EXIT_FAILURE);
		} else if(pid == 0) {
			// Child leaves here
			return (i+1);
		} else {
			// Parent goes on.
		}
	}
	return 0;		// Only parent had id 0
}

double sum(const double *a, const size_t n) {
	double r = 0;
	for(size_t i=0;i<n;i++)
		r += a[i];
	return r;
}

int main() { //int argc, char** argv) {
    const int child_id = fork_n(CHILDREN);
    // Ok, here we have now 8 processes that all execute the same
    
    //cout << "child " << child_id << endl;
    
    
    /* ==== Example section for Semaphore =================================== */
     
    Semaphore sem(IPC_KEY);
    sem.setValue(0);
    
    // Parent waits 1 second, and then releases the semaphore for the children
    if(child_id == 0) {
	    cout << "Parent releases semaphore in 1 second ... " << endl;
	    sleep(1);
	    sem.release(1);
    } else {
    	sem.aquire(1);
    	cout << "Child " << child_id << " has the semaphore now" << endl;
    	//sleep(1);
    	sem.release(1);
    }
    
    
    /* ==== Example section for shared memory =============================== */
    SharedMemory shm(IPC_KEY, sizeof(double)*CHILDREN);
    double *array = (double*)shm.get();			// *operator is also fine
    array[child_id] = child_id;
    
    // Periodically check memory until everyone has set it's values
    bool shm_ok = false;
    do {
    	shm_ok = true;
    	for(int i=0;i<CHILDREN;i++) {
    		if(array[i] != i) {
    			shm_ok = false;
    			break;
    		}
    	}
    } while(!shm_ok);
    cout << "Child " << child_id << " array sum (shm) = " << sum(array, CHILDREN) << endl;
    
    
    // Parent waits for children
    if(child_id == 0) {
		for(int i=0;i<CHILDREN;i++) {
			int status;
			pid_t pid = wait(&status);
			if (status != 0) {
				cerr << "Child " << pid << " terminated with status " << status << endl;
			}
		}
		sem.destroy();		// Semaphore needs to be destroyed manually by parent
		cout << "Bye" << endl;
	}
    return EXIT_SUCCESS;
}

