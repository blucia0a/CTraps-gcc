__thread pthread_key_t *tlsKey;

typedef struct _threadInitData{

  void *(*start_routine)(void*);
  void *arg;

} threadInitData;

