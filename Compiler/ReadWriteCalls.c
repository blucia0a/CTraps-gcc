/*
==CTraps -- A GCC Plugin to instrument shared memory accesses==

    ReadWriteCalls.c 
 
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

#include <gcc-plugin.h>
#include <coretypes.h>
#include <diagnostic.h>
#include <gimple.h>
#include <tree.h>
#include <tree-flow.h>
#include <tree-pass.h>
#include <input.h>
#include <assert.h>
#include <cgraph.h>
#include <cfgloop.h>

#undef SLCOPTEXC

tree Wr_type;
tree Wr_decl;
gimple Wr_call;

tree Rd_type;
tree Rd_decl;
gimple Rd_call;

#if defined(SLCOPTEXC)
tree Rd_Exc_type;
tree Rd_Exc_decl;
gimple Rd_Exc_call;
#endif

unsigned number_dommed_out;
unsigned number_escaped_out;
unsigned number_total;

typedef struct _ipoint{

  bool valid;
  basic_block b;
  gimple s;
  tree e;

} ipoint;

/*MAX 10000 per function --hack!*/
ipoint ipoints[10000];

bool inited = false;

void init_new_func(void){

  tree wr_param_type_list = tree_cons(NULL_TREE, ptr_type_node, NULL_TREE);
  Wr_type = build_function_type(void_type_node, wr_param_type_list); 
  Wr_decl = build_fn_decl ("MemWrite", Wr_type);
  
  tree rd_param_type_list = tree_cons(NULL_TREE, ptr_type_node, NULL_TREE);
  Rd_type = build_function_type(void_type_node, rd_param_type_list); 
  Rd_decl = build_fn_decl ("MemRead", Rd_type);
  
  #if defined(SLCOPTEXC)
  tree rd_exc_param_type_list = tree_cons(NULL_TREE, ptr_type_node, NULL_TREE);
  Rd_Exc_type = build_function_type(void_type_node, rd_exc_param_type_list); 
  Rd_Exc_decl = build_fn_decl ("MemReadExc", Rd_Exc_type);
  #endif

  int i;
  for( i = 0; i < 10000; i++ ){

    ipoints[i].b = NULL;
    ipoints[i].s = NULL;
    ipoints[i].e = NULL;
    ipoints[i].valid = false;


  }

}

bool points_to_escaped( tree ssaname ){

  struct ptr_info_def *pi = SSA_NAME_PTR_INFO( ssaname );

  if( pi ){

    struct pt_solution *pt = &pi->pt;    
    
    if( pt->anything || pt->nonlocal || pt->escaped || (pt->vars && pt->vars_contains_global) ){
      return true;
    }  

  }else{

    return true;

  }
  return false;
}

bool can_escape (tree var){

  bool escapes = false; 
  if (TREE_CODE (var) == SSA_NAME) {

    if (POINTER_TYPE_P (TREE_TYPE (var))){

      escapes = points_to_escaped( var );

    }

    var = SSA_NAME_VAR (var);

  }

  bool glob = false;
  if (var != NULL_TREE){

    glob = is_global_var(var);

  }


  if( !escapes && !glob ){
    number_total++;
    number_escaped_out++;
  }
  return escapes || glob;

}

static inline void
recompute_all_dominators (void)
{
  free_dominance_info (CDI_DOMINATORS);
  free_dominance_info (CDI_POST_DOMINATORS);
  calculate_dominance_info (CDI_DOMINATORS);
  calculate_dominance_info (CDI_POST_DOMINATORS);
}

bool stmt_postdominates_stmt_p (gimple s2, gimple s1)
{

  basic_block bb1 = gimple_bb (s1), bb2 = gimple_bb (s2);

  if (!bb1 || s1 == s2){
    //fprintf(stderr,"pd case 1\n");
    return true;
  }

  if (bb1 == bb2){

      gimple_stmt_iterator bsi;

      if (gimple_code (s2) == GIMPLE_PHI){

        //fprintf(stderr,"pd case 2\n");
        return true;

      }

      if (gimple_code (s1) == GIMPLE_PHI){

        //fprintf(stderr,"pd case 3\n");
        return false;

      }

      for (bsi = gsi_start_bb (bb1); gsi_stmt (bsi) != s2; gsi_next (&bsi)){

        if (gsi_stmt (bsi) == s1){

          //fprintf(stderr,"pd case 4\n");
          return true;

        }

      }

      //fprintf(stderr,"pd case 5\n");
      return false;
  }

  //fprintf(stderr,"pd case 5\n");
  return dominated_by_p (CDI_POST_DOMINATORS, bb1, bb2);

}

