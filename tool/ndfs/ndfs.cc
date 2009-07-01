#include <cstdio>
#include <iostream>
#include <unistd.h>
#include <cstring>
#include "ndfs.hh"

using namespace std;
using namespace divine;

// XXX segfault in MacOS X 10.4
// static vminfo_t vm;

// float get_consumed_memory()
// { return float(double(vm.getvmsize())/1024); }

int
main(int argc, char** argv) {  
    char c;
    bool print_help = false, print_leading_to_error = false;
    bool compiled_generator = false;
    unsigned long hash_table = 65536;
    explicit_storage_t st;
  
    while ((c = getopt(argc, argv, "hpH:E")) != -1) {
        switch (c) {
        case 'h': print_help = true; break;
        case 'H': hash_table = atoi(optarg); break;
        case 'p': compiled_generator = true; break;
        case 'E': print_leading_to_error = true; break;
        }
    }

    if (print_help) {
        cout << "Usage:\n"
            "divine.ndfs [-hE] [-H size] input_file\n"
            "\n"
            "Perform Nested-DFS LTL model checking.\n"
            "\n"
            "Options:\n"
            "-h   this help\n"
            "-p,  precompile DVE prior verification\n"
            "-q   don't print accepting cycle\n"
            "-H x hash table size to ( x<33 ? 2^x : x )\n"
            ;
        return 0;
    }

    if (argc < optind+1) { 
        cerr << "No input file given!" << endl;
        return 1;
    }

    try {
        explicit_system_t* p_System;
	system_description_t system_desc;
	p_System = system_desc.open_system_from_file(argv[optind],
						     compiled_generator,
						     true
						     );

        explicit_system_t& System = *p_System;
        int file_opening = System.read(argv[optind]);
        if (file_opening) return file_opening;
      
        //Advanced setting of hash table size:
        if (hash_table > 0) {
            if (hash_table < 33) { //sizes 1..32 interpret as 2^1..2^32
                hash_table = 1 << hash_table;
            }
            st.set_ht_size (hash_table);
        }

        st.set_appendix_size (sizeof (short)); //XXX
        st.init ();
        ndfs_t search (System, st);
     
        state_t init = System.get_initial_state();
        if (search.find_accepting_cycle (init)) {
            deque<state_ref_t> cycle = search.get_accepting_cycle();
            cout << "Found accepting cycle of length " << cycle.size() << endl;
            if (print_leading_to_error) {
                for (deque<state_ref_t>::iterator i = cycle.begin(); i != cycle.end(); ++i) {
                    state_t state = st.reconstruct(*i);
                    System.print_state (state);
                    delete_state (state);
                }
            }
         
        } else {
            cout << "No accepting cycle found." << endl;
        }
        delete_state(init);
        delete p_System;
    } catch  (ERR_throw_t & err_type) {
        return err_type.id;
    }
  
    return 0;
}
