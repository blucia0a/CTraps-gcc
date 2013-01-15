/*
==CTraps -- A GCC Plugin to instrument shared memory accesses==
 
    Copyright (C) 2012 Brandon Lucia

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

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
