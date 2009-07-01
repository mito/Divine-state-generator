/******************************************************************************
 *                                                                            *
 * This program tests expressions in DiVinE/Library                           *
 *                                                                            *
 ******************************************************************************/

#include <iostream>
#include <string>
#include "sevine.h"

using namespace std;
using namespace divine;

static string trans_str[3] =
{ "q_i -> idle { guard m==n; }",
  "q_i -> idle { guard m==n; effect m = 1+n; }",
  "q_i -> idle { guard m==n; sync kanal!m+1; }" };


int main()
{
 dve_system_t dve_system(gerr);
 dve_symbol_table_t & Table = *dve_system.get_symbol_table();
 dve_symbol_t * a = new dve_symbol_t(SYM_VARIABLE, "m", 1),
              * b = new dve_symbol_t(SYM_VARIABLE, "n", 1);
 
 a->set_valid(true);
 b->set_valid(true);
 a->set_const(false);
 b->set_const(false);
 a->set_var_type(VAR_BYTE);
 b->set_var_type(VAR_INT);
 a->set_init_expr(0);
 b->set_init_expr(0);
 a->set_process_gid(NO_ID);
 b->set_process_gid(NO_ID);
 a->set_lid(0);
 b->set_lid(1);
 
 Table.add_variable(a);
 Table.add_variable(b);
 
 dve_symbol_t * proc = new dve_symbol_t(SYM_PROCESS, "ahoj_jsem_process", 17);
 proc->set_valid(true);
 proc->set_process_gid(NO_ID);

 Table.add_process(proc);
 
 dve_symbol_t * state1 = new dve_symbol_t(SYM_STATE, "q_i", 3),
              * state2 = new dve_symbol_t(SYM_STATE, "idle", 4);
 state1->set_valid(true);
 state2->set_valid(true);
 state1->set_process_gid(proc->get_gid());
 state2->set_process_gid(proc->get_gid());
 
 Table.add_state(state1);
 Table.add_state(state2);
 
 dve_symbol_t * channel1 =  new dve_symbol_t(SYM_CHANNEL, "kanal",5);
 channel1->set_valid(true);
 channel1->set_channel_typed(false);
 channel1->set_channel_item_count(1);
 state1->set_process_gid(NO_ID);
 
 Table.add_channel(channel1);
 
 dve_transition_t my_trans(&dve_system);
 
 bool OK = true;
 string aux;
 
 for (int i=0; i<=2 && OK; ++i)
  {
   cout << "Testing transition:\n" << trans_str[i] << endl;
   my_trans.from_string(trans_str[i], proc->get_gid());
   aux = my_trans.to_string();
   if (aux != trans_str[i])
    {
     OK = false;
     cerr << "Test failure: " << trans_str[i] << " != " << aux << endl;
    }
  }
 
 if (!OK) return 1; else { cout << "Test successful" << endl; return 0; }
}

