#include "helper.h"
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/mman.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>


int num_threads = 0; // takes as argv[1]
char* log_file_id; // takes as argv[2]
int buffer_size = 0; // num_thread*2
char **buffer;

bool end_producer = false;

// summary
int work = 0;
int sleep_num = 0;
int ask = 0;
int receive = 0;
int complete = 0;
int *track_thread;
double transaction_per_second;
int qWork = 0;

// time
struct timeval start;
FILE* flog;

// threads
pthread_mutex_t mutex;
sem_t full, empty;

// Queue implementation
int front = -1;
int rear = -1;


// checks if the queue is full or not
int isFull(){
// https://www.programiz.com/dsa/circular-queue
	if ((front == rear +1) || ((front == 0) && (rear == buffer_size -1))){
		return 1;
	}
	return 0;
}


// checks if the queue is empty or not
int isEmpty(){
// https://www.programiz.com/dsa/circular-queue
	if (front == -1){
		return 1;
	}
	return 0;
}


// add transaction works to the queue
void enQueue (char* input_line){
// https://www.programiz.com/dsa/circular-queue
	if (front == -1){
		front = 0;
	}
	rear = (rear + 1)%buffer_size;
	buffer[rear] = input_line;
}


// removes transaction works from the queue and if queue is empty, it returns the letter E
char *deQueue() {
// https://www.programiz.com/dsa/circular-queue
  char *element;
  	if (isEmpty()){
    	return ("E");
  	}else {
    	element = buffer[front];
    	if (front == rear) {
      		front = -1;
      		rear = -1;
    	}
    	else {
      		front = (front + 1) % buffer_size;
    	}
    	return (element);
  	}
}


/*
	producer reads from stdin or input file line by line till it reaches EOF. If the first letter in each line is T it adds the transaction work
	to the queue. If the first letter in a line is S, the producer sleeps by calling Sleep() function
	I used sem_wait and sem_post as wait and signal functions. pthread_mutex_lock() and pthread_mutex_unlock() were used to protect
	the critical section and make its operations atomic. Adding transaction works to the queue is placed inside the critical section.
*/
void producer_thread(void *pno){
	// https://stackoverflow.com/questions/2125219/how-to-get-the-running-of-time-of-my-program-with-gettimeofday
	// https://medium.com/@sohamshah456/producer-consumer-programming-with-c-d0d47b8f103f

	int *pthread = (int*)pno;
	double ptime_taken;
	// time points for work and sleep in producer
	struct timeval prod_stop, sleep_stop, end_stop; 

	// readline
	char* token;
	token = malloc(1024 * sizeof(char));
	char *endptr;
	long l;
	while (!feof(stdin)){
		char* token;
		token = malloc(1024 * sizeof(char));

		if (fgets(token, 1024, stdin)){
			strtok(token, "\n");
			l = strtol(token+1, &endptr, 10);
		}

		// if the token is S, the producer call Sleep( int n)
		if (token[0] == 'S'){
			int stime = (int) l;
			Sleep(stime);
			gettimeofday(&sleep_stop, NULL);

			ptime_taken = (double) ((sleep_stop.tv_sec - start.tv_sec) + ((sleep_stop.tv_usec - start.tv_usec)/1000000.0));
			print_output(ptime_taken, pthread, 1, "Sleep", stime);
			sleep_num ++; // increments the number of sleeps in input		
		}

		sem_wait(&empty); // wait for queue to become empty, using semaphore
		pthread_mutex_lock(&mutex); // acquire the buffer

		// if the token starts with T, producer add the work to the queue
		if (token[0] == 'T'){
			int wtime = (int) l;
			enQueue(token);
			gettimeofday(&prod_stop, NULL);
			ptime_taken = (double) ((prod_stop.tv_sec - start.tv_sec) + ((prod_stop.tv_usec - start.tv_usec)/1000000.0));
			qWork ++;
			print_output(ptime_taken, pthread, qWork, "Work", wtime);
			work++;		
		}
		pthread_mutex_unlock(&mutex); // release buffer
		sem_post(&full); // signal semaphore, let consumer know that the mutex lock is released so you can consume the data from the buffer
	}

	gettimeofday(&end_stop, NULL);
	ptime_taken = (double) ((end_stop.tv_sec - start.tv_sec) + ((end_stop.tv_usec - start.tv_usec)/1000000.0));
	print_output(ptime_taken, pthread, 1, "End", 1);
	sleep(1); // sleeps for the consumers to catch up reading from the queue
	end_producer = true; // producer finishes adding work to the buffer
	sem_post(&full);
}


