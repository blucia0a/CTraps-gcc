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



#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <iostream>
#include <fstream>
#include <hash_map>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include "comm_graph.h"

using namespace std;

pthread_mutex_t communication_table_lock;

__thread __gnu_cxx::hash_map<unsigned long, __gnu_cxx::hash_map< unsigned long, unsigned long> > *communication_table;

void dumpCommunicationGraph(){
  
  char *p = getenv("CG_GRAPH");

  if(p){

    char *s = (char *)malloc(sizeof(char) * strlen(p) + strlen("executable") + 18);
    sprintf(s,"%s%s%lx",p,"executable",(unsigned long)pthread_self());
    ofstream fs(s); 
    //__gnu_cxx::hash_map<unsigned long, __gnu_cxx::hash_map< unsigned long, unsigned long> >::iterator ct_iter, ct_end;
    for( auto ct_iter = communication_table->begin(), ct_end = communication_table->end(); ct_iter != ct_end; ct_iter++ ){

      //__gnu_cxx::hash_map< unsigned long, unsigned long>::iterator sink_iter, sink_end;
      for( auto sink_iter = ct_iter->second.begin(), sink_end = ct_iter->second.end(); sink_iter != sink_end; sink_iter++ ){

        //string w = ct_iter->first & 0x8000000000000000 ? string("w") : string("r");
        #if defined(STACKS)
        unsigned long src0 = (ct_iter->first & 0x0000FFFFFF000000) >> 24;
        unsigned long src1 = (ct_iter->first & 0xFFFFFF);
        unsigned long sink0 = (sink_iter->first & 0x0000FFFFFF000000) >> 24;
        unsigned long sink1 = (sink_iter->first & 0xFFFFFF);
        fs <<  hex << src0 << ":" << src1 << " " << sink0 << ":" << sink1 << " " << dec << sink_iter->second << endl;
        #else
        unsigned long src = (ct_iter->first & 0xFFFFFF);
        unsigned long sink = (sink_iter->first & 0xFFFFFF);
        fs <<  hex << src << " " << sink << " " << dec << sink_iter->second << endl;
        #endif

      }

    }

  }else{

    //__gnu_cxx::hash_map<unsigned long, __gnu_cxx::hash_map< unsigned long, unsigned long> >::iterator ct_iter, ct_end;
    for( auto ct_iter = communication_table->begin(), ct_end = communication_table->end(); ct_iter != ct_end; ct_iter++ ){

      //__gnu_cxx::hash_map< unsigned long, unsigned long>::iterator sink_iter, sink_end;
      for( auto sink_iter = ct_iter->second.begin(), sink_end = ct_iter->second.end(); sink_iter != sink_end; sink_iter++ ){

        cerr << hex << ct_iter->first << "-" << sink_iter->first << ":" << dec << sink_iter->second << endl;

      }

    }

  }

}

void createCommunicationGraph(){
  
  communication_table = new __gnu_cxx::hash_map<unsigned long, __gnu_cxx::hash_map< unsigned long, unsigned long> >();

}

void addToCommunicationTable( void *src, void *sink ){

    
      auto ct_iter = communication_table->find( (unsigned long)src);
      if( ct_iter == communication_table->end()  ){ 
  
        (*communication_table)[ (unsigned long)src ] = __gnu_cxx::hash_map<unsigned long, unsigned long>();
        (*communication_table)[ (unsigned long)src ][ (unsigned long)sink ] = 1;
  
  
      }else{
  
        if( (ct_iter->second ).find( (unsigned long)sink ) == (ct_iter->second).end() ){
  
          (ct_iter->second )[ (unsigned long)sink ] = 1;
   
        }else{
  
          (ct_iter->second )[ (unsigned long)sink ]++;
  
        }
  
      }

}
