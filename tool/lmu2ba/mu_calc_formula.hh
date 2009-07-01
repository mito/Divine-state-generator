/* This file was created by Tomas Laurincik in 2007 as part 
   of bachelor thesis
   
   xlaurin1@fi.muni.cz
*/
#ifndef MU_CALC_FORMULA_H
#define MU_CALC_FORMULA_H

#include <iostream>
#include <string>
#include <vector>
#include "buchi.hh"

  /* definitions of constants for linear time mu-calculus formulae.
       - FORMULA_TYPE_? : defines formula type
  */
  #define FORMULA_TYPES_MAX 7 
  #define FORMULA_TYPE_UNKNOWN 0 
  #define FORMULA_TYPE_AND 1 
  #define FORMULA_TYPE_OR 2 
  #define FORMULA_TYPE_NOT 3   
  #define FORMULA_TYPE_NEXT 4 
  #define FORMULA_TYPE_LEAST_FP 5 
  #define FORMULA_TYPE_GREATEST_FP 6 
  #define FORMULA_TYPE_VARIABLE 7 
  
  // - FORMULA_DESCRIPTIONS: defines formula type descriptions
  static const string FORMULA_DESCRIPTIONS[] = {"unknown", 
						"conjunction", 
                                                "disjunction", 
                                                "negation",
                                                "next",
                                                "least fixed point",
 						"greatest fixed point",
                                                "variable" };

  // - FORMULA_ARITIES: defines formula arities
  static const unsigned short int FORMULA_ARITIES[] = {0, 
						       2, 
                                                       2, 
                                                       1,
                                                       1,
                                                       2,
 						       2,
                                                       0};


/* =====================================================================
                        MU-CALCULUS FORMULA class
   ===================================================================== */  
  class mu_calc_formula {
     private: 
        vector<mu_calc_formula*>* operands;
        unsigned short int type;
        set<int> universe;
        map<int, string> names_mapping;

        // variable_code is needed if this formula is VARIABLE
        int variable_code;
     public:
  // --- constructors ---
       mu_calc_formula();
       mu_calc_formula(unsigned short int t, int variable, set<int> univ, 
                        map<int, string> mapping);
       mu_calc_formula(unsigned short int t, set<int> univ, map<int, string> mapping);

  // --- destructor ---
       ~mu_calc_formula();

  // --- main formula methods ---
       short int get_arity();
       unsigned short int get_type();
       mu_calc_formula* get_operand(unsigned int n);
       bool is_properly_set();
       void set_universe(set<int> univ);
       void push(mu_calc_formula* operand);
       buchi_automaton to_buchi();
  };

#endif //MU_CALC_FORMULA_H
