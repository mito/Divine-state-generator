/* This file was created by Tomas Laurincik in 2007 as part 
   of bachelor thesis
   
   xlaurin1@fi.muni.cz
*/
#ifndef TRANSFORMER_H
#define TRANSFORMER_H

#include <string>
#include <vector>
#include <set>
#include <map>
#include "lexical_analyzer.hh"

/* =====================================================================
                INFIX TO PREFIX TRANSFORMER class for mu_calc_formula
   ===================================================================== 
   This class is working in a top of lexical analyzer. It transforms for- 
   mulas from infix to prefix notation - namely operations "and" and "or",
   since these are only ones with infix notation in definition of mu-cal-
   culus formula. It also removes unimportant parenthesis and (what is 
   very important) creates the universe of variables which are involved
   in these lexical_symbols (from lexical analyzer) and the names mapping
   for them.
*/ 
  class infix_to_prefix_transformer{
     private:
       vector<lexical_symbol*>* buffer;
       lexical_analyzer* analyzer;

  // --- private methods ---
  //      - find_group_start - finds the beginning of the group ending at the 
  //                           given position. 
       int find_group_start(int);       
     public: 
  // --- constructor ---
       infix_to_prefix_transformer(lexical_analyzer* a);
  // --- destructor ---
       ~infix_to_prefix_transformer();

  // --- main class methods ---
       lexical_symbol* get_next();
       void transform();

  // --- getters ---
       set<int> get_universe();
       map<int, string> get_names_mapping();       
  };

#endif
