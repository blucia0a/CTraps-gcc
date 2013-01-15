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
