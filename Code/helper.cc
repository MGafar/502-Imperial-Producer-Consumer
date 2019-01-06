/******************************************************************
 * The helper file that contains the following helper functions:
 * check_arg - Checks if command line input is a number and returns it
 * sem_create - Create number of sempahores required in a semaphore array
 * sem_init - Initialise particular semaphore in semaphore array
 * sem_wait - Waits on a semaphore (akin to down ()) in the semaphore array
 * sem_signal - Signals a semaphore (akin to up ()) in the semaphore array
 * sem_close - Destroy the semaphore array
 ******************************************************************/

# include "helper.h"

int check_arg (char *buffer)
{
  int i, num = 0, temp = 0;
  if (strlen (buffer) == 0)
    return -1;
  for (i=0; i < (int) strlen (buffer); i++)
  {
    temp = 0 + buffer[i];
    if (temp > 57 || temp < 48)
      return -1;
    num += pow (10, strlen (buffer)-i-1) * (buffer[i] - 48);
  }
  return num;
}

int sem_create (key_t key, int num)
{
  int id;
  if ((id = semget (key, num,  0666 | IPC_CREAT | IPC_EXCL)) < 0)
    return -1;
  return id;
}

int sem_init (int id, int num, int value)
{
  union semun semctl_arg;
  semctl_arg.val = value;
  if (semctl (id, num, SETVAL, semctl_arg) < 0)
    return -1;
  return 0;
}

void sem_wait (int id, short unsigned int num)
{
  struct sembuf op[] = {
    {num, -1, SEM_UNDO}
  };
  semop (id, op, 1);
}

void sem_signal (int id, short unsigned int num)
{
  struct sembuf op[] = {
    {num, 1, SEM_UNDO}
  };
  semop (id, op, 1);
}

int sem_close (int id)
{
  if (semctl (id, 0, IPC_RMID, 0) < 0)
    return -1;
  return 0;
}

int sem_timed_wait (int id, short unsigned int num, int time_delay)
{
  struct sembuf op[] = {
    {num, -1, SEM_UNDO}
  };
	
	struct timespec timeout;
	timeout.tv_nsec = 0;
	timeout.tv_sec = time_delay;

 	int error = semtimedop (id, op, 1, &timeout);
	
	return error;
}

/* The following error messages were obtained from the linux manual page for semget(2)*/
void print_semget_error(int error)
{
	switch(error) 
	{
		case EACCES:
			cerr << "A semaphore set exists for key, but the calling process does not have permission to access the set, and does not have the CAP_IPC_OWNER capability in the user namespace that governs its IPC namespace." << endl;
			cerr << "Please use a different key in the 'helper.h' file " << endl;
			break;
		
		case EEXIST:
			cerr << "IPC_CREAT and IPC_EXCL were specified in semflg, but a semaphore set already exists for key. " << endl;
			cerr << "Please verify status of semaphores using command line tools such as 'ipcs' and use 'ipcrm' for debugging. If the semaphore key is already in use by another user please change it in the 'helper.h' file." << endl;
			break;

		case ENOMEM:
			cerr << "A semaphore set has to be created but the system does not have enough memory for the new data structure." << endl;
			break;

		case ENOSPC:
			cerr << "A semaphore set has to be created but the system limit for the maximum number of semaphore sets (SEMMNI), or the system wide maximum number of semaphores (SEMMNS), would be exceeded." << endl;
			break;

		case EINVAL:
			cerr << "nsems is less than 0 or greater than the limit on the number of semaphores per semaphore set (SEMMSL)" << endl;
			break;

		case ENOENT:
			cerr << "No semaphore set exists for key and semflg did not specify IPC_CREAT." << endl;
			break;
	}
}

/* The following error messages were obtained from the linux manual page for semctl(4)*/
void print_semctl_error(int error)
{
	switch(error)
	{
		case EACCES:
			cerr << "The argument cmd has one of the values GETALL, GETPID, GETVAL, GETNCNT, GETZCNT, IPC_STAT, SEM_STAT, SETALL, or SETVAL and the calling process does not have the required permissions on the semaphore set and does not have the CAP_IPC_OWNER capability in the user namespace that governs its IPC namespace." << endl;
			break;
		
		case EFAULT:
			cerr << "The address pointed to by arg.buf or arg.array isn't accessible." << endl;
			break;
	
		case EIDRM:
			cerr << "The semaphore set was removed." << endl;
			break;
	
		case EINVAL:
			cerr << "Invalid value for cmd or semid.  Or: for a SEM_STAT operation, the index value specified in semid referred to an array slot that is currently unused." << endl;
			break;

		case EPERM:
			cerr << "The argument cmd has the value IPC_SET or IPC_RMID but the effective user ID of the calling process is not the creator (as found in sem_perm.cuid) or the owner (as found in sem_perm.uid) of the semaphore set, and the process does not have the CAP_SYS_ADMIN capability." << endl;
			break;

		case ERANGE:
			cerr << "The argument cmd has the value SETALL or SETVAL and the value to which semval is to be set (for some semaphore of the set) is less than 0 or greater than the implementation limit SEMVMX." << endl;
			break;
	}
}

