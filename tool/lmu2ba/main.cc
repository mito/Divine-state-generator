/* This file was created by Tomas Laurincik in 2007 as part 
   of bachelor thesis
   
   xlaurin1@fi.muni.cz
*/
#include <iostream>
#include <map>
#include <vector>
#include "buchi.hh"
#include "mu_calc_formula.hh"
#include "analyzer.hh" 

using namespace std;

int main(int argc, char** argv){   
   // read the mu-calculus formula:
   string input;
   
   if (argc < 2){             
      char data[512];
      cin.getline(data, 512);
      input = data;
   } else { 
      input = argv[1];

      if (input=="-h"){
         cerr << "This program is created to transform linear mu-calculus" << endl;
         cerr << "formula to Buchi automaton. " << endl;
         cerr << "Created by Tomas Laurincik (as part of bachelor thesis) in 2007" << endl << endl;

         cerr << "Syntax for mu-calculus formula:" << endl;
         cerr << "variable:             {a-zA-Z0-9}+" << endl;
         cerr << "and:                  [formula]&[formula]" << endl;
         cerr << "or:                   [formula]|[formula]" << endl;
         cerr << "negation:             ![var]" << endl;
         cerr << "next:                 O(formula)" << endl;
         cerr << "least fixed point:    m([var])([formula])" << endl;
         cerr << "greatest fixed point: n([var])([formula])\n" << endl;

         cerr << "Some requirements must be satisfied for the formula:" << endl;
         cerr << "  (1) Negation can only be applied to variables" << endl;
         cerr << "  (2) Negation cannot be applied to recursive variable" << endl;
         cerr << "  (3) Recursive variable can only appear in a scope of the next time operator\n" << endl;

         return 0;
      }
   }

   syntactic_analyzer analyzer;
   mu_calc_formula* f = analyzer.analyze(input);
   if(f == NULL) {
     cerr << "Error while analyzing formula - check your syntax" << endl;
   } else {
      buchi_automaton a = f->to_buchi();
      cout << a << endl; 
   }
} 
