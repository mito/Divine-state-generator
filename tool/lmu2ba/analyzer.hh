/* This file was created by Tomas Laurincik in 2007 as part 
   of bachelor thesis
   
   xlaurin1@fi.muni.cz
*/
#ifndef ANALYZER_H
#define ANALYZER_H

#include <iostream>
#include <string>
#include <vector>

#include "lexical_analyzer.hh"
#include "mu_calc_formula.hh"

using namespace std; 
/* =====================================================================
                             declarations  
   ===================================================================== */  

  /* definition of codes for stack symbols
  */  
  #define STACK_LEFT_PARENTHESIS CODE_LEFT_PARENTHESIS
  #define STACK_RIGHT_PARENTHESIS CODE_RIGHT_PARENTHESIS
  #define STACK_AND CODE_AND
  #define STACK_OR CODE_OR
  #define STACK_NOT CODE_NOT
  #define STACK_NEXT CODE_NEXT
  #define STACK_LEAST_FP CODE_LEAST_FP
  #define STACK_GREATEST_FP CODE_GREATEST_FP
  #define STACK_VARIABLE CODE_VARIABLE
  #define STACK_S CODE_VARIABLE+1
  #define STACK_F CODE_VARIABLE+2
  #define STACK_E CODE_VARIABLE+3

/* =====================================================================
                SYNTACTIC ANALYZER class for mu_calc_formula
   ===================================================================== 
   This class represents syntactic analyzer for the expressions of mu-
   -calculus. The LL(1) grammar for mu_calc_formula is:
       [S] -> [var] | ![E] | O([F]) | m([var])([F]) | n([var])([F])
       [F] -> [S] | & [E] [E] | | [E] [E]
       [E] -> [F] | ([F])
   The method that does the analysis is ANALYZE, which return pointer to
   the instance of mu_calc_formula class. 
*/  

  class syntactic_analyzer{
     private:
  // --- private methods ---
       void add_to_incubator(vector<mu_calc_formula*>*, mu_calc_formula*);
     public: 
  // --- constructor ---
       syntactic_analyzer(){};

  // --- main class methods ---
       mu_calc_formula* analyze(string);
  };

#endif // ANALYZER_H
