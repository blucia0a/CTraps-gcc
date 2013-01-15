#include <stdio.h>
#include <stdlib.h>
#include "ct_plugin.h"

void global_init(){

  fprintf(stderr,"Running Global Init\n");

}

void thread_init(){
  
  fprintf(stderr,"Running Thread Init (T=%lu)\n",(unsigned long)pthread_self());

} 

void global_deinit(){

  fprintf(stderr,"Running Global Init\n");

}

void thread_deinit(){

  fprintf(stderr,"Running Thread Deinit (T=%lu)\n",(unsigned long)pthread_self());

}

void read_trap(void *addr, void *oldPC, unsigned long oldTid, void *newPC, unsigned long newTid){

  fprintf(stderr,"M[%p]: T%lu,W@%p -> T%lu,R@%p\n",addr,oldTid,oldPC,newTid,newPC);

}

void write_trap(void *addr, void *oldPC, unsigned long oldTid, void *newPC, unsigned long newTid){

  fprintf(stderr,"M[%p]: T%lu,W@%p -> T%lu,W@%p\n",addr,oldTid,oldPC,newTid,newPC);

}
