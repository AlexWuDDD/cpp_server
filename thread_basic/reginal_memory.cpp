#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

pthread_key_t thread_log_key;

void write_to_thread_log(const char* message)
{
  if(message == NULL){
    return;
  }

  FILE* logfile = (FILE*)pthread_getspecific(thread_log_key);
  fprintf(logfile, "%s\n", message);
  fflush(logfile);
}

void close_thread_log(void* logfile)
{
  char logfilename[128];
  sprintf(logfilename, "close logfile: thread%ld.log\n", (unsigned long)pthread_self());
  printf("%s", logfilename);
  fclose((FILE*)logfile);
}

void* thread_function(void* args)
{
  char logfilename[128];
  sprintf(logfilename, "thread%ld.log\n", (unsigned long)pthread_self());

  FILE* logfile = fopen(logfilename, "w");
  if(logfile != NULL){
    pthread_setspecific(thread_log_key, logfile);
    write_to_thread_log("Thread starting...");
  }

  return NULL;
}

int main()
{
  pthread_t threadIDs[5];
  //int pthread_key_create(pthread_key_t *key, void (*destructor)(void *));
  //if key is not null, when thread exits, 
  //destructor is called with the value of the key as argument.
  pthread_key_create(&thread_log_key, close_thread_log);
  
  for(int i = 0; i < 5; i++){
    pthread_create(&threadIDs[i], NULL, thread_function, NULL);
  }

  for(int i = 0; i < 5; i++){
    pthread_join(threadIDs[i], NULL);
  }

  return 0;
}