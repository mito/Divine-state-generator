#include <iostream>
#include "sevine.h"

using namespace std;
using namespace divine;

dve_system_t System(gerr);
dve_symbol_table_t* table;

void draw_process(process_t* proc) {
  cout << "digraph "<< table->get_process(proc->get_gid())->get_name()<<" {"<<endl;

  for(size_t i=0; i<proc->get_trans_count();i++) {
    transition_t* tr = proc->get_transition(i);
    dve_transition_t *dve_tr = dynamic_cast<dve_transition_t*>(tr);
    
    cout << table->get_state(dve_tr->get_state1_gid())->get_name()<< " -> "<<
      table->get_state(dve_tr->get_state2_gid())->get_name()<<endl;
  }
  
  cout <<"}"<<endl<<endl;
}

int main(int argc, char** argv) {

  System.read(argv[1]);
  table = System.get_symbol_table();
  for(size_t i=0; i<System.get_process_count(); i++) {
    draw_process(System.get_process(i));
  }  
  
  return 0;
}

