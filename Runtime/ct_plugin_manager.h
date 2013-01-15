
void (*plugin_thd_init)(void);
void (*plugin_thd_deinit)(void);
void (*plugin_init)(void);
void (*plugin_deinit)(void);

void (*plugin_read_trap)(void *, void *, unsigned long, void *,unsigned long);
void (*plugin_write_trap)(void *, void *, unsigned long, void *,unsigned long);
