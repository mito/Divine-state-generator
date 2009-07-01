/* This file was created by Tomas Laurincik in 2007 as part 
   of bachelor thesis
   
   xlaurin1@fi.muni.cz
*/
#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <algorithm>
#include <iterator>

#include "buchi.hh"

using namespace std;

// CREATE_TOUPLE: This function creates new state_touple.
state_touple create_touple(int s1, int s2,int flag){
   state_touple result;
   result.state1 = s1;
   result.state2 = s2;
   result.flag = flag;

   return result;
}

buchi_automaton::buchi_automaton(const buchi_automaton& a){
  initial_state = a.initial_state;

  if (a.accepting_states != NULL){
    accepting_states = new set<int>(*(a.accepting_states));
  }else{
    accepting_states = NULL;
  }

  if (a.transitions !=NULL){
     transitions = new map<int, set<transition, transition_compare>* >();

     for (map<int, set<transition, transition_compare>* >::iterator it = a.transitions->begin();
          it!=a.transitions->end();it++){

        for (set<transition, transition_compare>::iterator it2 = it->second->begin();
             it2!=it->second->end(); it2++){

           this->addtransition(it2->state1,  new set<int>(*(it2->variables)),
                               new set<int>(*(it2->notvariables)), it2->state2);
        }
     }
  }

  universe = a.universe;
  names_mapping = a.names_mapping;
  name = a.name;
}

// simple constructor
buchi_automaton::buchi_automaton(){
  initial_state = 0;
  accepting_states = NULL;
  transitions = NULL;
  name = AUTOMATON_DEFAULT_NAME;
}

// complex constructor
buchi_automaton::buchi_automaton(int s_init, set<int>* s_accepting,
                map<int, set<transition, transition_compare>* >* ts){
   initial_state = s_init;
   accepting_states = s_accepting;
   transitions = ts;
}

// --- getters ---
string buchi_automaton::getname() const{
   return name;
}

set<int> buchi_automaton::getuniverse() const{
   return universe;
}

map<int, string> buchi_automaton::getmapping() const{
   return names_mapping;
}

set<transition, transition_compare>* buchi_automaton::gettransitionsfor(int state) const{
   return (*transitions)[state];
}

bool buchi_automaton::isaccepting(int state) const{
   if (accepting_states == NULL || accepting_states->count(state)==0)
      return false;
   else
      return true;
}

int buchi_automaton::getstatescount() const{
   return transitions->size();
}

set<int>* buchi_automaton::getacceptingstates() const{
   return accepting_states;
}

int buchi_automaton::getinitstate() const{
   return initial_state;
}

int buchi_automaton::gettransitionscount() const{
  int result = 0;
  for (map<int, set<transition, transition_compare>* >::iterator it = transitions->begin();
       it!=transitions->end(); it++){
     result+=it->second->size();
  }
  return result;
}

// --- setters ---
transition buchi_automaton::addtransition(int state1, set<int> *variables, set<int> *notvariables,  int state2){
  transition *a = new transition();
  a->state1 = state1;
  a->variables = variables;
  a->notvariables = notvariables;
  a->state2 = state2;

  if (transitions == NULL)
    transitions = new map<int, set<transition, transition_compare>*>();

  if (transitions->count(state1) == 0 || (*transitions)[state1] == NULL){
    (*transitions)[state1] =  new set<transition, transition_compare>();
  }

  (*transitions)[state1]->insert(*a);
  return *a;
}

void buchi_automaton::addtransitions(const buchi_automaton& a, int shift){
  if (a.transitions!=NULL){
    for (map<int, set<transition, transition_compare>* >::iterator it = a.transitions->begin();
         it!=a.transitions->end();it++){

      for (set<transition, transition_compare>::iterator it2 = it->second->begin();
           it2!=it->second->end(); it2++){
        this->addtransition(it2->state1+shift, new set<int>(*(it2->variables)),
                            new set<int>(*(it2->notvariables)), it2->state2+shift);
      }
    }
  }
}

void buchi_automaton::addacceptingstate(int statename){
  if (accepting_states == NULL){
    accepting_states = new set<int>();
  }
  accepting_states->insert(statename);
}

void buchi_automaton::addacceptingstates(const buchi_automaton& a, int shift){
  if (a.accepting_states != NULL){
    for (set<int>::iterator it = a.accepting_states->begin(); it!=a.accepting_states->end(); it++) {
       this->addacceptingstate(*it+shift);
    }
  }
}

void buchi_automaton::setname(string s){
  name = s;
}

int buchi_automaton::getinitialstate(){
  return initial_state;
}

void buchi_automaton::setinitialstate(int i){
  initial_state = i;
}

void buchi_automaton::setuniverse(set<int> u){
  universe = u;
}

