/******************************************************************
 * The Main program with the two functions. A simple
 * example of creating and using a thread is provided.
 ******************************************************************/

#include "helper.h"

/* Structure for jobs which are to be inserted in the circular quque */
struct job
{
	int job_id;
	int duration; //in seconds
};

/* Structure used to implement a circular queue */
struct circular_queue
{
	int head;
	int tail;
	int array_size;
	job *data;
};

/* Function prototype definitions */
void initializeQueue();
void *producer (void *id);
void *consumer (void *id);
int produce(int min, int max);
void deposit_item(job new_job);
job fetch_item();
void consume(int duration);
int initialize_required_semaphores();
void setup_variables(char **argv);

/* Global variable used for identifying semaphores and the semaphore id set */
int item = 0, space = 1, mutex = 2, sem_id;

/* Global variables used for operation of the consumers and producers */
int number_of_producers, number_of_consumers, jobs_per_producer, buffer_size;
circular_queue my_queue;

int main (int argc, char **argv)
{
  	int producer_id, consumer_id;

	//Used for seeding to avoid pseudo-random outputs.
	srand(time(NULL));

	//Verifications of number of arguments
	if(argc != 5)
	{
		cerr <<	"Incorrect number of arguments!" << endl;
		return INCORRECT_NUMBER_OF_ARGUMENTS;
	}
	
	//Verification of argument values
	for(int i = 1; i < argc; i++)
	{
		if(check_arg(argv[i]) == -1)
		{
			cerr << "Argument number " << i << " is not a valid number!" << endl;
			cerr << "Command line arguments are supposed to be positive integers" << endl;
			return NON_POSITIVE_INTEGER;
		}
	}

	//Generate a semaphore ID from a created set of semaphores
	sem_id = sem_create(SEM_KEY, 3);
	
	//Verify semaphores have been created correctly. Handle_semget_error returns an appropriate
	//message if the semaphore set wasn't created successfully.
	if (sem_id == -1)
	{
		print_semget_error (errno);
		return errno;
	}	

	setup_variables(argv);

	//Declaration for number of POSIX threads required for producers and consumers
	pthread_t producer_td[number_of_producers];
	pthread_t consumer_td[number_of_consumers];
	
	//Semaphore initialization which verifies for system call errors and if found the program outputs 
	//an appropriate message then closes the semaphore set.
	if (initialize_required_semaphores() != NO_ERROR)
	{
		print_semctl_error(errno);
		sem_close(sem_id);
		return errno;	
	}

	initializeQueue();
 
	//Create POSIX threads for producers
	for(producer_id = 0; producer_id < number_of_producers; producer_id++)
		pthread_create (&producer_td[producer_id], NULL, producer, (void *) (intptr_t) (producer_id + 1));
	
	//Create POSIX threads for consumers				
	for(consumer_id = 0; consumer_id < number_of_consumers; consumer_id++)
		pthread_create (&consumer_td[consumer_id], NULL, consumer, (void *) (intptr_t) (consumer_id + 1));

	//Wait for producer threads to terminate
	for(producer_id = 0; producer_id < number_of_producers; producer_id++)
		pthread_join (producer_td[producer_id], NULL);

	//Wait for consumer threads to terminate
	for(consumer_id = 0; consumer_id < number_of_consumers; consumer_id++)
		pthread_join (consumer_td[consumer_id], NULL);

	//Destroy semaphore set
	sem_close(sem_id);
	
	delete [] my_queue.data;

 	return NO_ERROR;
}

void *producer(void *id) 
{
	//Assign the producer ID and timeout state
	int producer_id = (intptr_t) id;
	bool timeout = false;
	job temp_job;

	//loop
	for(int i = 0; (i < jobs_per_producer); i++)
	{
		//produce job duration		
		int duration = produce(1, 10);

		//sleep 1-5 seconds before depositing job
		sleep(produce(1, 5));		

		//perform down operation on semaphore space and store timeout state
		timeout = sem_timed_wait (sem_id, space, 20);

		//if operation times out then break loop
		if (timeout)
		{
			printf("Producer(%d): terminated due to a timeout\n", producer_id);
			break;		
		}
	
		//perform down operation for mutex to protect the buffer
		sem_wait (sem_id, mutex);
		
		//assign job id based on queue tail and create new job with produced job id and duration
		temp_job.job_id = (my_queue.tail + 1);
		temp_job.duration = duration;
	
		//deposit job on the queue
		deposit_item(temp_job);

		//perform up operation for mutex and item semaphores
		sem_signal (sem_id, mutex);
		sem_signal (sem_id, item);

		//Output details of producer and the deposited job
		printf("Producer(%d): Job ID %d duration %d\n", producer_id, temp_job.job_id, temp_job.duration);
	}
	
	//if loop is broken without a timeout then output message
	if(!timeout)
		printf("Producer(%d): No more jobs to generate\n", producer_id);

	//close pthread
 	pthread_exit(0);
}

void *consumer (void *id) 
{
	//Assign consumer id
	int consumer_id = (intptr_t) id;
	job temp_job;

	//loop consumer until it times out due to no item being available for 20 seconds
	while(sem_timed_wait (sem_id, item, 20) == 0)
	{
		//perform down operation on mutex
		sem_wait (sem_id, mutex);

		//fetch job from queue
		temp_job = fetch_item();
	
		//perform up operation on mutex and space
		sem_signal (sem_id, mutex);
		sem_signal (sem_id, space);

		//print consumption status and details
		printf("Consumer(%d): Job ID %d executing sleep duration %d\n", consumer_id, temp_job.job_id, temp_job.duration);
		
		//perform job consumption (sleep for duration)
		sleep(temp_job.duration);
	
		//print consumption status after job completion
		printf("Consumer(%d): Job ID %d completed\n", consumer_id, temp_job.job_id);
	}

	//print message when loop is broken
	printf("Consumer(%d): No more jobs left\n", consumer_id);

	//close thread
	pthread_exit (0);
}

void setup_variables(char **argv)
{
	//Assigning data to the variables required for operation
	buffer_size = check_arg(argv[1]);
	jobs_per_producer = check_arg(argv[2]);
	number_of_producers = check_arg(argv[3]);
	number_of_consumers = check_arg(argv[4]);
}

/* Function used to initialize the circular buffer */
void initializeQueue()
{
	my_queue.head = 0;
	my_queue.tail = 0;
	my_queue.array_size = buffer_size;
	my_queue.data = new job[buffer_size];
}

/* Function used to deposit a job in the buffer and incrementing the queue tail */
void deposit_item(job new_job)
{
	my_queue.data[my_queue.tail] = new_job;
	my_queue.tail = ((my_queue.tail + 1) % buffer_size);	
}

/* Function used to fetch a job from the buffer and incrementing the queue head */
job fetch_item()
{	
	job myJob = my_queue.data[my_queue.head];
	my_queue.head = ((my_queue.head + 1) % buffer_size);
	
	return myJob;
}

/* Function used to produce a pseudo-random number between min and max. The seed is called in main */
int produce(int min, int max)
{
	int i = min + (rand() % max);
	return i;
}

int initialize_required_semaphores()
{
	if (sem_init (sem_id, item, 0))
	{
		cerr << "Error found in semaphore 'item' initialization due to: " << endl;
		return errno;
	} 
	else if (sem_init (sem_id, space, buffer_size))
	{
		cerr << "Error found in semaphore 'space' initialization due to: " << endl;
		return errno;
	}
	else if (sem_init (sem_id, mutex, 1))
	{
		cerr << "Error found in semaphore 'mutex' initialization due to: " << endl;
		return errno;
	}

	return NO_ERROR;
}
