#include <stdio.h>
#include <stdlib.h>
#include <hash_map>
#include <unistd.h>
#include <set>
#include "ct_plugin.h"

using namespace std;
__thread __gnu_cxx::hash_map<unsigned long, set< unsigned long> > *graph;

int is_gone = 0;

extern "C" void *queryLWT_PC_plugin(void *addr);

void load_graph(){

  graph = new __gnu_cxx::hash_map<unsigned long, set< unsigned long > >();

  char *p = getenv("CGTEST_GRAPH");
  if( p != NULL ){

    unsigned long src;
    unsigned long sink;
    FILE *gf = fopen(p,"r");
    while( fscanf(gf,"%lx %lx\n",&src,&sink) != EOF ){

      auto ct_iter = graph->find( (unsigned long)src);
      if( ct_iter == graph->end()  ){ 
 
         
        ((*graph)[ (unsigned long)src ]) = set<unsigned long>();   
        (*graph)[ (unsigned long)src ].insert( (unsigned long) sink);
  
      }else{
  
        if( (ct_iter->second ).find( (unsigned long)sink ) == (ct_iter->second).end() ){
  
          (ct_iter->second ).insert( (unsigned long)sink );
   
        }
  
      }
  
    }

  }else{
    fprintf(stderr,"Couldn't load the graph from %s\n",p);
    abort();
  }
  
}

extern "C"{
void global_init(){

  fprintf(stderr,"CTRAPS: CGAnom Plugin Startup\n");
  is_gone = 0;

}
}

extern "C" {
void thread_init(){
  
  load_graph(); 

} 
}

extern "C" {
void global_deinit(){

  fprintf(stderr,"CTRAPS: CGAnom Plugin Shutdown\n");

}
}

extern "C" {
void thread_deinit(){

  fprintf(stderr,"CTRAPS: CGAnom Plugin Thread Deinit (T=%lu)\n",(unsigned long)pthread_self());
  delete graph;

}
}

#define MAX_DELAY 100000
#define DELAY_STEP 1

extern "C" { 
void read_trap(void *addr, void *oldPC, unsigned long oldTid, void *newPC, unsigned long newTid){

  auto srcIter = graph->find( (unsigned long)oldPC );
  if( srcIter == graph->end() ){

    fprintf(stderr,"{Communication Anomaly - New SRC} M[%p]: T%lu,W@%p -> T%lu,R@%p\n",addr,oldTid,oldPC,newTid,newPC);

    //((*graph)[ (unsigned long)oldPC])();
    ((*graph)[ (unsigned long)oldPC]) = set<unsigned long>();   
    (*graph)[ (unsigned long)oldPC ].insert( (unsigned long)newPC );
    return; 

  } else {

    auto sinkIter = srcIter->second.find( (unsigned long)newPC );
    if( sinkIter == srcIter->second.end() ){

      fprintf(stderr,"{Communication Anomaly} M[%p]: T%lu,W@%p -> T%lu,R@%p\n",addr,oldTid,oldPC,newTid,newPC);
      srcIter->second.insert( (unsigned long)newPC );
      return;

    } else {
 
      return;
    
    }
 
  }

}
}

extern "C" {
void write_trap(void *addr, void *oldPC, unsigned long oldTid, void *newPC, unsigned long newTid){

  auto srcIter = graph->find( (unsigned long)oldPC );
  if( srcIter == graph->end() ){

    fprintf(stderr,"{Communication Anomaly - NEW SRC} M[%p]: T%lu,W@%p -> T%lu,W@%p\n",addr,oldTid,oldPC,newTid,newPC);
    ((*graph)[ (unsigned long)oldPC]) = set<unsigned long>();   
    (*graph)[ (unsigned long)oldPC ].insert( (unsigned long)newPC );
    return; 

  } else {

    auto sinkIter = srcIter->second.find( (unsigned long)newPC );
    if( sinkIter == srcIter->second.end() ){

      fprintf(stderr,"{Communication Anomaly} M[%p]: T%lu,W@%p -> T%lu,W@%p\n",addr,oldTid,oldPC,newTid,newPC);
      srcIter->second.insert( (unsigned long)newPC );
      return;

    } else {
 
      return;
    
    }
 
  }

}
}