void buchi_automaton::setmapping(map<int, string> m){
  names_mapping = m;
}

bool buchi_automaton::hasacceptingstates(){
  if (accepting_states == NULL || accepting_states->size()==0)
    return false;
  else
    return true;
}

// CREATEITMDSTATE function fills the itmd_newstate structure with data from the iterators
itmd_newstate createitmdstate(map<int, set<transition, transition_compare>::iterator> iterators,
                              int recursion_variable){
    itmd_newstate result;
    result.stateset = new set<int>;
    result.variables = new set<int>;
    result.notvariables = new set<int>;

    result.direct_anc = new map<int, set<int>* >;
    result.indirect_anc = NULL;
    result.referenced = false;

    for (map<int, set<transition, transition_compare>::iterator >::iterator it = iterators.begin();
         it!=iterators.end();it++){

       // fill the state set
       result.stateset->insert(it->second->state2);

       // if this transition in original automaton references the recursion variable...
       if (it->second->variables->count(recursion_variable)>0){
          if (result.indirect_anc == NULL){
             result.indirect_anc = new set<int>;
          }

          // ...add the state1 to the indirect_anc set, because it will have indirect successor
          result.indirect_anc->insert(it->second->state1);
          result.referenced = true;
       }

       // now add the state1 as the ancestor for state2:
       if (result.direct_anc->count(it->second->state2)==0){
          (*result.direct_anc)[it->second->state2] = new set<int>;
       }

       (*result.direct_anc)[it->second->state2]->insert(it->second->state1);

       // fill the variables
       insert_iterator<set<int> > new_it(*result.variables, result.variables->begin());
       set_union(result.variables->begin(), result.variables->end(),
                 it->second->variables->begin(), it->second->variables->end(), new_it);

        // fill the notvariables
       insert_iterator<set<int> > new_notit(*result.notvariables, result.notvariables->begin());
       set_union(result.notvariables->begin(), result.notvariables->end(),
                 it->second->notvariables->begin(), it->second->notvariables->end(), new_notit);
    }

    return result;
}

/*  OPERATOR-:  This method computes the complement of given "simple" automaton.

    Note: This methods complements only the automaton with 2 states, with
          one transition under the one-item set of propositions.
          (this is sufficient from the definition of mu-calculus, where the
          negation can stand only in front of the variable)
*/
buchi_automaton buchi_automaton::operator- () const{
   buchi_automaton result;
   result.setuniverse(this->getuniverse());
   result.setmapping(names_mapping);

   result.addtransition(1, new set<int>(), new set<int>(), 1);
   result.addtransition(0, new set<int>(), new set<int>(*this->gettransitionsfor(0)->begin()->variables), 1);

   result.setinitialstate(0);
   result.addacceptingstate(1);

   return result;
}

/*  OPERATOR+:  This method computes the sum of two automata.
    
    Sum operation: The sum of two Buchi automata over E A1=(S1, E, f1, q1, F1) and
    A2=(S2, E, f2, q2, F2) ,  where S1, S2 are disjoint is the automaton 
    A=( union(S1,S2,{q0}), E, union(f1,f2), q0, union(F1,F2) ), where q0 is a new
    (initial) state.

    // This operation can produce unreachable states from the initial states of the 
       automata A1 and A2 

*/
buchi_automaton buchi_automaton::operator+ (const buchi_automaton& a) const{
   buchi_automaton result; // initial state set to 0

   result.setuniverse(a.getuniverse());
   result.setmapping(names_mapping); 

   // add transitions from this automaton, shifted by 1. For initial state q0 of this automaton, for initial state q0' 
   // of resulting automaton: if q0 --a--> q' in this automaton, then q0' --a--> q' in resulting automaton
   result.addtransitions(*this, 1); 
   result.addacceptingstates(*this, 1);
   for (set<transition, transition_compare>::iterator it = \
        this->gettransitionsfor(this->initial_state)->begin(); 
        it!=this->gettransitionsfor(this->initial_state)->end(); it++){

      result.addtransition(result.initial_state, new set<int>(*it->variables),
                           new set<int>(*it->notvariables), it->state2+1);
   }
   // add transitions from a, shifted by the number of states in resulting automaton. For initial state q0 of a, 
   // for initial state q0' of resulting automaton: if q0 --a--> q' in this automaton, 
   // then q0' --a--> q' in resulting automaton
   int shift = result.getstatescount();
   result.addtransitions(a, shift);
   result.addacceptingstates(a, shift);
   for (set<transition, transition_compare>::iterator it = a.gettransitionsfor(a.initial_state)->begin(); 
        it!=a.gettransitionsfor(a.initial_state)->end(); it++)
      result.addtransition(result.initial_state, new set<int>(*it->variables), 
                           new set<int>(*it->notvariables), it->state2+shift);

   return result;
}

