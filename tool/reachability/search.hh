
#ifndef _SEARCH_HH_
#define _SEARCH_HH_

#include <map>
#include <string>
#include <vector>
#include "reachability_observer.hh"
#include "parameters.hh"
#include "sevine.h"
#include "bit_hash.hh"

class search_t {
public:
  search_t(explicit_system_t* s, parameters_t* p, reachability_observer_t* o) { system = s; parameters = p; observer = o;}
  virtual void do_search() =0;
  virtual ~search_t() {}

protected:
  explicit_system_t* system;
  parameters_t* parameters;
  reachability_observer_t* observer;  
};

// Full classical search
class full_search_t : public search_t {
public:
  full_search_t(explicit_system_t* s, parameters_t* p, reachability_observer_t* o);
  full_search_t(explicit_system_t* s, parameters_t* p, reachability_observer_t* o, string out);
  void do_search();
private:
  ofstream output;
  explicit_storage_t visited;
  path_t* reconstruct_path(state_ref_t&);
  state_t init;
};

// Random Walk search
class rw_search_t : public search_t {
public:
  rw_search_t(explicit_system_t* s, parameters_t* p, reachability_observer_t* o);
  void do_search();
};
// specific parameters: x = depth limit


// Partial Order search --- not implemented yet
class por_search_t : public search_t {
public:
  por_search_t(explicit_system_t* s, parameters_t* p, reachability_observer_t* o);
  void do_search();
private:
  explicit_storage_t visited;
};

// Full search with matching on abstract states
class alpha_search_t : public search_t {
public:
  alpha_search_t(explicit_system_t* s, parameters_t* p, reachability_observer_t* o);
  void do_search();
private:
  bool initialize();
  string abstraction(state_t);
  
  map<string, bool> visited;
  vector<expression_t*> predicates;
};

// Full search with bitstate hashing
class bithash_search_t : public search_t {
public:
  bithash_search_t(explicit_system_t* s, parameters_t* p, reachability_observer_t* o);
  void do_search();
private:
  void initialize();
  bit_hash_t* visited;
};

// Reduced InterLeaving search
//class ril_search_t : public search_t {
//public:
//  ril_search_t(explicit_system_t* s, parameters_t* p, reachability_observer_t* o);
//  void do_search();
//  
//private:
//  explicit_storage_t visited;
//  vector< vector<bool> > free;
//
//  bool initialize();
//  void write_free();
//  int select_enabled_transitions(state_t q, enabled_trans_container_t & result_trans);  
//};
//

#endif
