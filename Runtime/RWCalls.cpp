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
#include <pthread.h>
#include <dlfcn.h>
#include <string.h>
#include <atomic>
#include <unistd.h>
#include <signal.h>
#include <set>
#include "LWT.h"
#include "thd_ctr.h"
#include "comm_graph.h"
#include "ct_plugin_manager.h"

#undef SAMPLING
#undef COMMGRAPH
#undef PLUGIN
#undef RRRW
#undef SLCOPTEXC

using namespace std;

pthread_mutex_t tidLock;
unsigned long nexttid;
__thread unsigned long myTid;

pthread_t samplingThread;

unsigned long num_reads;
unsigned long num_gone_reads;

#ifdef RRRW
pthread_mutex_t cciprevlock;
std::set<unsigned long> *cciprev_glob;
__thread std::set<unsigned long> *cciprev;
#endif

extern "C"{
void thdDestructor(void *v){

  #if defined(PLUGIN)
  /*This is where pluggable per-thread shutdown gets called*/
  plugin_thd_deinit();
  #endif

  #if defined(COMMGRAPH)
  dumpCommunicationGraph();
  #endif
  
  #if defined(RRRW)
  pthread_mutex_lock(&cciprevlock);
  for(auto it = cciprev->begin(), et = cciprev->end(); it != et; it++){
    cciprev_glob->insert(*it);
  }
  pthread_mutex_unlock(&cciprevlock);
  #endif

}
}

extern "C"{
void *threadStartFunc(void *arg){

  threadInitData *tid = (threadInitData *)arg;   
  
  tlsKey = (pthread_key_t *)malloc( sizeof( pthread_key_t ) );

  pthread_key_create( tlsKey, thdDestructor );

  pthread_setspecific(*tlsKey,(void*)0x1);

  pthread_mutex_lock(&tidLock);
  myTid = (nexttid << 48) & 0xffff000000000000;
  nexttid++;
  if( nexttid > 0xffff ){
    nexttid = 1;
  }
  pthread_mutex_unlock(&tidLock);

  #if defined(PLUGIN)
  /*This is where pluggable startup gets called*/ 
  plugin_thd_init();
  #endif
  
  #if defined(COMMGRAPH)
  createCommunicationGraph();  
  #endif

  #if defined(RRRW)
  cciprev = new std::set<unsigned long>();
  #endif

  return (tid->start_routine(tid->arg)); 

}
}

void null_init(){

}

void null_trap(void*a,void*b,unsigned long c,void*d,unsigned long e){

}

void loadPlugins(){

  char *p = getenv("CTRAP_PLUGIN");
  if( p ){

    void *p_handle = dlopen(p, RTLD_LAZY | RTLD_GLOBAL);
    if( p_handle != NULL ){

      plugin_init = (void(*)(void))dlsym(p_handle,"global_init");
      plugin_thd_init = (void(*)(void))dlsym(p_handle,"thread_init");
      plugin_deinit = (void(*)(void))dlsym(p_handle,"global_deinit");
      plugin_thd_deinit = (void(*)(void))dlsym(p_handle,"thread_deinit");

      plugin_read_trap = (void(*)(void*,void*,unsigned long,void*,unsigned long))dlsym(p_handle,"read_trap");
      plugin_write_trap = (void(*)(void*,void*,unsigned long,void*,unsigned long))dlsym(p_handle,"write_trap");

    }

  }else{

      plugin_init = &null_init;
      plugin_thd_init = &null_init;
      plugin_deinit = &null_init;
      plugin_thd_deinit = &null_init;

      plugin_read_trap = &null_trap;
      plugin_write_trap = &null_trap;
    

  }
   
}

/*1 Quantum is 100 us*/
#define SAMPLE_QUANTUM 1000
#define NON_SAMPLE_PERIOD 100000

atomic<unsigned long> sampleGeneration;
atomic<bool> samplingOn;
void *sampleTimer(void*v){

  srand( time(NULL) );
  samplingOn = false;
  while(true){

    /*Non-Sampling Period*/
    usleep(NON_SAMPLE_PERIOD);

    /*Sample Generation is ordered by samplingOn's fence*/
    unsigned long t = sampleGeneration.load(memory_order_acquire);
    sampleGeneration.store(t + 1, memory_order_release);
    samplingOn.store(true, memory_order_release);

    /*Non-Sampling Period*/
    while( samplingOn.load(memory_order_acquire) ){
    
      usleep(SAMPLE_QUANTUM); 

    }

  }

}