bool in_DOM_with_other( basic_block b, gimple stmt, tree expr ){
  int i;
  for( i = 0; i < 10000; i++ ){

    if( ipoints[i].valid ){

      if( ipoints[i].e == expr ){

        if( stmt_dominates_stmt_p(ipoints[i].s,stmt) ){
          return true;
        }

        if( stmt_dominates_stmt_p(stmt,ipoints[i].s) ){
          return true;
        }

      }

    }

  }

  return false;

}

bool in_SLC_with_other( basic_block b, gimple stmt, tree expr ){

  int i;
  for( i = 0; i < 10000; i++ ){

    if( ipoints[i].valid ){

      if( ipoints[i].e == expr ){

        if( stmt_dominates_stmt_p(ipoints[i].s,stmt) &&
            stmt_postdominates_stmt_p(stmt,ipoints[i].s) ){
          return true;
        }

        if( stmt_postdominates_stmt_p(ipoints[i].s,stmt) &&
            stmt_dominates_stmt_p(stmt,ipoints[i].s) ){
          return true;
        }

      }

    }

  }

  return false;
}


void update_ipoints_list( basic_block bb, gimple stmt, tree expr ){

    int i;
    for( i = 0; i < 10000; i++ ){

      if( !ipoints[i].valid ){

        ipoints[i].b = bb;
        ipoints[i].e = expr;
        ipoints[i].s = stmt; 
        ipoints[i].valid = true;
        break;
 
      }

    }

}

bool try_insert_rd_off_loop( tree expr, basic_block bb, gimple_stmt_iterator *gsi, bool *hoisted );

