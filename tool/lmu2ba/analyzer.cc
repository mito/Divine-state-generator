/* This file was created by Tomas Laurincik in 2007 as part 
   of bachelor thesis
   
   xlaurin1@fi.muni.cz
*/
#include <iostream>
#include <string>
#include <vector>

#include "analyzer.hh"
#include "transformer.hh"
#include "mu_calc_formula.hh"

using namespace std;

/* ADD_TO_INCUBATOR: This method adds given formula to the incubator             
*/
void syntactic_analyzer::add_to_incubator(vector<mu_calc_formula*>* incubator, mu_calc_formula* formula){
   vector<mu_calc_formula*>::reverse_iterator it=incubator->rbegin();

   // find the parent formula for this formula
   while(it!=incubator->rend()){     
      if (!(*it)->is_properly_set()){
         (*it)->push(formula);
         break;
      }     
      it++;
   }

   // test if it is neccessary to add this formula to incubator
   if (formula->get_arity()!=0 || incubator->size()==0){
      incubator->push_back(formula); 
   }
}

/* ANALYZE: This method does the LL(1) analysis.             
*/
mu_calc_formula* syntactic_analyzer::analyze(string s){
   // create some temporal variables:
   // - stack : this vector will act as PDA's stack
   vector<unsigned int>* stack = new vector<unsigned int>();
   // - incubator : this vector will be used for creating the result mu_calc_formula class. 
   vector<mu_calc_formula*>* incubator = new vector<mu_calc_formula*>();
  
   // create the transformer on a top of the lexical_analyzer and get the pointer to the 
   // universe of variables
   infix_to_prefix_transformer transformer(new lexical_analyzer(s));
   
   set<int> universe = transformer.get_universe();
   map<int, string> names_mapping = transformer.get_names_mapping();

   // initialize the PDA automaton
   stack->push_back(STACK_E);
   lexical_symbol* next = transformer.get_next();

   while(!stack->empty() && next!=NULL){
      unsigned int top = stack->back();
 
      switch(top){
         case STACK_S : {
            switch(next->symbol_code){
                case CODE_VARIABLE : {
                   stack->erase(stack->end()-1);
                   stack->push_back(STACK_VARIABLE);

                   break;
                }
                case CODE_NOT : {
                   stack->erase(stack->end()-1);
                   stack->push_back(STACK_E);
                   stack->push_back(STACK_NOT);
                   break;
                }
                case CODE_NEXT : {
                   stack->erase(stack->end()-1);
                   stack->push_back(STACK_RIGHT_PARENTHESIS);
                   stack->push_back(STACK_F);
                   stack->push_back(STACK_LEFT_PARENTHESIS);
                   stack->push_back(STACK_NEXT);

                   break;
                }
                case CODE_LEAST_FP : {
                   stack->erase(stack->end()-1);
                   stack->push_back(STACK_RIGHT_PARENTHESIS);
                   stack->push_back(STACK_F);
                   stack->push_back(STACK_LEFT_PARENTHESIS);
                   stack->push_back(STACK_RIGHT_PARENTHESIS);
                   stack->push_back(STACK_VARIABLE);
                   stack->push_back(STACK_LEFT_PARENTHESIS);
                   stack->push_back(STACK_LEAST_FP);

                   break;
                }
                case CODE_GREATEST_FP : {
                   stack->erase(stack->end()-1);
                   stack->push_back(STACK_RIGHT_PARENTHESIS);
                   stack->push_back(STACK_F);
                   stack->push_back(STACK_LEFT_PARENTHESIS);
                   stack->push_back(STACK_RIGHT_PARENTHESIS);
                   stack->push_back(STACK_VARIABLE);
                   stack->push_back(STACK_LEFT_PARENTHESIS);
                   stack->push_back(STACK_GREATEST_FP);

                   break;
                }
                default:{ 
                   return NULL;
                }
            }
            break;
         }
         case STACK_F : {
            switch(next->symbol_code){
                case CODE_AND : {
                   stack->erase(stack->end()-1);
                   stack->push_back(STACK_E);
                   stack->push_back(STACK_E);
                   stack->push_back(STACK_AND);

                   break;
                }
                case CODE_OR : {
                   stack->erase(stack->end()-1);
                   stack->push_back(STACK_E);
                   stack->push_back(STACK_E);
                   stack->push_back(STACK_OR);

                   break;
                }
                case CODE_VARIABLE : {
                   stack->erase(stack->end()-1);
                   stack->push_back(STACK_S);
                   break;
                }
                case CODE_NOT : {
                   stack->erase(stack->end()-1);
                   stack->push_back(STACK_S);
                   break;
                }
                case CODE_NEXT : {
                   stack->erase(stack->end()-1);
                   stack->push_back(STACK_S);

                   break;
                }
                case CODE_LEAST_FP : {
                   stack->erase(stack->end()-1);
                   stack->push_back(STACK_S);

                   break;
                }
                case CODE_GREATEST_FP : {
                   stack->erase(stack->end()-1);
                   stack->push_back(STACK_S);

                   break;
                }
                default:{ 
                   return NULL;
                }
            }
            break;
         }
         case STACK_E : {
            switch(next->symbol_code){
                case CODE_LEFT_PARENTHESIS : {
                   stack->erase(stack->end()-1);
                   stack->push_back(STACK_RIGHT_PARENTHESIS);
                   stack->push_back(STACK_F);
                   stack->push_back(STACK_LEFT_PARENTHESIS);
                   break;
                }
                case CODE_VARIABLE : {
                   stack->erase(stack->end()-1);
                   stack->push_back(STACK_F);
                   break;
                }
                case CODE_NOT : {
                   stack->erase(stack->end()-1);
                   stack->push_back(STACK_F);
                   break;
                }
                case CODE_NEXT : {
                   stack->erase(stack->end()-1);
                   stack->push_back(STACK_F);

                   break;
                }
                case CODE_LEAST_FP : {
                   stack->erase(stack->end()-1);
                   stack->push_back(STACK_F);

                   break;
                }
                case CODE_GREATEST_FP : {
                   stack->erase(stack->end()-1);
                   stack->push_back(STACK_F);

                   break;
                }
                case CODE_AND : {
                   stack->erase(stack->end()-1);
                   stack->push_back(STACK_F);
                   break;
                }
                case CODE_OR : {
                   stack->erase(stack->end()-1);
                   stack->push_back(STACK_F);
                   break;
                }
               default:{ 
                   return NULL;
                }
            }
            break;
         }
         // terminals:
         case CODE_VARIABLE:{
             if (next->symbol_code == CODE_VARIABLE){                
                add_to_incubator(incubator, new mu_calc_formula(FORMULA_TYPE_VARIABLE, next->variable_code,
                                                                universe, names_mapping));
                
		stack->erase(stack->end()-1); 
                next = transformer.get_next();
             }else{
                return NULL;
             }
             break;
         }
         case CODE_LEFT_PARENTHESIS:{
             if (next->symbol_code == CODE_LEFT_PARENTHESIS){
		stack->erase(stack->end()-1); 
                next = transformer.get_next();
             }else{
                return NULL;
             }
             break;
         }
         case CODE_RIGHT_PARENTHESIS:{
             if (next->symbol_code == CODE_RIGHT_PARENTHESIS){
                stack->erase(stack->end()-1); 
                next = transformer.get_next();
             }else{
                return NULL;
             }
             break;
         }
         case CODE_AND:{
             if (next->symbol_code == CODE_AND){
                add_to_incubator(incubator, new mu_calc_formula(FORMULA_TYPE_AND, universe, names_mapping));
                
                stack->erase(stack->end()-1); 
                next = transformer.get_next();
             }else{
                return NULL;
             }
             break;
         }
         case CODE_OR:{
             if (next->symbol_code == CODE_OR){
                add_to_incubator(incubator, new mu_calc_formula(FORMULA_TYPE_OR, universe, names_mapping));
                
                stack->erase(stack->end()-1); 
                next = transformer.get_next();
             }else{
                return NULL;
             }
             break;
         }
         case CODE_NOT:{
             if (next->symbol_code == CODE_NOT){
                add_to_incubator(incubator, new mu_calc_formula(FORMULA_TYPE_NOT, universe, names_mapping));
                
                stack->erase(stack->end()-1); 
                next = transformer.get_next();
             }else{
                return NULL;
             }
             break;
         }
         case CODE_NEXT:{
             if (next->symbol_code == CODE_NEXT){
                add_to_incubator(incubator, new mu_calc_formula(FORMULA_TYPE_NEXT, universe, names_mapping));
                
                stack->erase(stack->end()-1); 
                next = transformer.get_next();
             }else{
                return NULL;
             }
             break;
         }
         case CODE_LEAST_FP:{
             if (next->symbol_code == CODE_LEAST_FP){
                add_to_incubator(incubator, new mu_calc_formula(FORMULA_TYPE_LEAST_FP, universe, names_mapping));
                
                stack->erase(stack->end()-1); 
                next = transformer.get_next();
             }else{
                return NULL;
             }
             break;
         }
         case CODE_GREATEST_FP: { 
             if (next->symbol_code == CODE_GREATEST_FP){
                add_to_incubator(incubator, new mu_calc_formula(FORMULA_TYPE_GREATEST_FP, universe, names_mapping));
                
                stack->erase(stack->end()-1); 
                next = transformer.get_next();
             }else{
                return NULL;
             }
             break;
         }
         default:{ 
            return NULL;
         }
      }
   }
   if (next==NULL && stack->empty())
      return incubator->at(0);

   return NULL;
}