/*  OPERATOR*:  This method computes the intersection of two automata.
     
    Paralell composition: The intersection of two Buchi automata over E A1=(S1, E, f1, q1, F1) and
    A2=(S2, E, f2, q2, F2) is the automaton A=( S, E, f, q0, F), where
    S = S1 x S2 x {1,2}
    f: (s1,s2,1) -a-> (s1', s2', 1) <= s1-a->s1' in f1 and s2-a->s2' in f2 and s1 not in F1
       (s1,s2,1) -a-> (s1', s2', 2) <= s1-a->s1' in f1 and s2-a->s2' in f2 and s1 in F1
       (s1,s2,2) -a-> (s1', s2', 2) <= s1-a->s1' in f1 and s2-a->s2' in f2 and s2 not in F2
       (s1,s2,2) -a-> (s1', s2', 1) <= s1-a->s1' in f1 and s2-a->s2' in f2 and s2 in F2
    q0 = (q1, q2, 1)
    F = S1 x F2 x {2}   
*/
buchi_automaton buchi_automaton::operator* (const buchi_automaton& a) {
   buchi_automaton result; // - initial state set to 0

   result.setuniverse(a.getuniverse());
   result.setmapping(names_mapping); 

   map<state_touple, int, state_touple_compare> names;
   map<state_touple,int,state_touple_compare>::iterator name_pair;
   queue<state_touple> fifo;
   
   state_touple init = create_touple(this->initial_state, a.initial_state, 1);
   names[init] = result.initial_state;
   fifo.push(init);
   
   // we will build the states and transitions of a new automaton. 
   while(!fifo.empty()){
   // -- 1. Get next touple from queue and get some information about this touple
      state_touple front = fifo.front();
      fifo.pop();
      int front_name = names[front];    
      bool state1_final = this->isaccepting(front.state1)!=0?true:false;
      bool state2_final = a.isaccepting(front.state2)!=0?true:false;
      
  // -- 2. If it is of the form (s1, s2, 2), where s2 is in F2, s1 in S1, add it to the accepting states
      if (state2_final && front.flag==2){
         result.addacceptingstate(front_name);
      }

  // -- 3. For each transition under var1 in A1 beginning in state1 of the touple and for every transition under var2
  //       in A2 beginning in state2 of the touple, create new transition under union(var1, var2). Then add new states
  //       to the queue. 
       
      for (set<transition, transition_compare>::iterator it1 = (*this->transitions)[front.state1]->begin();
           it1!=(*this->transitions)[front.state1]->end();it1++){

         for (set<transition, transition_compare>::iterator it2 = (*a.transitions)[front.state2]->begin();
            it2!=(*a.transitions)[front.state2]->end();it2++){                  
                        
            state_touple nextstate;
            nextstate.state1 = it1->state2;
            nextstate.state2 = it2->state2;
            nextstate.flag = front.flag;

            if (front.flag == 1 && state1_final){
               nextstate.flag = 2;
            } else if (front.flag == 2 && state2_final){
               nextstate.flag = 1;
            }   
           
            int newstatename;
            if (names.count(nextstate)>0)
               newstatename = names[nextstate];
            else{
               newstatename = names.size();
               names[nextstate] = newstatename;

               fifo.push(nextstate);
            }   

            set<int> *new_variables = new set<int>();
            insert_iterator<set<int> > new_it(*new_variables, new_variables->begin());
            set_union(it1->variables->begin(), it1->variables->end(), 
                      it2->variables->begin(), it2->variables->end(), new_it);             

            set<int> *new_notvariables = new set<int>();
            insert_iterator<set<int> > new_notit(*new_notvariables, new_notvariables->begin());
            set_union(it1->notvariables->begin(), it1->notvariables->end(), 
                      it2->notvariables->begin(), it2->notvariables->end(), new_notit);  
            
	    result.addtransition(front_name,  new_variables, new_notvariables, newstatename);
         }
      }
   }
   return result;
}

