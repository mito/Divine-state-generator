/* This file was created by Tomas Laurincik in 2007 as part 
   of bachelor thesis
   
   xlaurin1@fi.muni.cz
*/
#include <iostream>
#include <string>
#include <map>
#include <set>

#include "lexical_analyzer.hh"

using namespace std; 

char lexical_analyzer::get_next_char(){
   if (position == analyzed_string.length()){
      end = true;
      return 0;
   }else{              
      return analyzed_string[position++];
   }            
}

lexical_analyzer::lexical_analyzer(string s){
  this->analyzed_string = s; 
  this->last_char = 0; 
  this->position = 0;
  this->end = false;
  this->variable_count = 0;
  codes_mapping = new map<string, int>();
}

lexical_analyzer::~lexical_analyzer(){
  delete codes_mapping;
  codes_mapping = NULL;
}

set<int> lexical_analyzer::get_universe(){
  return universe;
}
  
map<int, string> lexical_analyzer::get_names_mapping(){
  return names_mapping;
}

lexical_symbol* lexical_analyzer::get_next(){
     // if this is first call of this method, initialize the last_char
     if (position == 0){
        last_char =  get_next_char(); 
     }

     // if no next symbol is available, return NULL
     if (end)
        return NULL;
     
     lexical_symbol* result = new lexical_symbol();

     // initialize the automaton 
     int automaton_state = 0;
     string name = "";

     // ==== run the automaton ====
     while(automaton_state!=3){
        switch(automaton_state){
           case 0: {
              if (last_char == SYMBOL_LEFT_PARENTHESIS){
                 result->symbol_code = CODE_LEFT_PARENTHESIS;
                 automaton_state=1;

              }else if (last_char == SYMBOL_RIGHT_PARENTHESIS){
                 result->symbol_code = CODE_RIGHT_PARENTHESIS;
                 automaton_state=1;

              }else if (last_char == SYMBOL_AND){
                 result->symbol_code = CODE_AND;
                 automaton_state=1;

              }else if (last_char == SYMBOL_OR){
                 result->symbol_code = CODE_OR;
                 automaton_state=1;

              }else if (last_char == SYMBOL_NOT){
                 result->symbol_code = CODE_NOT;
                 automaton_state=1;

      } else if (isalnum(last_char)){
	 result->symbol_code = CODE_VARIABLE;
	 name = last_char;
	 automaton_state=2;

      } else if (isspace(last_char)){
	 automaton_state=0;

      } else
	 result = NULL;

      break;
   }

   case 1:{
      automaton_state = 3;
      break;
   }

   case 2: {
      if (isalnum(last_char)){
                 name = name+last_char;
                 automaton_state = 2; 

              } else {
                 automaton_state = 3;
              }
              break;
           }
        }       
        
        if (automaton_state != 3)   
           last_char = get_next_char();
     }

     // before sending the result, we must identify possible keywords
     // and if we find one, change the symbol_code to the corresponding code.
     if (result!=NULL && result->symbol_code==CODE_VARIABLE) {
        if (name == KEYWORD_NEXT){
           result->symbol_code = CODE_NEXT; 

        }else if (name == KEYWORD_LEAST_FP){
           result->symbol_code = CODE_LEAST_FP; 

        }else if (name == KEYWORD_GREATEST_FP){
           result->symbol_code = CODE_GREATEST_FP; 

        }else {           
           result->variable_name = name;
	   if (codes_mapping->count(name)>0)
              result->variable_code = (*codes_mapping)[name];
           else{
              result->variable_code = variable_count++;
              universe.insert(result->variable_code);
              names_mapping[result->variable_code] = result->variable_name;
              (*codes_mapping)[name] = result->variable_code;
              
           }
        }
     }

     return result;
}
