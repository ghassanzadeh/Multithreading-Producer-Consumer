#ifndef DECLARATIONS
#define DECLARATIONS

// functions declarations
void Trans( int n );
void Sleep( int n );
void producer_thread(void *pno);
void *consumer_thread(void *cno);
int isFull();
int isEmpty();
void enQueue (char* input_line);
char *deQueue();
void initialize();
void print_summary();
void print_output(double time, int* tid, int ntransaction, char* event, int token_time);
#endif