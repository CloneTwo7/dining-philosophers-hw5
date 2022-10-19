#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/sem.h>
#include <time.h>

#define PHIL_NUM 5
#define EAT_TIME 10
int randomGaussian(int mean, int stddev);

int main(int argc, char *argv[]) {
	int parent = getpid();
	int pid = parent;
	/* Initializes PHIL_NUM semaphores representing each chopstick */
	int semID = semget(IPC_PRIVATE, PHIL_NUM, IPC_CREAT | IPC_EXCL | 0600);
	if(semID == -1) {
		perror("");
		return(1);
	}
	for(int i = 0; i < PHIL_NUM; i++) {
		if(semctl(semID, i, SETVAL, 1) == -1) {
			perror("");
			return 1;
		}
	}

	//Generates PHIL_NUM child processes
	int pNum = 0;
	for(pNum; pNum < PHIL_NUM; pNum++) {
		if(fork() == 0) {
			pid = getpid();
			srand(pid);
			break;
		}
	}

	int totalEating = 0;
	if(pid == parent) { /*waits for PHIL_NUM child processes to be done*/
		for(int i = 0; i < PHIL_NUM; i++) {
			wait(NULL);
		}
		semctl(semID, 0, IPC_RMID);
		printf("%d out of %d processes returned\n", PHIL_NUM, PHIL_NUM);
		printf("No deadlock occured\n");
	} else { //Child processes eat for EAT_TIME seconds or more
		/*sembuf declaration for picking up the chopsticks */
		struct sembuf pickup[2] = {{pNum, -1, 0}, {(pNum+1) % PHIL_NUM, -1, 0}};
		/*sembuf declaration for putting down the chopsticks */
		struct sembuf down[2] = {{pNum, 1, 0}, {(pNum+1) % PHIL_NUM, 1, 0}};
		while(1) { /*philosophers eat and think*/
			/*atomically pick up two chopsticks*/
			if(semop(semID, pickup, 2) == -1) {
				perror("Process Failed");
				return(1);
			}
			int delta = randomGaussian(9,3);
			printf("Philosopher %d\teating for %ds\t\n", pNum, delta);
			fflush(stdout);
			sleep(delta);
			totalEating += delta;
			/*Atomically put down two chopsticks*/
			if(semop(semID, down, 2) == -1) {	
				perror("Process Failed");
				return(1);
			}
			printf("Philosopher %d is thinking....\n", pNum);
			fflush(stdout);
			/*increment total eating time*/
			if(totalEating >= EAT_TIME) {
				printf("Philosopher %d is done with meal - ", pNum);
				printf("Total eating time: %d\n", totalEating);
				break;
			}
		}
	}
	return 0;
}

/* Copy/Pasted randomGaussian() function provided by Professor */
int randomGaussian(int mean, int stddev) {
	double mu = 0.5 + (double) mean;
	double sigma = fabs((double) stddev);
	double f1 = sqrt(-2.0 * log((double) rand() / (double) RAND_MAX));
	double f2 = 2.0 * 3.1415926359 * (double) rand() / (double) RAND_MAX;
	if( rand() & (1 << 5) ) {
		return (int) floor(mu + sigma * cos(f2) * f1);
	} else {
		return (int) floor(mu + sigma * sin(f2) * f1);
	}
}
