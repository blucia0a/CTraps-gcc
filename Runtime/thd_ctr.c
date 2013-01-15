#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <dlfcn.h>

typedef struct _threadInitData{

  void *(*start_routine)(void*);
  void *arg;

} threadInitData;

/*Thread Constructor Stuff*/
void (*thd_ctr)(void*,void*(*)(void*));

extern void *threadStartFunc(void *arg);

static int (* __real_pthread_create)(pthread_t *, const pthread_attr_t *, void *(*)(void*), void *);
int pthread_create( pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg){


  threadInitData *tid = (threadInitData*)malloc(sizeof(*tid));

  tid->start_routine = start_routine;

  tid->arg = arg;

  int ret = __real_pthread_create(thread,attr,threadStartFunc,(void*)tid);

  return ret;

}

int __call_real_pthread_create( pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg){

  __real_pthread_create(thread,attr,start_routine,arg);

}

void setup_thd_ctr(){

  dlerror();

  __real_pthread_create = (int (*)(pthread_t *, const pthread_attr_t *, void *(*)(void*), void *)) dlsym(RTLD_NEXT, "pthread_create");

  if( __real_pthread_create == NULL ){

    fprintf(stderr,"Couldn't load pthread_create %s\n",dlerror());
    exit(-1);

  }

}
