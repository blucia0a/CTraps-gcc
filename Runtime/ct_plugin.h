#ifdef __cplusplus
extern "C"{
#endif
void global_init();
void thread_init();
void global_deinit();
void thread_deinit();
void read_trap(void *addr, void *oldPC, unsigned long oldTid, void *newPC, unsigned long newTid);
void write_trap(void *addr, void *oldPC, unsigned long oldTid, void *newPC, unsigned long newTid);
#ifdef __cplusplus
}
#endif
