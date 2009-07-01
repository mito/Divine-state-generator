/* This file was created by Tomas Laurincik in 2007 as part 
   of bachelor thesis
   
   xlaurin1@fi.muni.cz
*/
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>

#include "lexical_analyzer.hh"
#include "transformer.hh"

using namespace std;

infix_to_prefix_transformer::infix_to_prefix_transformer(lexical_analyzer* a){
  this->analyzer = a;
  transform();
}

infix_to_prefix_transformer::~infix_to_prefix_transformer(){
  delete buffer;
  delete analyzer;
  buffer = NULL;
  analyzer = NULL;
}

set<int> infix_to_prefix_transformer::get_universe(){
  return analyzer->get_universe();
}

map<int, string> infix_to_prefix_transformer::get_names_mapping(){
  return analyzer->get_names_mapping();
}

/* FIND_GROUP_START : finds the beginning of the group ending at given position. Group can be:
      - variable name - in this case it begins where it ends.
      - fixed point expression - the group begins with the keyword KEYWORD_LEAST_FP or KEYWORD_GREATEST_FP
      - negation or next expression - the group begins with the keyword KEYWORD_NOT, KEYWORD_NEXT
      - parenthesis group - the group begins with the left parenthesis that is closed by right parenthesis 
        	at the end of the group
*/
int infix_to_prefix_transformer::find_group_start(int end){
   int j = end;
   int left_count = 0;
   int right_count = 0;
   bool finish = false;
   
   // we need to count left and right parenthesis and in the case they are equal,
   // do some neccessary shifting.
   while(j>=0 && !finish){      
      if ((buffer->at(j))->symbol_code == CODE_RIGHT_PARENTHESIS){ 
         right_count++;
      }else if ((buffer->at(j))->symbol_code == CODE_LEFT_PARENTHESIS){
         left_count++;
      }      

      if(left_count-right_count == 0){
         if (j-1>=0){
            // we must move the beginning of the group to the left:
            switch((buffer->at(j-1))->symbol_code){
              // in case, this is NOT-group, move one to the left
               case CODE_NOT : j--; break;
              // in case, this is NEXT-group, move one to the left
               case CODE_NEXT : j--; break;
              // in case, this is ?_FP-group, move one to the left
               case CODE_RIGHT_PARENTHESIS : j-=4; break; 
            }
         }
         // ... finish searching
         finish = true;
      }else
         j--;
   }
   // return -1 if failed
   if (j<0 || !finish) 
      j=-1;
   return j;
}

/* TRANSFORM : transforms the expression from infix to postfix notation. This method
       fills the buffer with lexical symbols. If the symbol with code CODE_AND or 
       CODE_OR is found, it determines the beginning of the first operand in the buffer and
       inserts the symbol there. When it ends, buffer is filled with lexical symbols for
       given expression in prefix notation.
*/
void infix_to_prefix_transformer::transform(){
   buffer = new vector<lexical_symbol*>(); 
   lexical_symbol* symbol;
  
   // fill buffer with lexical symbols.
   int group_start = -1;
   while((symbol = analyzer->get_next()) != NULL){  
      // **** remove unneccessary parenthesis
      if (symbol->symbol_code == CODE_RIGHT_PARENTHESIS){
         // add right parenthesis to the buffer and find corresponding left parenthesis
         buffer->push_back(symbol);
         int right = buffer->size()-1; 
         int left = find_group_start(right);

         // if not found, end (syntactic error)
         if (left == -1){
            continue;
         }

         // reduce parenthesis
         while (buffer->size()!=0 && left<right && right>0) {
         // 1. test if there are two parenthesis at the beginning and end of the group
            right--;
            left++;
            if (buffer->at(right)->symbol_code == CODE_RIGHT_PARENTHESIS &&
                buffer->at(left)->symbol_code == CODE_LEFT_PARENTHESIS){
         // 2. if yes, test if these two begin and end the same group         
               group_start = find_group_start(right);
               if (group_start == left){
         // 3. if yes, reduce them
                  buffer->erase(buffer->begin()+right);
                  buffer->erase(buffer->begin()+left);       
                  right = buffer->size()-1;
                  left--;
               }else{
         // -3. if no, end reducing
                  break;
               }         
            }else{
         // -2. if no, end reducing
               break;
            }
         }

      } else if (symbol->symbol_code == CODE_AND || symbol->symbol_code == CODE_OR){
      // **** transform to prefix notation
         group_start = find_group_start(buffer->size()-1);
         buffer->insert(buffer->begin()+group_start, symbol);

      }else{
      // **** if no transformation needed for this symbol, only add it to the buffer
         buffer->push_back(symbol);

      // and if the symbol is variable, put it to the universe and the mapping
          
      }
   } 
}

/* GET_NEXT : gets the next lexical symbol or NULL, if there isn't any. This method takes
        lexical symbols from the buffer, if it's not empty.
*/
lexical_symbol* infix_to_prefix_transformer::get_next(){
   lexical_symbol* result = NULL;
   if (buffer != NULL){
      if (!buffer->empty()){
         result = *buffer->begin();
         buffer->erase(buffer->begin());
      }else{
         delete buffer;
         buffer = NULL;
      }
   }
   return result;
}
