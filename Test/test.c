#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

int *x;

void *worker(void *v){

  int i;
  for(i = 0; i < 10000; i++){
    (*x)++;
  }

}

int main(int argc, char *argv){
   
  x = malloc(sizeof(int)); 
  pthread_t t1,t2;
  pthread_create(&t1,NULL,worker,NULL);
  pthread_create(&t2,NULL,worker,NULL);

  pthread_join(t1,NULL);
  pthread_join(t2,NULL);
  fprintf(stderr,"FINAL VALUE: (%x)=%d\n",x,*x); 

}
