/* This file was created by Tomas Laurincik in 2007 as part 
   of bachelor thesis
   
   xlaurin1@fi.muni.cz
*/
#ifndef LEXICAL_ANALYZER_H
#define LEXICAL_ANALYZER_H

#include <string>
#include <map>
#include <set>

using namespace std;

  /* definition of keywords
  */
  #define KEYWORD_NEXT "O"
  #define KEYWORD_LEAST_FP "m"
  #define KEYWORD_GREATEST_FP "n"
  #define KEYWORD_AND "and"
  #define KEYWORD_OR "or"
  #define KEYWORD_NOT "not"

  /* definition of symbols
  */
  #define SYMBOL_LEFT_PARENTHESIS '('
  #define SYMBOL_RIGHT_PARENTHESIS ')'
  #define SYMBOL_AND '&'
  #define SYMBOL_OR '|'
  #define SYMBOL_NOT '!'

  /* definition of codes for lexical symbols
  */
  #define CODE_LEFT_PARENTHESIS 0
  #define CODE_RIGHT_PARENTHESIS 1
  #define CODE_AND 2
  #define CODE_OR 3
  #define CODE_NOT 4
  #define CODE_NEXT 5
  #define CODE_LEAST_FP 6
  #define CODE_GREATEST_FP 7
  #define CODE_VARIABLE 8

/* =====================================================================
                LEXICAL ANALYZER class for mu_calc_formula
   ===================================================================== */  
  struct lexical_symbol{
     unsigned int symbol_code;
     // if symbol is variable, put his code and name here
     int variable_code;
     string variable_name; 
  };
  
  class lexical_analyzer{
     private: 
        string analyzed_string;
        char last_char;
        unsigned int position;
        bool end;
        int variable_count; 
       
        map<string, int>* codes_mapping;
        map<int, string> names_mapping; 
        set<int> universe;

  // --- private methods ---
        char get_next_char();
     public:
  // --- constructors ---
        lexical_analyzer(string s);

  // --- destructor ---
        ~lexical_analyzer();

  // --- main formula methods ---
        lexical_symbol* get_next();
        set<int> get_universe();
        map<int, string> get_names_mapping();
  };

#endif
