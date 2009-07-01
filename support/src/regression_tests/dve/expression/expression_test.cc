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

static string expr_str[3] =
{ "a*b&(6%a) and 123/b+14",
  "123123+3-18 and  not (a+(400))",
  "a" };


int main()
{
 dve_system_t dve_system(gerr);
 dve_symbol_table_t & Table = *dve_system.get_symbol_table();
 dve_symbol_t * a = new dve_symbol_t(SYM_VARIABLE, "a", 1),
              * b = new dve_symbol_t(SYM_VARIABLE, "b", 1);
 
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
 
 Table.add_variable(a);
 Table.add_variable(b);

 dve_expression_t my_expr(&dve_system);
 
 bool OK = true;
 string aux;
 
 for (int i=0; i<=2 && OK; ++i)
  {
   cout << "Testing expression: " << expr_str[i] << endl;
   my_expr.from_string(expr_str[i]);
   aux = my_expr.to_string();
   if (aux != expr_str[i])
    {
     OK = false;
     cerr << "Test failure: " << expr_str[i] << " != " << aux << endl;
    }
  }
 
 if (!OK) return 1; else { cout << "Test successful" << endl; return 0; }
}