/*  GETITMDAUTOMATON:  This method computes the intermediate automaton for this Buchi automaton
     
    Intermediate automaton: The intermediate automaton for Buchi automaton over E A1=(S1, E, f1, q1, F1) 
    is the automaton A=( S, E, f, {q0}, F), where S is the system of subsets of S1, F is {}  and f is defined:
    1.
    {s1, ... ,sm} -a-> (s1', ... , sm') if s1 -a1-> s1', ... , sm -am-> sm' and a=union(a1,...,am) and recursion
    variable X is not in a+.

    2. 
*/
itmd_automaton buchi_automaton::getitmdautomaton (int recursive_variable) {
   itmd_automaton result; // - initial state set to 0
   result.automaton.setuniverse(this->getuniverse());
   result.automaton.setmapping(names_mapping); 

   // this mapping will be used to determine the items of states in the resulting intermediate automaton
   map<int, set<int>*> state_sets;

   // this set will contain all states, we have built yet.
   map<set<int>, int> names;

   // this map  will contain the iterators 
   map<int, set<transition, transition_compare>::iterator > iterators;
   map<int, set<transition, transition_compare>::iterator >::iterator iter;

   queue<int> fifo;
   int front_name;

   set<int> *temp_state = new set<int>();
   temp_state->insert(this->initial_state);

   state_sets[result.automaton.initial_state] = temp_state;
   names[*temp_state] = result.automaton.initial_state;

   fifo.push(result.automaton.initial_state);
   
   // we will build the states and transitions of a new (intermediate) automaton. 
   while(!fifo.empty()){
   // -- 1. Pop the next state from the queue
      front_name = fifo.front();
      set<int> *front = state_sets[front_name];
      fifo.pop();

   // -- 2. Now we will  build the transitions of this stateset based on the transitions of all states of Buchi 
   //       automaton contained in this stateset.

   // ----- 2a. First fill the map with iterators for transition vectors of all states in this stateset

      iterators.clear();
      for (set<int>::iterator st = front->begin(); st!=front->end(); st++) {         
         iterators[*st] = (*this->transitions)[*st]->begin();
      }
   
   // ----- 2b. Calculate all transitions of this stateset
      bool end = false;
      while (!end){
         iter = iterators.begin();
          
   // -------- 2b(i). Proccess this combination of transitions

         // build the new state set based on current combination of transitions:
         itmd_newstate new_state = createitmdstate(iterators, recursive_variable);             
         // if the recursion variable is not referenced, ...
         if (!new_state.referenced){

            // check if the state has already been created. If no add all the neccessary information 
            // push the state to the queue.         
            int newstatename;
            if (names.count(*new_state.stateset)>0)
               newstatename = names[*new_state.stateset];               
            else{
               newstatename = names.size();
               names[*new_state.stateset] = newstatename;
               state_sets[newstatename] = new_state.stateset;

               fifo.push(newstatename);
            }   
            // get the transition added to the transition system ...
	    transition a = result.automaton.addtransition(front_name,  new_state.variables, 
                                                          new_state.notvariables, newstatename); 
            // ... and update the direct_anc mapping; The indirect_anc mapping is not updated in this case.
            result.direct_anc[a] = new_state.direct_anc;
            
         }
         // otherwise (if the recursion variable is referenced): 
         else{
            // remove the recursive variable
            new_state.variables->erase(recursive_variable);

            // for every transition (by the assumption not referencing the recursion variable) from initial 
            // state of Buchi automaton:
            for (set<transition, transition_compare>::iterator init_it = \
                 (*this->transitions)[this->initial_state]->begin();
                 init_it!=(*this->transitions)[this->initial_state]->end(); init_it++){ 
		/* We assume that there is no transition from q0 under a=(a+, a-) such that 
                   recursion variable is in a+
 
                   if (init_it->variables->count(recursive_variable)!=0)
                      continue;
		*/

                // add the state accesible from initial state of Buchi automaton to the stateset
                set<int> *new_stateset = new set<int>(*new_state.stateset);
                new_stateset->insert(init_it->state2);
            
                // add the variables under which it is accesible from initial state (of Buchi automaton)
                set<int> *new_variables = new set<int>();
                insert_iterator<set<int> > new_it(*new_variables, new_variables->begin());
                set_union(new_state.variables->begin(), new_state.variables->end(), 
 		          init_it->variables->begin(), init_it->variables->end(), new_it);

                // add the notvariables under which it is accesible from initial state (of Buchi automaton)
                set<int> *new_notvariables = new set<int>();
                insert_iterator<set<int> > new_notit(*new_notvariables, new_notvariables->begin());
                set_union(new_state.notvariables->begin(), new_state.notvariables->end(), 
 		          init_it->notvariables->begin(), init_it->notvariables->end(), new_notit);
               
                // now do the same as before
                int newstatename;
                if (names.count(*new_stateset)>0)
                   newstatename = names[*new_stateset];               
                else{
                   newstatename = names.size();
                   names[*new_stateset] = newstatename;
                   state_sets[newstatename] = new_stateset;

                   fifo.push(newstatename);
                }   

                // add transition to the intermediate automaton
   	        transition a = result.automaton.addtransition(front_name, new_variables, 
                                                              new_notvariables, newstatename); 
                
                // ... and update the direct_anc and indirect_anc mappings
                result.direct_anc[a] = new_state.direct_anc;

                indirect_data* temp = new indirect_data();
                temp->ind_state = init_it->state2;
                temp->ind_anc = new_state.indirect_anc;

	        result.indirect_anc[a] = temp;
            }
         }


   /* -------- 2b(ii). Calculate next combination of possible transitions.
               Move the iterators forward while they reach the end. If the last iterator has to be 
               moved and it reaches the end after moving, we have computed all transitions of this
               stateset. 
   */        
         bool moved = false;
         while(!moved && !end){
            iter->second++;
            // if this iterator reached the end:
            if (iter->second == (*this->transitions)[iter->first]->end()){
                  int index = iter->first;
                  iter++;
                  iterators[index] = (*this->transitions)[index]->begin();

                  if (iter == iterators.end()){
                     end = true;
                  }
            // if the iterator hasn't reached the end, we can continue computing next transition
            }else{
               moved = true;
            }            
         }
      }
   }
  
   result.state_sets = state_sets;
   result.names = names;
   result.underlaying = this;

   return result;
}