/*
	consumer threads read transaction works from the queue. If the queue was not empty, they call Trans function.
	I used sem_wait() and sem_post() as wait() and signal() functions. pthread_mutex_lock() and pthread_mutex_unlock() were used to protect
	the critical section and make its operations atomic. Removing the transaction works from the queue is placed inside the critical section.
	The consumer threads break out of the loop when both conditions of queue being empty and producer finished adding job to the queue are satisfied.
*/
void *consumer_thread(void *cno){
	// https://stackoverflow.com/questions/2125219/how-to-get-the-running-of-time-of-my-program-with-gettimeofday
	// https://medium.com/@sohamshah456/producer-consumer-programming-with-c-d0d47b8f103f

	char *consumer_item;
	int *thread = (int*)cno;
	
	double ctime_taken;
	char* ttime;
	int transaction_time;
	// time points for ask, receive and complete work by consumer threads
	struct timeval ask_stop, receive_stop, complete_stop;

	while (true){
		gettimeofday(&ask_stop, NULL);
		ctime_taken = (double) ((ask_stop.tv_sec - start.tv_sec) + ((ask_stop.tv_usec - start.tv_usec)/1000000.0));
		print_output(ctime_taken, thread, 1, "Ask", 1);
		ask ++;
		sem_wait(&full); // wait for queue to become full, using semaphore

		// checks if the queue is empty and producer finished adding work to the queue
		if (isEmpty() && end_producer){
			break;
		}
		pthread_mutex_lock(&mutex); // acquire the buffer
		consumer_item = deQueue();
		if (consumer_item != "E"){
			ttime = &consumer_item[1];
			transaction_time = (int) atoi(ttime);
			gettimeofday(&receive_stop, NULL);
			ctime_taken = (double) ((receive_stop.tv_sec - start.tv_sec) + ((receive_stop.tv_usec - start.tv_usec)/1000000.0));
			qWork --;
			receive ++;
			print_output(ctime_taken, thread, qWork, "Receive",transaction_time);
		}
		pthread_mutex_unlock(&mutex); // release buffer
		sem_post(&empty); // signal semaphore, let consumer know that the mutex lock is released so you can consume the data from the buffer
		
		if (consumer_item != "E"){	
			Trans(transaction_time);
			gettimeofday(&complete_stop, NULL);
			ctime_taken = (double) ((complete_stop.tv_sec - start.tv_sec) + ((complete_stop.tv_usec - start.tv_usec)/1000000.0));
			transaction_per_second = ctime_taken;
			print_output(ctime_taken, thread, 1, "Complete",transaction_time);
			complete ++;
			int index = (int) (*thread-1);
			track_thread[index] = (track_thread[index] + 1);
		}
	}
	sem_post(&full);
}


// prints the logs for Ask, Receive, Complete, Work and Sleep into the output file
void print_output(double time, int* tid, int ntransaction, char* event, int token_time){

	char buffer_out[1024];
	int* thread_id = tid;
	int q_id = ntransaction;
	if ((event == "Ask") || (event == "End")){
		sprintf(buffer_out, "   %0.3f ID= %d        %s         \n", time, *thread_id, event);
	}else if ((event == "Sleep") || (event == "Complete")){
		sprintf(buffer_out, "   %0.3f ID= %d        %s      %d\n", time, *thread_id, event, token_time);
	}else{
		sprintf(buffer_out, "   %0.3f ID= %d  Q= %d  %s       %d\n", time, *thread_id, q_id, event, token_time);
	}
	fwrite(buffer_out, sizeof(char), strlen(buffer_out), flog);
}


