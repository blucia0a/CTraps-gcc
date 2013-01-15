#include <stdio.h>
#include <stdlib.h>
#include "ct_plugin.h"

int is_gone = 0;
void global_init(){

  fprintf(stderr,"CTRAPS: SFI Plugin Startup\n");
  is_gone = 0;

}

void thread_init(){
  
  fprintf(stderr,"CTRAPS: SFI Plugin Thread Init (T=%lu)\n",(unsigned long)pthread_self());

} 

void global_deinit(){

  fprintf(stderr,"CTRAPS: SFI Plugin Shutdown\n");

}

void thread_deinit(){

  fprintf(stderr,"CTRAPS: SFI Plugin Thread Deinit (T=%lu)\n",(unsigned long)pthread_self());

}

void read_trap(void *addr, void *oldPC, unsigned long oldTid, void *newPC, unsigned long newTid){

  if( newPC == (void*)0x400c53 && 
      *( (unsigned long *)addr) == 0 ){

    fprintf(stderr,"M[%p]: T%lu,W@%p -> T%lu,R@%p\n",addr,oldTid,oldPC,newTid,newPC);
    fprintf(stderr,"NULL POINTER!!!\n",addr,oldTid,oldPC,newTid,newPC);
    abort();

  }

}

void write_trap(void *addr, void *oldPC, unsigned long oldTid, void *newPC, unsigned long newTid){


}
