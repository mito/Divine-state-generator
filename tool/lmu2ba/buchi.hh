/* This file was created by Tomas Laurincik in 2007 as part 
   of bachelor thesis
   
   xlaurin1@fi.muni.cz
*/
#ifndef BUCHI_H
#define BUCHI_H

#define DEBUG_LEVEL 0

#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <algorithm>
#include <iterator>

using namespace std;

/* ============================================================
                           declarations 
   ============================================================*/
#define AUTOMATON_STATE_PREFIX "q"
#define AUTOMATON_DEFAULT_NAME "mu_calc_property"

struct state_touple;
struct transition;
class state_touple_compare;
class buchi_automaton;
struct altstate;
struct itmd_automaton;
struct gfp_automaton;
struct lfp_automaton;

// compute the least fixed point automaton for given node
lfp_automaton getlfpautomaton(itmd_automaton& a, set<int> node);
// compute the least fixed point automaton
buchi_automaton getlfpautomaton(itmd_automaton& itmd);
// compute the greatest fixed point automaton
gfp_automaton getgfpautomaton(itmd_automaton& a);

// -----------------------------------------------------------
// STATE_TOUPLE: This structure will be used to compute the transitions of paralell composition 
// of two buchi automata.
struct state_touple{
   int state1, state2, flag;
};

// STATE_TOUPLE_COMPARE: This functor is used to compare two state_touples.
class state_touple_compare{
  public:
     bool operator()(const state_touple& a,const state_touple& b) const{
        if (a.state1<b.state1)
	   return true;
        else if (a.state1==b.state1){
           if (a.state2<b.state2)
              return true;
           else if (a.state2==b.state2)
              return a.flag<b.flag;
           else
              return false;
        }else
           return false;
     }
};

// TRANSITION: This structure represents the item of transition relation of Buchi automaton
struct transition{
   int state1, state2;
   set<int> *variables;
   set<int> *notvariables;
};

// TRANSITION_COMPARE: This functor is used to compare two transitions.
class transition_compare{
  public:
     bool operator()(const transition& a,const transition& b) const{
        if (a.state1<b.state1)
	   return true;
        else if (a.state1==b.state1){
           if (a.state2<b.state2)
              return true;
           else if (a.state2==b.state2){
	      if (*(a.variables)<*(b.variables))
                 return true;
	      else if (*(a.variables)==*(b.variables))
                 return *(a.notvariables)<*(b.notvariables);
	      else
                 return false;
           }else
              return false;
        }else
           return false;
     }
};

// ALTSTATE: This structure represents the state of alternating automaton used to create 
// automaton for greatest fixed point. 
struct altstate{
   set<int> t1, t2;
};

// ALTSTATE_COMPARE: This functor is used to compare two alternating states.
class altstate_compare{
  public:
     bool operator()(const altstate& a,const altstate& b) const{
        if (a.t1<b.t1)
	   return true;
        else if (a.t1==b.t1){
           return a.t2<b.t2;
        }else
           return false;
     }
};

// EXTALTSTATE: This structure represents the extended state of alternating automaton used to create 
// automaton for least fixed point automaton. The extension is map from set of states of base automaton
// to set of altstates.
struct extaltstate{
   altstate alt;
   map<int, set<int> > f_1;
   map<int, set<int> > f_2;
};


// EXTALTSTATE_COMPARE: This functor is used to compare two extended alternating states.
class extaltstate_compare{
  public:
     bool operator()(const extaltstate& a,const extaltstate& b) const{
        if (a.f_1 < b.f_1){
	   return true;
        }else if (a.f_1 == b.f_1) {
           if (a.f_2 < b.f_2){
              return true;
           }else if (a.f_2 == b.f_2 ){
              altstate_compare temp;
              return temp(a.alt, b.alt);
           }else{
              return false;
           }
        }else
           return false;
     }
};

/* =====================================================================
                        BUCHI_AUTOMATON class 
   ===================================================================== 
   This class represents Buchi automaton with single initial state, which
   transitions are under a set of atoms, i.e. the items a=(a+, a-) where
   a+ and a- are subsets of some set of variables. 
   This class implements this operators:
   : "-" for computing complementation of simple(!) automaton 
   : "+" for computing disjoint sum of two automatons
   : "*" for computing parallel composition of two automatons
   and has methods for computing intermediate automaton for given 
   recursive variable.
*/ 
class buchi_automaton {
   private: 
 // --- instance fields --- 
     // parts of transition system:
     int initial_state;
     set<int>* accepting_states;