// prints the summary after logs to show the activities of producer and consumer threads
void print_summary(){

	char buffer_summary[1024];
	sprintf(buffer_summary, "Summary:\n");
	fwrite(buffer_summary, sizeof(char), strlen(buffer_summary), flog);
	sprintf(buffer_summary, "   Work          %d\n", work);
	fwrite(buffer_summary, sizeof(char), strlen(buffer_summary), flog);
	sprintf(buffer_summary, "   Ask           %d\n", ask);
	fwrite(buffer_summary, sizeof(char), strlen(buffer_summary), flog);
	sprintf(buffer_summary, "   Receive       %d\n", receive);
	fwrite(buffer_summary, sizeof(char), strlen(buffer_summary), flog);
	sprintf(buffer_summary, "   Complete      %d\n", complete);
	fwrite(buffer_summary, sizeof(char), strlen(buffer_summary), flog);
	sprintf(buffer_summary, "   Sleep         %d\n", sleep_num);
	fwrite(buffer_summary, sizeof(char), strlen(buffer_summary), flog);
	for ( int i = 0; i < num_threads; i++){
		sprintf(buffer_summary, "   Thread   %d   %d\n", i+1, track_thread[i]);
		fwrite(buffer_summary, sizeof(char), strlen(buffer_summary), flog);
	}
	double tran_per_sec = (double)(work/transaction_per_second);
	sprintf(buffer_summary, "Transactions per second:         %0.3f\n", tran_per_sec);
	fwrite(buffer_summary, sizeof(char), strlen(buffer_summary), flog);

}


// Initialize mutex and semaphores
void initialize(){
	pthread_mutex_init(&mutex, NULL);
	sem_init(&full, 0, 0);
	sem_init(&empty, 0, buffer_size);
}


int main(int argc, char** argv){

	char output_name[1024];
	num_threads = (int) atoi(argv[1]);
	buffer_size = num_threads * 2;

	buffer = malloc(buffer_size * sizeof(char*));
	if (!buffer){
		fprintf(stderr, "buffer allocation faild for tokenization\n");
		exit(EXIT_FAILURE);
	}

	// takes the log id, if the user enters a number for log id
	if (argc > 2){
		log_file_id = argv[2];
		if (atoi(log_file_id) > 0){
			strcpy(output_name, "prodcon.");
			strcat(output_name, log_file_id);
			strcat(output_name,".log");
		}else if (atoi(log_file_id) == 0){
			strcpy(output_name, "prodcon.");
			strcat(output_name,"log");
		}
	}

	// if the user does not enter a log id
	if (argc == 2){
		strcpy(output_name, "prodcon.");
		strcat(output_name,"log");
	}

	// opens the log file to write the output to
	// https://beginnersbook.com/2014/01/c-file-io/
	flog= fopen(output_name, "w");

	if (flog== NULL){
    	fprintf(stderr,"Issue in opening the Output file\n");
    }


    // threads
	pthread_t consumer[num_threads];
	initialize();

	int parray[1];
	parray[0] = 0;


	// to keep track of the number of works each thread completed
	track_thread = malloc(num_threads * sizeof(int));
	if (!track_thread){
		fprintf(stderr, "buffer allocation faild for track_thread\n");
		exit(EXIT_FAILURE);
	}

	int array[num_threads];
	for ( int i = 0; i < num_threads ; i++){
		array[i] = i+1;
	}

	// start point to record the time
	gettimeofday(&start, NULL);

	// creating consumer threads

	for (int i = 0; i < num_threads; i++){
		pthread_create(&consumer[i], NULL, (void *) consumer_thread, (void*)&array[i]);
	}

	// calling producer to add work to the queue
	producer_thread((void*)&parray[0]);

	for (int i = 0; i < num_threads ; i++){
		pthread_join(consumer[i], NULL);
	}

	//printf("reached main\n");

	print_summary();
	fclose(flog);

	// destroying the semaphores and mutex
	pthread_mutex_destroy(&mutex);
	sem_destroy(&empty);
	sem_destroy(&full);

	return 0;
}