extern "C" void setup_thd_ctr();
extern "C" int __call_real_pthread_create( pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine)(void*), void *arg);
static void __attribute__ ((constructor)) init();
static void init(){

  #if defined(SLCOPTEXC)
  num_reads = 0;
  num_gone_reads = 0;
  #endif

  #if defined(PLUGIN)
  /*This is where plugins get loaded*/
  loadPlugins();

  /*This is where pluggable global startup gets called*/
  plugin_init();

  /*This is where pluggable per-thread startup gets called.*/
  plugin_thd_init();
  #endif 
  
  #if defined(COMMGRAPH)
  createCommunicationGraph();
  #endif

  #if defined(RRRW)
  cciprev_glob = new std::set<unsigned long>();
  cciprev = new std::set<unsigned long>();
  #endif

  myTid = 0;
  nexttid = 1;
  pthread_mutex_init(&tidLock,NULL);

  #ifdef USE_ATOMICS
  LWT_table = new LWT_Entry[LWT_ENTRIES]; 
  #else
  LWT_table = (LWT_Entry *)calloc(LWT_ENTRIES,sizeof(LWT_Entry));
  #endif


  setup_thd_ctr();
 
  sigset_t olds; 
  sigset_t sigs;
  sigfillset(&sigs);
  pthread_sigmask(SIG_BLOCK,&sigs,&olds);
  #if defined(SAMPLING) 
  __call_real_pthread_create(&samplingThread,NULL,sampleTimer,NULL);
  #endif
  pthread_sigmask(SIG_SETMASK,&olds,NULL);
  
  #if defined(RRRW) 
  pthread_mutex_init(&cciprevlock,NULL);
  #endif

}


static void __attribute__ ((destructor)) deinit();
static void deinit(){

  thdDestructor(NULL);
  
  #if defined(PLUGIN)
  /*This is where pluggable global shutdown gets called*/
  plugin_deinit();
  #endif
  
  #if defined(RRRW)
  pthread_mutex_lock(&cciprevlock);
  fprintf(stderr,"CCI-Prev: Dumping\n");
  for(auto it = cciprev_glob->begin(), et = cciprev_glob->end(); it != et; it++){
    fprintf(stderr,"%p\n",*it);
  }
  pthread_mutex_unlock(&cciprevlock);
  #endif
  
  #if defined(SLCOPTEXC)
  fprintf(stderr,"Dom %lu %lu\n",num_reads,num_gone_reads);
  #endif

}

  #if defined(SLCOPTEXC)
extern "C"{
void MemReadExc(void *addr){
  num_reads++;
  num_gone_reads++;
  return;
}
}
  #endif

