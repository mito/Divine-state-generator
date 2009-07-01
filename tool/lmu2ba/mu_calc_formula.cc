/* This file was created by Tomas Laurincik in 2007 as part 
   of bachelor thesis
   
   xlaurin1@fi.muni.cz
*/
#include <iostream>
#include <vector>
#include "mu_calc_formula.hh"

using namespace std;

/*  =======================================================================
                               definitions
    ========================================================================
*/
mu_calc_formula::mu_calc_formula(){
   operands = NULL;
   variable_code = -1;
   type = FORMULA_TYPE_UNKNOWN;
}

mu_calc_formula::mu_calc_formula(unsigned short int t, 
                                 int variable, 
                                 set<int> univ,
                                 map<int, string> mapping){
   operands = NULL;
   universe = univ;
   names_mapping = mapping;
   variable_code = variable;
   type = t<=FORMULA_TYPES_MAX?t:FORMULA_TYPE_UNKNOWN;
}

mu_calc_formula::mu_calc_formula(unsigned short int t, 
                                 set<int> univ, 
                                 map<int, string> mapping){
  operands = NULL;
  universe = univ;
  names_mapping = mapping;
  variable_code = -1;
  type = t<=FORMULA_TYPES_MAX?t:FORMULA_TYPE_UNKNOWN;
}

mu_calc_formula::~mu_calc_formula(){
  delete operands;
  operands = NULL;
}

// --- main formula methods ---
//    - getters
short int mu_calc_formula::get_arity(){
  return FORMULA_ARITIES[type];
}

unsigned short int mu_calc_formula::get_type(){
  return type;
}

mu_calc_formula* mu_calc_formula::get_operand(unsigned int n){
  if (operands!=NULL && operands->size()>n)
     return operands->at(n);
  return NULL;
}

// IS_PROPERLY_SET returns true, it this formula has the number of operands,
// defined by its arity.
bool mu_calc_formula::is_properly_set(){
  bool result = false;

  if ((operands == NULL && FORMULA_ARITIES[type]==0) ||
    (operands!=NULL && FORMULA_ARITIES[type]==operands->size()))
  result = true;

  return result;
}

//    - set the universe
void mu_calc_formula::set_universe(set<int> univ){
  universe = univ;
}

//    - adding operands
void mu_calc_formula::push(mu_calc_formula* operand){
  if (operands==NULL){
    operands = new vector<mu_calc_formula*>();
  }
  operands->push_back(operand);
}



buchi_automaton mu_calc_formula::to_buchi(){
   buchi_automaton result;
   result.setuniverse(universe);
   result.setmapping(names_mapping);

   switch (this->get_type()){
      case FORMULA_TYPE_VARIABLE: {
         int var[1] = {this->variable_code};

         result.setinitialstate(0);
         result.addtransition(0, new set<int>(var, var+1), new set<int>(), 1);
         result.addtransition(1, new set<int>(), new set<int>(), 1);
         result.addacceptingstate(1);

         break;
      }

      case FORMULA_TYPE_NOT: {
         mu_calc_formula* temp = *(this->operands->begin());
         buchi_automaton a  = temp->to_buchi();
         result = -a;

         break;
      }

      case FORMULA_TYPE_OR: {
         vector<mu_calc_formula*>::iterator it = this->operands->begin();

         result = (*it)->to_buchi() + ((*(it+1))->to_buchi());
         break;
      }

      case FORMULA_TYPE_AND: {
         vector<mu_calc_formula*>::iterator it = this->operands->begin();
         result = (*it)->to_buchi() * ((*(it+1))->to_buchi());

         break;
      }
      case FORMULA_TYPE_NEXT: {
         vector<mu_calc_formula*>::iterator it = this->operands->begin();
         buchi_automaton temp = (*it)->to_buchi();

         result.setinitialstate(0);
         result.addtransitions(temp, 1);
         result.addacceptingstates(temp, 1);
         result.addtransition(0, new set<int>(), new set<int>(), 1);

         break;
      }

      case FORMULA_TYPE_GREATEST_FP: {
         vector<mu_calc_formula*>::iterator it = this->operands->begin();
         int recursion_variable = (*it)->variable_code;

         buchi_automaton  temp = (*(it+1))->to_buchi();
         itmd_automaton itmd = temp.getitmdautomaton(recursion_variable);
         result = getgfpautomaton(itmd).automaton;

         break;
      }

      case FORMULA_TYPE_LEAST_FP: {
         vector<mu_calc_formula*>::iterator it = this->operands->begin();
         int recursion_variable = (*it)->variable_code;

         buchi_automaton temp =  (*(it+1))->to_buchi();
         itmd_automaton itmd = temp.getitmdautomaton(recursion_variable);

         result = getlfpautomaton(itmd);

         break;
      }
   }


   return result;
}
