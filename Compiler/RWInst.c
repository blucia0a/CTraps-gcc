/******************************************************************************
 * RWInst.c 
 *
 * RWInst - RWInst instrumentation plugin 
 *****************************************************************************/
#include <gcc-plugin.h>
#include <coretypes.h>
#include <diagnostic.h>
#include <gimple.h>
#include <tree.h>
#include <tree-flow.h>
#include <tree-pass.h>
#include <line-map.h>
#include <input.h>
#include <cfgloop.h>

int plugin_is_GPL_compatible = 1;

/* Help info about the plugin if one were to use gcc's --version --help */
static struct plugin_info RWInst_info =
{
    .version = "1",
    .help = "CTraps: A Shared Memory Access Instrumentation Plugin (http://github.com/blucia0a/CTraps-gcc  --  email blucia@gmail.com)",
};


static struct plugin_gcc_version RWInst_ver =
{
    .basever = "4.7",
};


/* We don't need to run any tests before we execute our plugin pass */
static bool RWInst_gate(void)
{
    return true;
}

extern unsigned number_dommed_out;
extern unsigned number_escaped_out;
extern unsigned number_total;
void my_insert_rd_wr(basic_block bb, gimple_stmt_iterator *gsi);
static unsigned RWInst_exec(void)
{

    unsigned i;
    const_tree str, op;
    basic_block bb;
    gimple stmt;
    gimple_stmt_iterator gsi;

    number_dommed_out = 0;
    number_escaped_out = 0;
    number_total = 0;
    init_new_func();

    struct loops loo;
    int numloops = flow_loops_find(&loo);

    calculate_dominance_info(CDI_DOMINATORS);
    calculate_dominance_info(CDI_POST_DOMINATORS);
    
    FOR_EACH_BB(bb)
      for (gsi=gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {

          my_insert_rd_wr(bb, &gsi);

      }
    return 0;

}


/* See tree-pass.h for a list and desctiptions for the fields of this struct */
static struct gimple_opt_pass RWInst_pass = 
{
    .pass.type = GIMPLE_PASS,
    .pass.name = "RWInst",       /* For use in the dump file */
    .pass.gate = RWInst_gate,
    .pass.execute = RWInst_exec, /* Pass handler/callback */
};



/* Return 0 on success or error code on failure */
int plugin_init(struct plugin_name_args   *info,  /* Argument infor */
                struct plugin_gcc_version *ver)   /* Version of GCC */
{
    struct register_pass_info pass;

    if (strncmp(ver->basever, RWInst_ver.basever, strlen("4.7")))
       return -1; /* Incorrect version of gcc */

    pass.pass = &RWInst_pass.pass;
    //pass.reference_pass_name = "alias";
    pass.reference_pass_name = "dom";
    pass.ref_pass_instance_number = 1;
    pass.pos_op = PASS_POS_INSERT_AFTER;

    /* Tell gcc we want to be called after the first SSA pass */
    register_callback("RWInst", PLUGIN_PASS_MANAGER_SETUP, NULL, &pass);
    register_callback("RWInst", PLUGIN_INFO, NULL, &RWInst_info);

    return 0;
}