/* GETGFPAUTOMATON: This method computes the greatest fixed point automaton based on this instance 
    (representing the original automaton) and intermediate automaton.   

      WARNING: for testing if the state is accepting it should be sufficient to check if it is the 
      state with second item empty. By doing this, we must not build the automaton for every node S of 
      intermediate automaton (with accepting states (S,emptyset) ), but only one automaton with accepting
      states (S, emptyset) for every node S of intermediate automaton.
*/
gfp_automaton getgfpautomaton(itmd_automaton& a){   
   gfp_automaton result; // - initial state set to 0
   result.automaton.setuniverse(a.underlaying->getuniverse());
   result.automaton.setmapping(a.underlaying->getmapping()); 
 
   map<altstate, int, altstate_compare> names;
   map<int, altstate> state_data;
   queue<altstate> fifo;
   
   // start with the initial state: ({q0}, {q0})
   altstate init;
   init.t1.insert(a.underlaying->getinitstate());
   init.t2.insert(a.underlaying->getinitstate());

   names[init] = result.automaton.getinitstate();
   state_data[result.automaton.getinitstate()] = init;
   fifo.push(init);
   
   while(!fifo.empty()){
   // -- 1. Get next altstate (t1, t1') from the queue
      altstate front = fifo.front();
      fifo.pop();

      int front_name = names[front];
    
   // -- 2. If t1' is empty add this state to accepting states
   // 
   //  WARNING: for testing if this state is accepting it should be sufficient to check if this is the 
   //  state with second item empty. By doing this, we must not build the automaton for every node S of 
   //  intermediate automaton (with accepting states (S,emptyset) ), but only one automaton with accepting
   //  states (S, emptyset) for every node S of intermediate automaton.
   //
   // if (front.t2.empty() && front.t1 == node)
      if (front.t2.empty()){
        result.automaton.addacceptingstate(front_name);
      }
      int t1_name = a.names[front.t1];

   // -- For every transition t1--a-->t2 beginning in state t1 of intermediate automaton ... 
      for (set<transition, transition_compare>::iterator it1 = a.automaton.gettransitionsfor(t1_name)->begin();
           it1!=a.automaton.gettransitionsfor(t1_name)->end();it1++){
   
         // create new state (t2, t2'), where t2' is set of all q2 in t2-F such that q2 is the 
         // direct successor of some q1 in t1'.
         altstate newstate;
         newstate.t1 = *a.state_sets[it1->state2];
         map<int, set<int>* >* direct = a.direct_anc[*it1];


         // So for every state q2 in t2 ... 
         for (set<int>::iterator it2 = newstate.t1.begin(); it2!=newstate.t1.end();it2++){

            // if q2 is not accepting state => q2 is in t2-F ...
            if (a.underlaying->isaccepting(*it2) == 0){
            
               // ... check, if q2 is the direct successor of some q1  ...
               if (direct->count(*it2) > 0 && (*direct)[*it2]!=NULL && !(*direct)[*it2]->empty()){
                 
                  // ... now we have to distinguish 2 cases: when t1' is empty and when it's not.

                  // If t1' is not empty, we have to check, that some of the q1, for which
                  // q2 is the direct successor, is in t1'
                  if ( !front.t2.empty() ){
                     for (set<int>::iterator it3 = (*direct)[*it2]->begin(); 
                          it3!=(*direct)[*it2]->end();it3++){

                         // if there is such a q1 from t1, add q2 to t2' and finish searching.
                         if (front.t2.count(*it3)>0){
                            newstate.t2.insert(*it2);
                            break; 
                         }
                     }                                          

                  // if t1' is empty, add q2 to t2' (we already know that q2 is direct successor of 
                  // some q1 from t1.
                  }else{
                     newstate.t2.insert(*it2);
                  }
               }
            }
         }
            
   //  ---- 2b. Find out the name of the new state or create one and add it to the queue. Add transition to the
   //           automaton
         int newstatename;
         if (names.count(newstate)>0)
            newstatename = names[newstate];
         else{
            newstatename = names.size();

            names[newstate] = newstatename;
            state_data[newstatename] = newstate;

            fifo.push(newstate);
         }

         result.automaton.addtransition(front_name, new set<int>(*it1->variables),
                                 new set<int>(*it1->notvariables), newstatename);
      }
   }

   result.state_data = state_data;
   result.names = names;
   result.underlaying = &a;

   return result;       
}

