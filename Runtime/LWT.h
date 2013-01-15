#ifndef _LWT_H_
#define _LWT_H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <pthread.h>
#include <atomic>

#undef USE_ATOMICS
#define LWT_SIZE 0xfffffff
#define LWT_ENTRIES 0x10000000

#ifdef USE_ATOMICS
typedef std::atomic<unsigned long> LWT_Entry;
#else
typedef unsigned long LWT_Entry;
#endif

static LWT_Entry *LWT_table;

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
