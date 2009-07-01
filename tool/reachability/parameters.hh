
#ifndef _PARAMETERS_HH_
#define _PARAMETERS_HH_

enum order_t { BFS = 0, DFS = 1 };

struct parameters_t {

  bool paths, first_goal;
  int htsize;
  int x, y, z;
  string special_file;
  order_t order;
  
  parameters_t() {
    paths  = first_goal = false;
    htsize = 65536;
    x = y = z = 0;
    order = BFS;
    special_file = "";
  }   
};

#endif