// GETMAPPINGPAIR: This method computes the value of g(q). 
altstate getmappingpair(itmd_automaton& itmd, transition itmd_trans, altstate f_q){
   altstate result;   
   set<int>* s2 = itmd.state_sets[itmd_trans.state2];

   // for each member q2 of S2 ... 
   for (set<int>::iterator q2 = s2->begin(); q2!=s2->end(); q2++){
      
      set<int>* dir_anc = (*itmd.direct_anc[itmd_trans])[*q2];      
      indirect_data* ind_data = NULL;
       
      // if there is ind_data structure for q2, check if it holds indirect ancestors. If no, set ind_data to NULL.
      // so we can easily check if q2 is indirect successor
      if (itmd.indirect_anc.count(itmd_trans)>0){
         ind_data = itmd.indirect_anc[itmd_trans];
         
         if (ind_data != NULL){
            if (ind_data->ind_state!=*q2 || ind_data->ind_anc == NULL || ind_data->ind_anc->empty())
               ind_data = NULL;
         }
      }

      // ... for every direct ancestor q1 of q2 for this transition ...
      if (dir_anc!=NULL){
         bool added_1 = false;   
         bool added_2 = false;
         for (set<int>::iterator q1 = dir_anc->begin(); q1!=dir_anc->end(); q1++){
            // ...if q1 belongs to t1, add q2 to the t2 and break if added_2
            if (f_q.t1.count(*q1)>0){

               result.t1.insert(*q2);
               if (added_2)
                  break;
               else 
                  added_1 = true;
            }
 
            // ...if q1 belongs to t1', add q2 to the t2' and break if added_1
            if (f_q.t2.count(*q1)>0){
  
               result.t2.insert(*q2);
               if (added_1)
                  break;
               else
                  added_2 = true;         
            }
         }       
      }


      // if q2 is indirect successor in this transition ...
      if (ind_data != NULL){
         // ... for every indirect ancestor q1 of q2 for this transition ...
         for (set<int>::iterator q1 = ind_data->ind_anc->begin(); q1!=ind_data->ind_anc->end(); q1++){
         
            // ...if q1 belongs to t1 or t1', add q2 to the t2' and break
            if (f_q.t1.count(*q1)>0 || f_q.t2.count(*q1)>0){

               result.t2.insert(*q2);
               break;
            }
         }                
      }
   }
     
   return result;
}

// ISCONSISTENT: This function checks the consistency of extended alternating state with given linear ordering
bool isconsistent(extaltstate state, map<int, int> ordering){
   // let state = (S, S*, f). For each q in S...
   for (set<int>::iterator it = state.alt.t1.begin();it!=state.alt.t1.end();it++){
      altstate f_q;
      f_q.t1 = state.f_1[*it];
      f_q.t2 = state.f_2[*it];
          
      // ... if f(q) = (T, T*), for each q* in T* ...
      for (set<int>::iterator it2 = f_q.t2.begin();it2!=f_q.t2.end();it2++){

         // ... check if the condition q*<q is satisfied. If not, return false.
         if (!(ordering[*it2]<ordering[*it])){
            return false;
         }           
      }
   }
   
   // if no inconsistency found, return true
   return true;
}