void insert_rd( tree expr, basic_block bb, gimple_stmt_iterator *gsi, bool *hoisted ){

    number_total++;



#ifndef WRITEONLY

#if defined(DOMOPT) || defined(SLCOPT)
#ifdef DOMOPT
    if( in_DOM_with_other( bb, gsi_stmt(*gsi), expr ) ){
#endif

#if defined(SLCOPT) 
    if( in_SLC_with_other( bb, gsi_stmt(*gsi), expr ) ){
#endif
      

      #if defined(SLCOPTEXC)

      gimple Rd_Exc_call = gimple_build_call(Rd_Exc_decl, 1, expr );
    
      gsi_insert_before(gsi, Rd_Exc_call, GSI_SAME_STMT);
      
      recompute_all_dominators();
  
      struct cgraph_node *current_fun_decl_node = cgraph_get_create_node(current_function_decl);
  
      struct cgraph_node *Rd_decl_node = cgraph_get_create_node(Rd_Exc_decl);
  
      struct cgraph_edge *e; 
      if( !(e = cgraph_edge(current_fun_decl_node,Rd_call)) ){
    
        cgraph_create_edge(current_fun_decl_node, 
                           Rd_decl_node, 
                           Rd_call, 
                           compute_call_stmt_bb_frequency(current_function_decl, bb), 
                           bb->loop_depth);
      }

      #endif

      number_dommed_out++;
      return;


    }
#endif

    update_ipoints_list( bb, gsi_stmt(*gsi), expr );

    gimple Rd_call = gimple_build_call(Rd_decl, 1, expr );
    
    gsi_insert_before(gsi, Rd_call, GSI_SAME_STMT);
      
    recompute_all_dominators();
  
    struct cgraph_node *current_fun_decl_node = cgraph_get_create_node(current_function_decl);
  
    struct cgraph_node *Rd_decl_node = cgraph_get_create_node(Rd_decl);
  
    struct cgraph_edge *e; 
    if( !(e = cgraph_edge(current_fun_decl_node,Rd_call)) ){
    
      cgraph_create_edge(current_fun_decl_node, 
                         Rd_decl_node, 
                         Rd_call, 
                         compute_call_stmt_bb_frequency(current_function_decl, bb), 
                         bb->loop_depth);
    }

#endif

}

/*TODO: FIx this mess of trash*/
bool try_insert_rd_off_loop( tree expr, basic_block bb, gimple_stmt_iterator *gsi, bool *hoisted ){

      *hoisted = true;
      struct loop *lo = bb->loop_father;

      print_node(stderr,"\n\nTrying to make it work with a loop doodly\n\n",expr,0);
      gimple stmt = gsi_stmt(*gsi);
      fprintf(stderr,"The statement is: \n");
      print_gimple_stmt(stderr,stmt,0,0);

      fprintf(stderr,"Getting loop...\n");
      if( lo != NULL ){

        fprintf(stderr,"Got it.  Getting preheader edge\n");
        edge entry_e = loop_preheader_edge (lo);

        fprintf(stderr,"Got it.  Null testing preheader edge\n");
        if( entry_e ){

          fprintf(stderr,"Got it.  Not Null.\n");

          basic_block hdr = entry_e->src;

          fprintf(stderr,"Got header block.\n");
          if( hdr != NULL ){

            fprintf(stderr,"header block was not null.  Getting def stmt\n");
            //gimple defstmt = SSA_NAME_DEF_STMT(expr);
            
            //fprintf(stderr,"null testing def stmt\n");
            //if( defstmt != NULL ){
  
              fprintf(stderr,"def stmt was not null. getting start bb gsi\n");
              gimple_stmt_iterator hdrsi = gsi_start_bb(hdr);
              
              fprintf(stderr,"end-checking start bb gsi\n");
              if( !gsi_end_p(hdrsi) ){
              
                fprintf(stderr,"getting loop preheader start stmt.\n");
                gimple hdrstmt = gsi_stmt(hdrsi);

                /*This last condition checks to be sure the symbol we refer to
 *                in the call is defined by the time we make the call, i.e., after the loop header*/
                //fprintf(stderr,"checking domination.\n");

                //print_gimple_stmt(stderr,hdrstmt,0,0);
               // print_gimple_stmt(stderr,defstmt,0,0);

             //   if( stmt_dominates_stmt_p( hdrstmt, defstmt ) ){
  
                  fprintf(stderr,"Insertion.\n");
                  insert_rd( expr, hdr, &hdrsi, hoisted );
             
                  return true;

              //  }

              } 
  
            //}
  
          }

        }

      }
      return false;

}

void insert_wr( tree expr, basic_block bb, gimple_stmt_iterator *gsi ){

  gimple Wr_call = gimple_build_call(Wr_decl, 1, expr );
 
  gsi_insert_before(gsi, Wr_call, GSI_SAME_STMT);
 
  recompute_all_dominators();

  struct cgraph_node *current_fun_decl_node = cgraph_get_create_node(current_function_decl);

  struct cgraph_node *Wr_decl_node = cgraph_get_create_node(Wr_decl);

  struct cgraph_edge *e; 
  if( !(e = cgraph_edge(current_fun_decl_node,Wr_call)) ){
  
  cgraph_create_edge(current_fun_decl_node, 
                     Wr_decl_node, 
                     Wr_call, 
                     compute_call_stmt_bb_frequency(current_function_decl, bb), 
                     bb->loop_depth);
  }

}

int compute_component_ref_offset(tree cref){
  
  /*Compute the offset from properties of the field declaration*/
  tree fielddecl = TREE_OPERAND(cref,1); 
  tree bitoffset = DECL_FIELD_BIT_OFFSET(fielddecl);
  tree fieldoffset = DECL_FIELD_OFFSET(fielddecl);
  unsigned long byteoffset = ((TREE_INT_CST_HIGH (fieldoffset) << HOST_BITS_PER_WIDE_INT) + TREE_INT_CST_LOW (fieldoffset));
  unsigned long ibitoffset = ((TREE_INT_CST_HIGH (bitoffset) << HOST_BITS_PER_WIDE_INT) + TREE_INT_CST_LOW (bitoffset));

  /*Build a new offset constant node to pass to our instrumentation*/
  return (byteoffset + (ibitoffset/8));

}

tree component_ref_offset_tree(int offset){
  
  return build_int_cst( integer_type_node, offset);
  
}


tree get_base(tree refbase){

  if( TREE_CODE(refbase) == MEM_REF ){

    /*BASE CASE #1*/
    /*If it is a mem_ref, the first argument is the address of the base of the component ref*/
    return TREE_OPERAND(refbase,0);

  }else if( DECL_P(refbase) ){

    /*BASE CASE #2*/
    /*If it is a decl, then it's a global or static or something, so we have to take the address*/
    return build_addr (refbase, current_function_decl);

  }else{
    return NULL;
  }

}


/*Recursively descend into component_ref and array_refs that base off of one another.
 *
 * The code accumulates an offset expression (as a tree) and when it hits the bottom
 * it fills in the base tree*/
void get_offset_and_base(tree expr, tree *offset, tree *base){

  tree ref_base = TREE_OPERAND(expr,0);


  if( TREE_CODE( ref_base ) == ARRAY_REF ||
      TREE_CODE( ref_base ) == COMPONENT_REF ){

      get_offset_and_base(ref_base, offset, base);

  }
  else if( TREE_CODE(ref_base) == MEM_REF ){
    /*Base Case!  We have a base value here!*/
    *base = get_base(ref_base);
    *offset = TREE_OPERAND(ref_base, 1);
  }else{
    
    /*Base Case!  We have a base value here!*/
    *base = get_base(ref_base);

  }

  /*On the way back up the return stack, update the offset expr*/
  /*Only ARRAY_REFs and COMPONENT_REFs get here*/
  if( TREE_CODE( expr ) == ARRAY_REF ){

    tree elem_size = array_ref_element_size(expr);
   
    tree arr_idx = TREE_OPERAND(expr,1);

    tree aoffset = fold_build2 (MULT_EXPR, TREE_TYPE (arr_idx), arr_idx, elem_size); 

    if( *offset == NULL ){

      *offset = aoffset;

    }else{

      *offset = fold_build2(PLUS_EXPR, integer_type_node, *offset, aoffset );
 
    }


  }else if( TREE_CODE(expr) == COMPONENT_REF ){


    int coffset = compute_component_ref_offset(expr); 

    if( *offset == NULL ){

      tree constoff =  build_int_cst(integer_type_node, coffset);

      *offset = constoff;


    }else{
  
      *offset = fold_build2(PLUS_EXPR, integer_type_node, *offset, build_int_cst( integer_type_node, coffset) );

    }
    
  }else{

    print_node(stderr,"\n\n--------------------OFFSET TYPE ERROR ---------------\n\n",expr,0);

  }

  return;

}

/*Handle a read in a COMPONENT_REF tree*/
void insert_rd_comp_ref(tree expr, gimple stmt, basic_block bb, gimple_stmt_iterator *gsi){

  tree offset = NULL;
  tree base = NULL;

  get_offset_and_base(expr, &offset, &base);

  tree final_addr = fold_build_pointer_plus(base,offset);

  if( can_escape( base ) ){

    bool hoisted = false;
    insert_rd(final_addr, bb,gsi, &hoisted);

  } 
 
}

/*Handle a read in an ARRAY_REF tree*/
void insert_rd_arr_ref(tree expr, gimple stmt, basic_block bb, gimple_stmt_iterator *gsi){
  
  tree offset = NULL;
  tree base = NULL;

  tree ref_base = TREE_OPERAND(expr,0);
  if( TREE_CODE(ref_base) == STRING_CST ){ return; }

  get_offset_and_base(expr, &offset, &base);

  tree final_addr = fold_build_pointer_plus(base,offset);

  if( can_escape( base) ){

    bool hoisted = false;
    insert_rd(final_addr, bb,gsi, &hoisted);

  } 

}

/*Handle a read in an MEM_REF tree*/
void insert_rd_mem_ref(tree expr, gimple stmt, basic_block bb, gimple_stmt_iterator *gsi){

  tree base = TREE_OPERAND(expr, 0);
  tree offset = TREE_OPERAND(expr, 1);

  tree final_addr = fold_build_pointer_plus(base, offset);

  if( can_escape( base ) ){

    bool hoisted = false;
    insert_rd(final_addr,bb,gsi,&hoisted);

  } 
 
}

/*Handle a write in an COMPONENT_REF tree*/
void insert_wr_comp_ref(tree expr, gimple stmt, basic_block bb, gimple_stmt_iterator *gsi){

  tree offset = NULL;
  tree base = NULL; 

  get_offset_and_base(expr, &offset, &base);

  tree final_addr = fold_build_pointer_plus(base,offset);

  if( can_escape( base) ){

    insert_wr(final_addr, bb,gsi);

  } 
 
}

/*Handle a write in an ARRAY_REF tree*/
void insert_wr_arr_ref(tree expr, gimple stmt, basic_block bb, gimple_stmt_iterator *gsi){
  
  tree offset = NULL;
  tree base = NULL;

  tree ref_base = TREE_OPERAND(expr,0);
  if( TREE_CODE(ref_base) == STRING_CST ){ return; }

  get_offset_and_base(expr, &offset, &base);

  tree final_addr = fold_build_pointer_plus(base,offset);

  if( can_escape( base) ){

    insert_wr(final_addr, bb, gsi);

  } 

}

/*Handle a write in an MEM_REF tree*/
void insert_wr_mem_ref(tree expr, gimple stmt, basic_block bb, gimple_stmt_iterator *gsi){
  
  tree base = TREE_OPERAND(expr, 0);
  tree offset = TREE_OPERAND(expr, 1);

  tree final_addr = fold_build_pointer_plus(base, offset);

  if( can_escape( base ) ){

    insert_wr(final_addr,bb,gsi);

  } 

}

void handle_read_ref(tree ref, gimple stmt, basic_block bb, gimple_stmt_iterator *gsi){
  

    if( TREE_CODE(ref) == MEM_REF ){

      insert_rd_mem_ref(ref,stmt,bb,gsi);

    }else if( TREE_CODE(ref) == COMPONENT_REF ){
        
      insert_rd_comp_ref(ref,stmt,bb,gsi);

    }else if( TREE_CODE(ref) == ARRAY_REF ){
        
      insert_rd_arr_ref(ref,stmt,bb,gsi);

    }else if( DECL_P(ref) ){
    
      if( can_escape(ref)  ){

        bool hoisted = false;
        insert_rd( build_addr(ref,current_function_decl),bb,gsi,&hoisted );

      }

    }else if( TREE_CODE(ref) == BIT_FIELD_REF ){
      /*Would like to handle these, but the IR is new and undocumented, and the args aren't what they seemm...*/
      /*
      tree memref = TREE_OPERAND(ref,0);
      insert_rd_mem_ref(memref,stmt,bb,gsi);
      */
    }else{

      /*I know about these three, and they are not important.  Ctrs, Locals (SSA), Csts and Addrs are all constant*/
      if( TREE_CODE(ref) != CONSTRUCTOR &&
          TREE_CODE(ref) != SSA_NAME &&
          !TREE_CONSTANT(ref) &&
          TREE_CODE(ref) != ADDR_EXPR ){ 

        print_node(stderr,"\n\n--------------------UNHANDLED ASSIGN RHS---------------\n\n",ref,0);

      }

    }

}

void handle_write_ref(tree ref, gimple stmt, basic_block bb, gimple_stmt_iterator *gsi){

  if( TREE_CODE(ref) == MEM_REF ){

    insert_wr_mem_ref(ref,stmt,bb,gsi);

  }else if( TREE_CODE(ref) == COMPONENT_REF ){
    
    insert_wr_comp_ref(ref,stmt,bb,gsi);

  }else if( TREE_CODE(ref) == ARRAY_REF ){
    
    insert_wr_arr_ref(ref,stmt,bb,gsi);

  }else if( DECL_P(ref) ){

    if( can_escape(ref)  ){

      insert_wr( build_addr(ref,current_function_decl), bb, gsi );

    }

  }else if( TREE_CODE(ref) == BIT_FIELD_REF ){
    /*
    tree memref = TREE_OPERAND(ref,0);
    insert_wr_mem_ref(memref,stmt,bb,gsi);
    */
  }else{

    /*I know about these three, and they are not important.  Ctrs,  Csts and Addrs are all constant*/
    if( TREE_CODE(ref) != CONSTRUCTOR &&
        !TREE_CONSTANT(ref) &&
        TREE_CODE(ref) != SSA_NAME &&
        TREE_CODE(ref) != ADDR_EXPR ){ 

      print_node(stderr,"\n\n--------------------UNHANDLED ASSIGN LHS---------------\n\n",ref,0);

    }

  }

}

void insert_for_assign(basic_block bb, gimple_stmt_iterator *gsi){

  gimple stmt = gsi_stmt(*gsi);

  if( gimple_assign_rhs_class(stmt) == GIMPLE_SINGLE_RHS){
  
    tree rhs_full = gimple_assign_rhs1 (stmt); 
   
    handle_read_ref(rhs_full,stmt,bb,gsi); 
  
    tree lhs_full = gimple_assign_lhs (stmt); 
    
    handle_write_ref(lhs_full,stmt,bb,gsi); 

  }

}


void insert_for_call(basic_block bb, gimple_stmt_iterator *gsi){
   
  gimple stmt = gsi_stmt(*gsi);

  tree function = gimple_call_fndecl(stmt);

  //if( !function || !DECL_EXTERNAL(function) ){ return; }
  if( !function  ){ return; }

  unsigned num_args = gimple_call_num_args(stmt);

  int i;
  for(i = 0; i < num_args; i++){
 
    stmt = gsi_stmt(*gsi);
    bb = gimple_bb(stmt); 
 
    tree arg;

    arg = gimple_call_arg(stmt,i);

    handle_read_ref(arg,stmt,bb,gsi);
 
    tree name_id = DECL_NAME(function);
    if( name_id != NULL ){ 
      const char *name_str = IDENTIFIER_POINTER(name_id);
      if( !strncmp( name_str,"free",strlen(name_str)) ||
          !strncmp( name_str,"operator delete",strlen(name_str)) ){ 

        if( TREE_CODE(arg) == SSA_NAME ){
          insert_wr( arg, bb, gsi );
        }else{
          print_node(stderr,"\n\n--------------------UNHANDLED DEALLOCATION ARGUMENT---------------\n\n",arg,0);
        }

      }

    }

  } 

}

void insert_for_return(basic_block bb, gimple_stmt_iterator *gsi){

  gimple stmt;

  stmt = gsi_stmt(*gsi);

  tree ret = gimple_return_retval(stmt);

  if( ret ){

    handle_read_ref( ret, stmt, bb, gsi );

  }

}

void my_insert_rd_wr(basic_block bb, gimple_stmt_iterator *gsi){

  gimple stmt;

  stmt = gsi_stmt(*gsi);
  if( gimple_has_mem_ops( stmt ) && 
      gimple_code(stmt) != GIMPLE_ASSIGN  && 
      gimple_code(stmt) != GIMPLE_RETURN  && 
      gimple_code(stmt) != GIMPLE_CALL){

    fprintf(stderr,"Has MemOps we won't be touching...\n");
    fprintf(stderr,"Code=%s\n",gimple_code_name[ gimple_code(stmt) ]);
    print_gimple_stmt(stderr,stmt,0,0);
    

  }

  if( gimple_code(stmt) == GIMPLE_ASSIGN ){ 

    insert_for_assign(bb,gsi);

  }else if( gimple_code(stmt) == GIMPLE_CALL ){

    insert_for_call(bb, gsi);

  }else if( gimple_code(stmt) == GIMPLE_RETURN){

    insert_for_return(bb, gsi);

  }

}
