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
#include <hash_map>
#include <unistd.h>
#include "ct_plugin.h"

using namespace std;
__thread __gnu_cxx::hash_map<unsigned long, __gnu_cxx::hash_map< unsigned long, unsigned long> > *graph;
__thread __gnu_cxx::hash_map<unsigned long,  unsigned long > *graph_max;

int is_gone = 0;

extern "C" void *queryLWT_PC_plugin(void *addr);

void load_graph(){

  graph = new __gnu_cxx::hash_map<unsigned long, __gnu_cxx::hash_map< unsigned long, unsigned long> >();
  graph_max = new __gnu_cxx::hash_map<unsigned long, unsigned long >();

  char *p = getenv("CGTEST_GRAPH");
  if( p != NULL ){

    unsigned long src;
    unsigned long sink;
    unsigned long num;
    FILE *gf = fopen(p,"r");
    while( fscanf(gf,"%lx-%lx:%lu\n",&src,&sink,&num) != EOF ){

      auto ct_iter = graph->find( (unsigned long)src);
      if( ct_iter == graph->end()  ){ 
  
        (*graph)[ (unsigned long)src ] = __gnu_cxx::hash_map<unsigned long, unsigned long>();
        (*graph)[ (unsigned long)src ][ (unsigned long)sink ] = num;
  
  
      }else{
  
        if( (ct_iter->second ).find( (unsigned long)sink ) == (ct_iter->second).end() ){
  
          (ct_iter->second )[ (unsigned long)sink ] = num;
   
        }else{
  
          (ct_iter->second )[ (unsigned long)sink ] += num;
  
        }

      }
      (*graph_max)[ src ] += num;
  
    }

  }else{
    fprintf(stderr,"Couldn't load the graph from %s\n",p);
    abort();
  }
  
}

extern "C"{
void global_init(){

  fprintf(stderr,"CTRAPS: CGTest Plugin Startup\n");
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

  fprintf(stderr,"CTRAPS: CGTest Plugin Shutdown\n");

}
}

extern "C" {
void thread_deinit(){

  fprintf(stderr,"CTRAPS: CGTest Plugin Thread Deinit (T=%lu)\n",(unsigned long)pthread_self());

}
}

#define MAX_DELAY 100000
#define DELAY_STEP 1

extern "C" { 
void read_trap(void *addr, void *oldPC, unsigned long oldTid, void *newPC, unsigned long newTid){

  unsigned long delay = 0;
  unsigned long amt = 0;
  unsigned long max = 0;
  auto srcIter = graph->find( (unsigned long)oldPC );
  if( srcIter == graph->end() ){

    /*For testing, we want to delay the least likely operations the least.
     * We've never seen this, so let it go immediately*/
    return; 

  } else {

    auto sinkIter = srcIter->second.find( (unsigned long)newPC );
    if( sinkIter == srcIter->second.end() ){

      return;

    } else {

      amt = (*graph)[(unsigned long)oldPC][(unsigned long)newPC];
      max = (*graph_max)[(unsigned long)oldPC];
      float pct = ((float)amt)/((float)max); 
      delay = (unsigned long)(((float)pct) * ((float)MAX_DELAY));
      //fprintf(stderr,"%f * %lu = %lu\n",pct,MAX_DELAY,delay);
    
    }
 
  }
  
  unsigned long e = 0;
  void *origPC = oldPC;
  //fprintf(stderr,"T%lu: Delaying@%p because %p->%p is %lu/%lu frequent (%f)\n",(unsigned long)newTid,newPC,oldPC,newPC,amt,max,delay);
  while( true ){


    if( e >= delay ){ /*Max delay time for this edge*/
      break;
    }

    void *newc = NULL; 
    if( (newc = queryLWT_PC_plugin(addr)) != origPC ){ /*communication changed*/
      fprintf(stderr,"PCBREAK! %p became %p\n",origPC,newc);
      break;
    }else{
      fprintf(stderr,"newc == %p and orig == %p\n",newc,origPC);
    }
    usleep(DELAY_STEP);
    e += DELAY_STEP;

  }


}
}

extern "C" {
void write_trap(void *addr, void *oldPC, unsigned long oldTid, void *newPC, unsigned long newTid){


}
}
