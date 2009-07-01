#ifndef _BIT_HASH_HH_
#define _BIT_HASH_HH_

#include "sevine.h"

using namespace std;
using namespace divine;

class bit_hash_t {
protected:
  size_t ht_size;
  char* start_ptr;
  int collisions;
  int hf_id;

  size_t hash_function(state_t);
  
public:
  bit_hash_t(size_t s);
  ~bit_hash_t() { delete start_ptr; };

  bool check(state_t);
  int get_collision_count() {return collisions;};

  void print();
};


#endif
