/*
==CTraps -- A GCC Plugin to instrument shared memory accesses==

    LWT.h 
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

#ifndef _LWT_H_
#define _LWT_H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <pthread.h>
#include <atomic>
#include <set>

#undef USE_ATOMICS
#define LWT_SIZE 0xffffff
#define LWT_ENTRIES 0x1000000


#ifdef USE_ATOMICS
typedef std::atomic<unsigned long> LWT_Entry;
#else
typedef unsigned long LWT_Entry;
#endif

#define EVAL_TRACKMEMSIZE
#ifdef EVAL_TRACKMEMSIZE
std::set<unsigned long> MSet;
pthread_mutex_t mset_lock;
#endif

extern "C"{
static LWT_Entry *LWT_table;
}

pthread_mutex_t last_writer_lock;

void init_LWT();

void insert_into_LWT( pthread_t thd, void *pc, void *addr );

inline unsigned long find_LWT_bin_for_addr( void *addr );

LWT_Entry get_LWT_entry_for_addr( void *addr );

LWT_Entry create_LWT_entry( pthread_t thd, void *pc );

bool LWT_entry_thd_equals( pthread_t thd, LWT_Entry e );

bool LWT_entry_pc_equals( void *pc, LWT_Entry e);

bool LWT_entry_equals( pthread_t thd, void *pc, LWT_Entry e);

void *LWT_entry_get_pc( LWT_Entry e);

#endif