// GETLFPAUTOMATON: This method computes the least fixed point automaton for given node
lfp_automaton getlfpautomaton(itmd_automaton& a, set<int> node){

   lfp_automaton result; // - initial state set to 0
   result.automaton.setuniverse(a.underlaying->getuniverse());
   result.automaton.setmapping(a.underlaying->getmapping()); 

   map<extaltstate, int, extaltstate_compare> names;
   map<int, extaltstate> state_data;
   queue<extaltstate> fifo;
   
   set<int> potentialy_accepting;
   // start with the initial state: (S, S, f), where f(q)=({q}, emptyset) for every q in S.
   extaltstate init;
   init.alt.t1 = node;
   init.alt.t2 = node;
   for (set<int>::iterator it = node.begin();it!=node.end();it++){
       set<int> temp;
       temp.insert(*it);
       init.f_1[*it] = temp;
   }

   names[init] = result.automaton.getinitstate();
   state_data[result.automaton.getinitstate()] = init;
   fifo.push(init);
   
   while(!fifo.empty()){
   // -- 1. Get next extaltstate (t1, t2, f) from the queue
      extaltstate front = fifo.front();
      fifo.pop();
     
   // -- 2. get the name of this extaltstate and the name of underlying altstate. Check if
   //       we have accepting state
      int front_name = names[front];
      int t1_name = a.names[front.alt.t1];      

 /*   Instead of building each time new automaton, we just build one for the node specified 
      and then for every linear ordering on S we check, which states are accepting. But 
      we can save all states of the form (S,emptyset,f) for some f.
  
        if (front.alt.t2.empty() && front.alt.t1 == node && isconsistent(front, ordering)){
           result.automaton.addacceptingstate(front_name);	 
        }
 */
      if (front.alt.t2.empty() && front.alt.t1 == node){
         potentialy_accepting.insert(front_name);
      }

      // for each transition t1 -> t1' in intermediate automaton ...
      for (set<transition, transition_compare>::iterator it1 = \
           a.automaton.gettransitionsfor(t1_name)->begin();
           it1!=a.automaton.gettransitionsfor(t1_name)->end();it1++){
   
         // create new state (t2, t2'), where t2' is set of all q2 in t2-F such that q2 is the 
         // direct successor of some q1 in t1'.
         extaltstate newstate;
         newstate.alt.t1 = *a.state_sets[it1->state2];
         map<int, set<int>* >* direct = a.direct_anc[*it1];

         // So for every state q2 in t2 ... 
         for (set<int>::iterator it2 = newstate.alt.t1.begin(); it2!=newstate.alt.t1.end();it2++){

            // if q2 is in t2-F ...
            if (a.underlaying->isaccepting(*it2) == 0){
            
               // ... check, if q2 is the direct successor of some q1  ...
               if (direct->count(*it2) > 0 && (*direct)[*it2]!=NULL && !(*direct)[*it2]->empty()){
                 
                  // ... now we have to distinguish 2 cases: when t1' is empty and when it's not.

                  // If t1' is not empty, we have to check, that some of the q1, for which
                  // q2 is the direct successor, is in t1'
                  if ( !front.alt.t2.empty() ){
                     for (set<int>::iterator it3 = (*direct)[*it2]->begin(); 
                          it3!=(*direct)[*it2]->end();it3++){

                         // if there is such a q1 from t1, add q2 to t2' and finish searching.
                         if (front.alt.t2.count(*it3)>0){
                            newstate.alt.t2.insert(*it2);
                            break; 
                         }
                     }                                          

                  // if t1' is empty, add q2 to t2' (we already know that q2 is direct successor of 
                  // some q1 from t1.
                  }else{
                     newstate.alt.t2.insert(*it2);
                  }
               }
            }
         }
         // ---- 3b. for each q in S, if f(q)=(T, T'), then g(q) = (dir(T), Union(dir(T'), ind(T), ind(T')))
         for (set<int>::iterator it_S = node.begin();it_S!=node.end();it_S++){

             // get the value of f(q)
             altstate f_q;
             f_q.t1 = front.f_1[*it_S];
             f_q.t2 = front.f_2[*it_S];

             // and compute the value for g(q):             
             altstate g_q = getmappingpair(a, *it1, f_q);
             newstate.f_1[*it_S] = g_q.t1;
             if (!g_q.t2.empty())
                newstate.f_2[*it_S] = g_q.t2;
             
         }  

         int newstatename;
         if (names.count(newstate)>0)
            newstatename = names[newstate];
         else{
            newstatename = names.size();

            names[newstate] = newstatename;
            state_data[newstatename] = newstate;
 
            fifo.push(newstate);
         }

         result.automaton.addtransition(front_name, new set<int>(*it1->variables), 
                                        new set<int>(*it1->notvariables), newstatename);   
 
      }
   }

   result.names = names;
   result.state_data = state_data;

   if (!potentialy_accepting.empty()){
      // compute all permutations

      map<int, set<int>::iterator > iterators;
      map<int, set<int> > sets;
      map<int, set<int>::iterator >::reverse_iterator st_it;
      set<int> orders;

      // initialize
      int i = 0;
      for (set<int>::iterator it = node.begin(); it!=node.end(); it++){
         set<int> temp;
         sets[*it] = temp;

         orders.insert(i);
         i++;
      }
      
      // set the structures for first linear ordering
      for (set<int>::iterator it = node.begin(); it!=node.end(); it++){          
         sets[*it] = orders;
         iterators[*it] = sets[*it].begin();
         orders.erase(*iterators[*it]);
      }
      
      int first = *node.begin();

      // if we haven't proccessed all linear orderings
      while (iterators[first]!=sets[first].end()){

         // create ordering based on actual positions of iterators
         map<int,int> ordering;
         for (set<int>::iterator it_n = node.begin(); it_n!=node.end(); it_n++) {
            ordering[*it_n]= *iterators[*it_n];
         }

         // check which states are consistent with this ordering and make them accepting
         for (set<int>::iterator it = potentialy_accepting.begin(); it!=potentialy_accepting.end();it++){
            if (isconsistent(state_data[*it], ordering)){
               result.automaton.addacceptingstate(*it);
 

            }
         }
         st_it = iterators.rbegin();
      
         bool complete = false;
         while (!complete){
            st_it->second++;
            if (st_it->second == sets[st_it->first].end()){
               st_it++;          
               if (st_it == iterators.rend())
                  complete = true;
            }else{
               complete = true;
               while(st_it!=iterators.rbegin()){
                  int last = st_it->first;
                  st_it--;
                  sets[st_it->first] = sets[last];
                  sets[st_it->first].erase(*iterators[last]);
                  iterators[st_it->first] = sets[st_it->first].begin();
               }
            }
         }
      }
   }

   return result;       
}