     /* transitions: the value for a key in this map is a vector of 
        transitions state1 -a->state2, in which the state1 is equal to the key.  */
     map<int, set<transition, transition_compare>* > *transitions;     

     // additional information about this automaton:
     string name;
     set<int> universe;
     map<int, string> names_mapping;      
   public:

 // --- constructors ---
     // copying constructor
     buchi_automaton(const buchi_automaton& a);
     // simple constructor
     buchi_automaton();
     // complex constructor
     buchi_automaton(int s_init, set<int>* s_accepting, 
                     map<int, set<transition, transition_compare>* >* ts);

  // --- getters ---     
     string getname() const;     
     set<int> getuniverse() const;
     map<int, string> getmapping() const;
     set<transition, transition_compare>* gettransitionsfor(int state) const;
     int getstatescount() const;
     int gettransitionscount() const;
     int getinitstate() const;
     set<int>* getacceptingstates() const;
     bool isaccepting(int state) const;

  // --- setters ---     
     transition addtransition(int state1, set<int> *variables, set<int> *notvariables,  int state2);
     void addtransitions(const buchi_automaton& a, int shift);
     void addacceptingstate(int statename);
     void addacceptingstates(const buchi_automaton& a, int shift);
     void setname(string s);
     int getinitialstate();
     void setinitialstate(int i);
     void setuniverse(set<int> u);
     void setmapping(map<int, string> m);
     bool hasacceptingstates();

  // --- operations --- 
     // printing textual representation of this automaton
     friend ostream& operator<< (ostream&, buchi_automaton&);
     // compute the (simple) complement of the automaton
     buchi_automaton operator- () const;
     // compute the sum of the automata
     buchi_automaton operator+ (const buchi_automaton&) const;
     // compute the parallel composition of the automata
     buchi_automaton operator* (const buchi_automaton&) ;
     // compute the intermediate automaton
     itmd_automaton getitmdautomaton (int recursive_variable); 
     
};

struct indirect_data {
   set<int>* ind_anc;
   int ind_state;
};

// ITMD_AUTOMATON structure will represent the intermediate automaton. This structure contains a buchi_automaton
// instance with no accepting states, a mapping from the state names to sets of states - the states of 
// intermediate automaton and the reverse mapping
// 
//  
struct itmd_automaton {
    // these two maps does the conversion from state name to state semantics and back
    map<int, set<int>* > state_sets;
    map<set<int>, int> names;

    // these two maps represent the INVERSE succesor relation for each transition in the itmd automaton
    map<transition, map<int, set<int>* >*, transition_compare > direct_anc;    
    map<transition, indirect_data* , transition_compare> indirect_anc;

    // this automaton represents the transition system of this intermediate automaton
    buchi_automaton automaton;    

    // this pointer holds the address of the original automaton, from which this intermediate automaton has
    // been built
    buchi_automaton* underlaying;
};

// GFP_AUTOMATON structure will represent the greatest fixed point automaton. This structure contains a buchi_automaton
// instance, a mapping from the state names to altstates - the states of 
// greatest fixed point automaton and the reverse mapping
struct gfp_automaton {
    map<int, altstate> state_data;
    map<altstate, int, altstate_compare> names;

    buchi_automaton automaton;
    itmd_automaton* underlaying;
};

// LFP_AUTOMATON structure will represent the least fixed point automaton. This structure contains a buchi_automaton
// that represents the transition system, the greatest fixed point automaton from which it was built and 
// the map from normalized state names to state semantics and the reverse map.
struct lfp_automaton {
    map<int, extaltstate> state_data;
    map<extaltstate, int, extaltstate_compare> names;

    buchi_automaton automaton;
    gfp_automaton* underlaying;
};

// ITMD_NEWSTATE structure represents the new state created, the variables under which the transition can be
// made to this state and whether the recursion variable is referenced.
struct itmd_newstate {
    set<int> *stateset;
    set<int> *variables;
    set<int> *notvariables;
    
    // this map represents the INVERSE (direct) succesor relation for this new transition 
    map<int, set<int>* >* direct_anc;    
   
    // this set contains the names of the states, that will have indirect successor in this transition
    set<int>* indirect_anc;

    // if the recursion variable is referenced
    bool referenced;
};

#endif
