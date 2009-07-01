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

static string proc_str[3] =
{ "process small_process {\n"
  "byte m;\n"
  "byte n;\n"
  "state q_i, idle;\n"
  "init q_i;\n"
  "trans\n"
  "q_i -> idle { guard m==n; };\n"
  "}\n",
  
  "process cabin {\n"
  "byte t = 3;\n"
  "byte req[2] = {0, 1};\n"
  "state idle, mov, open;\n"
  "init idle;\n"
  "trans\n"
  "idle -> mov { guard v>0; },\n"
  "mov -> open { guard t==p; },\n"
  "mov -> mov { guard t<p; effect p = p-1; },\n"
  "mov -> mov { guard t>p; effect p = p+1; },\n"
  "open -> idle { effect req[p] = 0, v = 0; };\n"
  "}\n",

  "process ETS {\n"
  "state q;\n"
  "init q;\n"
  "trans\n"
  "q -> q { guard p>=2; sync electrons?; "
  "effect p = p-2, v = v+2; };\n"
  "}\n"
  };


int main()
{
 dve_system_t dve_system(gerr);
 dve_symbol_table_t & Table = *dve_system.get_symbol_table();
 dve_symbol_t * a = new dve_symbol_t(SYM_VARIABLE, "p", 1),
              * b = new dve_symbol_t(SYM_VARIABLE, "v", 1);
 
 a->set_valid(true);
 b->set_valid(true);
 a->set_lid(0);
 b->set_lid(1);
 a->set_const(false);
 b->set_const(false);
 a->set_var_type(VAR_BYTE);
 b->set_var_type(VAR_INT);
 a->set_init_expr(0);
 b->set_init_expr(0);
 a->set_process_gid(NO_ID);
 b->set_process_gid(NO_ID);
 
 Table.add_variable(a);
 Table.add_variable(b);
 
 dve_symbol_t * channel1 =  new dve_symbol_t(SYM_CHANNEL, "electrons",9);
 channel1->set_valid(true);
 channel1->set_channel_typed(false);
 channel1->set_channel_item_count(0);
 
 Table.add_channel(channel1);
 
 dve_process_t my_proc(&dve_system);
 
 bool OK = true;
 string aux;
 
 for (int i=0; i<=2 && OK; ++i)
  {
   cout << "Testing process:\n" << proc_str[i] << endl;
   if (!my_proc.from_string(proc_str[i]))
    {
     aux = my_proc.to_string();
     if (aux != proc_str[i])
      {
       OK = false;
       cerr << "Test failure:\n"<< proc_str[i] << "\n\n  !=\n\n" << aux << endl;
      }
    }
   else OK = false;
  }
 
 if (!OK) return 1; else { cout << "Test successful" << endl; return 0; }
}