buchi_automaton getlfpautomaton(itmd_automaton& itmd){
   buchi_automaton result = itmd.automaton;
   result.setmapping(itmd.underlaying->getmapping()); 
   result.setuniverse(itmd.underlaying->getuniverse()); 

   for (map<int, set<int>* >::iterator it = itmd.state_sets.begin(); it!=itmd.state_sets.end();it++){
      buchi_automaton temp = getlfpautomaton(itmd, *it->second).automaton;
 
      if (temp.hasacceptingstates()){
         int shift = result.getstatescount();
         result.addtransitions(temp, shift);
         result.addacceptingstates(temp, shift);

         set<transition, transition_compare>* trs = temp.gettransitionsfor(temp.getinitialstate());
         for (set<transition, transition_compare>::iterator it2 =trs->begin(); it2!=trs->end(); it2++) {
            result.addtransition(it->first, new set<int>(*it2->variables),
                                 new set<int>(*it2->notvariables), it2->state2 + shift);
         }
      }
   }

   return result;
}


/*  
   OPERATOR<<:  This method writes the textual representation of Buchi automaton:
*/
ostream& operator<<(ostream& out, buchi_automaton& automaton){
   // write the header of the specification
   out << "process " << automaton.getname() << " {\n" ;

   // if this automaton has some states, declare their names:
   if (automaton.getstatescount()>0){
      out << "state ";
      for (int i=0;i<automaton.getstatescount();i++){
         out << AUTOMATON_STATE_PREFIX << i;
         if (i!=automaton.getstatescount()-1)
            out << ",";
      }
      out << ";\n";

   // write the initial state name
      out << "init " << AUTOMATON_STATE_PREFIX << "0;\n";

   // if this automaton has accepting states, write their names here
      if (automaton.accepting_states!=NULL && automaton.accepting_states->size()>0){
      	 out << "accept ";
                     
	 unsigned int counter = 0;
   	 for (set<int>::iterator it=automaton.accepting_states->begin();
              it!=automaton.accepting_states->end();  it++){
      	    out << AUTOMATON_STATE_PREFIX << (*it);
      	    if (counter!=automaton.accepting_states->size()-1)
               out << ",";

            counter++;
         }
         out << ";\n";
      }

   // if this automaton has some transitions, write them 
      if (automaton.transitions!=NULL && automaton.transitions->size()>0){
      	 out << "trans\n";
         for (map<int, set<transition, transition_compare>*>::iterator it=automaton.transitions->begin();
              it!=automaton.transitions->end(); it++){
              
            for (set<transition, transition_compare>::iterator itt=(it->second)->begin();
                 itt!=(it->second)->end(); itt++){

                  if (it != automaton.transitions->begin() || itt!=(it->second)->begin())
                     out << ",\n";               
               
                  out << " " << AUTOMATON_STATE_PREFIX << itt->state1 << " -> " << AUTOMATON_STATE_PREFIX << itt->state2;
                  out << " {";
		  
                  if (itt->variables->size()>0 || itt->notvariables->size()>0){
                     out << "guard ";   
                     for (set<int>::iterator it2=itt->variables->begin();
                          it2!=itt->variables->end(); it2++){
                         if (it2 != itt->variables->begin())
                            out << " and ";
                         out << automaton.names_mapping[*it2];
                     } 
                     for (set<int>::iterator it2=itt->notvariables->begin();
                          it2!=itt->notvariables->end(); it2++){
                         if (it2 != itt->notvariables->begin() || itt->variables->size()>0)
                            out << " and " ;
                         out << "(not "<<automaton.names_mapping[*it2] << ")";
                     }
		     out << ";";
                  }
                  out << "}";                              
            }
         }
      }
      out << ";\n";
   }
   
   out << "}\n";

   return out;
}

