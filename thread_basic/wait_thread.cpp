#include <stdio.h>
#include <string.h>
#include <pthread.h>

#define TIME_FILENAME "time.txt"

void *fileThreadFunc(void *arg){
  time_t now = time(NULL);
  struct tm* t = localtime(&now);
  char timeStr[32] = {0};
  snprintf(timeStr, 32, "%04d/%02d/%02d %02d:%02d:%02d",
           t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
           t->tm_hour, t->tm_min, t->tm_sec);
  //if file does not exist, create it; otherwise, overwrite it
  FILE *fp = fopen(TIME_FILENAME, "w");
  if(fp == NULL){
    printf("create file %s failed\n", TIME_FILENAME);
    return NULL;
  }

  size_t sizeToWrite = strlen(timeStr) + 1;
  size_t sizeWritten = fwrite(timeStr, 1, sizeToWrite, fp);
  if(sizeWritten != sizeToWrite){
    printf("write file %s failed\n", TIME_FILENAME);
    return NULL;
  }

  fclose(fp);
  return NULL;
}

int main(){
  pthread_t fileThreadID;
  int ret = pthread_create(&fileThreadID, NULL, fileThreadFunc, NULL);
  if(ret != 0){
    printf("Failed to create fileThread.\n");
    return -1;
  }

  int* retval;
  pthread_join(fileThreadID, (void**)&retval);

  FILE* fp = fopen(TIME_FILENAME, "r");
  if (fp == NULL){
    printf("open file %s failed\n", TIME_FILENAME);
    return -2;
  }

  char buf[32] = {0};
  int sizeRead = fread(buf, 1, 32, fp);
  if(sizeRead == 0){
    printf("read file %s failed\n", TIME_FILENAME);
    fclose(fp);
    return -3;
  }

  printf("Current TIme is:  %s.\n", buf);
  fclose(fp);
  return 0;
}