__thread unsigned long myGeneration;
__thread unsigned long sampleTicks;
#define LOCAL_SAMPLE_TICKS 100
extern "C"{
void MemRead(void *addr){

  #if defined(SLCOPTEXC)
  num_reads++;
  #endif

    #if defined(SAMPLING)
    #if defined(COMMGRAPH) || defined(PLUGIN) 
    if( !samplingOn.load(memory_order_acquire) ){
    /*Not in a sampling period*/
      return;

    }

    unsigned long gGen = sampleGeneration.load( memory_order_acquire );
    if( myGeneration != gGen ){

      myGeneration = gGen;
      sampleTicks = 0; 

    }

    /*In a sampling period*/
    if( ++sampleTicks > LOCAL_SAMPLE_TICKS ){

      samplingOn.store(false,memory_order_release); 

    }
    #endif
    #endif

    unsigned long index =  ((unsigned long)addr) & ((unsigned long)LWT_SIZE);

    #ifdef USE_ATOMICS
    unsigned long e = LWT_table[index].load(memory_order_consume);
    #else
    LWT_Entry e = LWT_table[index];
    #endif
 
    unsigned long you = (e & ((unsigned long)0xffff000000000000));

    #if defined(RRRW)
    #if defined(STACKS) 
    void *pc0 = __builtin_return_address( 0 );
    void *pc1 = __builtin_return_address( 1 );
    void *pc = (void*)((((unsigned long)pc1) << 24)  | ((unsigned long)pc0));
    #else
    void *pc = __builtin_return_address( 0 );
    #endif
    #endif

 
    if( myTid != you){

      void *oldPC = (void*)(0x0000ffffffffffff & e);
    
      #if defined(RRRW)
      cciprev->insert((unsigned long)oldPC);
      #endif
  
      #if !defined(RRRW) 
      #if defined(STACKS) 
      void *pc0 = __builtin_return_address( 0 );
      void *pc1 = __builtin_return_address( 1 );
      void *pc = (void*)((((unsigned long)pc1) << 24)  | ((unsigned long)pc0));
      #else
      void *pc = __builtin_return_address( 0 );
      #endif
      #endif
 
      #if defined(PLUGIN)
      plugin_read_trap(addr,oldPC,you,pc,myTid);
      #endif

      #if defined(COMMGRAPH)
      addToCommunicationTable( oldPC, pc );
      #endif
 
    
    }
 
  #ifdef RRRW
  unsigned long who = myTid;
  unsigned long where = (0x0000ffffffffffff & ((unsigned long)pc));
  unsigned long newe = ( who | where );

  #ifdef USE_ATOMICS 
  LWT_table[index].store(newe, memory_order_relaxed);
  #else
  LWT_table[ index ] = newe;
  #endif

  #endif

}
}


extern "C"{
void MemWrite(void *addr){
  
  #if defined(SAMPLING)
  if( !samplingOn.load(memory_order_acquire) ){
  /*Not in a sampling period*/
    return;

  }

  unsigned long gGen = sampleGeneration.load( memory_order_acquire );
  if( myGeneration != gGen ){

    myGeneration = gGen;
    sampleTicks = 0; 
  }

  /*In a sampling period*/
  if( ++sampleTicks > LOCAL_SAMPLE_TICKS ){

    samplingOn.store(false,memory_order_release); 

  }
  #endif

  
 
  //unsigned long selfThd = (unsigned long)pthread_self(); 


  unsigned long index = ((unsigned long)addr) & ((unsigned long)LWT_SIZE);

  #ifdef USE_ATOMICS 
  unsigned long e = LWT_table[index].load(memory_order_consume );
  #else
  LWT_Entry e = LWT_table[index];
  #endif 
  
  #if defined(STACKS) 
  void *pc0 = __builtin_return_address( 0 );
  void *pc1 = __builtin_return_address( 1 );
  /*0x8... means "write"*/
  void *pc = (void*)((((unsigned long)pc1) << 24)  | ((unsigned long)pc0));
  #else
  void *pc = __builtin_return_address( 0 );
  #endif
  
  unsigned long who = myTid;
  unsigned long where = (0x0000ffffffffffff & ((unsigned long)pc));
  unsigned long newe = ( who | where );

  #if defined(PLUGIN) || defined(RRRW) || defined(COMMGRAPH) 
  unsigned long you = (e & ((unsigned long)0xffff000000000000));
  if( myTid != you ){
      

    void *oldPC = (void*)(0x0000ffffffffffff & e);
    
    #if defined(RRRW)
    cciprev->insert((unsigned long)oldPC);
    #endif

    #if defined(PLUGIN)
    plugin_write_trap(addr,oldPC,you,pc,myTid);
    #endif

    #if defined(COMMGRAPH)
    addToCommunicationTable( oldPC, pc );
    #endif
    
  }
  #endif
  
  #ifdef USE_ATOMICS 
  LWT_table[index].store(newe, memory_order_relaxed );
  #else
  LWT_table[ index ] = newe;
  #endif

}
}

extern "C"{
void *queryLWT_PC_plugin(void *addr){

  unsigned long index = ((unsigned long)addr) & ((unsigned long)LWT_SIZE);

  unsigned long e = LWT_table[index];
   
  return (void*)(0x0000ffffffffffff & e);

}
}
