#include <getopt.h>
#include <iostream>
#include <queue>
#include <sstream>
#include <unistd.h>

#include "sevine.h"

#define ES_FMT_DIVIDE_PROCESSES_BY_BACKSLASH_N 16

using namespace std;
using namespace divine;

explicit_system_t *sys;

void print_state(state_t state,int format)
{
      sys->print_state(state, cout);

      ostringstream mystream;
      dynamic_cast<dve_explicit_system_t*>(sys)->DBG_print_state(state,mystream,format);
      string str = mystream.str();
      cout << str;

      return;
}


int main(int argc, char **argv)
{
  
  bool recognized = false;
  int file_opening=0;
  
  char *filename = argv[1];
  int filename_length = strlen(filename);

  if (filename_length>=4 && strcmp(filename+filename_length-4,".dve")==0)
    {
      sys = new dve_explicit_system_t(gerr);
      file_opening = sys->read(argv[optind]);
      recognized = true;
    }

  if (!recognized)
    {
      cerr << "File type not recognized. Supported extensionl is .dve" << endl;
      return 1;
    }


  if (file_opening)
    {
      cerr << "Filename " << argv[1] << " does not exist." << endl;
      return 1;
    }

    state_t state;
    if (argc == 2) {
        //state = sys->get_initial_state();
        char data[7];
        (*((sshort_int_t *)(&((byte_t *)(data))[0]))) = 1;
        (*((sshort_int_t *)(&((byte_t *)(data))[1]))) = 0;
        (*((sshort_int_t *)(&((byte_t *)(data))[2]))) = 0;
        (*((sshort_int_t *)(&((byte_t *)(data))[3]))) = 0;
        (*((sshort_int_t *)(&((byte_t *)(data))[4]))) = 0;
        (*((sshort_int_t *)(&((byte_t *)(data))[5]))) = 0;
        (*((sshort_int_t *)(&((byte_t *)(data))[6]))) = 0;
        (*((sshort_int_t *)(&((byte_t *)(data))[7]))) = -18688; 

        state = new_state (data, 7);
        for (int i = 0; i < state.size; i++) {
            cout << (*((sshort_int_t *)(&((byte_t *)(state.ptr))[i]))) << ',';
        }
        cout << endl;
        cout << "size: " << state.size << endl;
        cout <<*(int *) state.ptr << endl;
        print_state(state, 0);
        cout << endl;

        int succs_result;
        succ_container_t succs(*sys);
        succs_result = sys->get_succs(state,succs);	

        for (std::size_t info_index=0; info_index!=succs.size(); info_index++) {
            //sys.DBG_print_state(succs[info_index],cout,16);
            cout << endl <<"1st successors" << endl;
            for (int i = 0; i < state.size; i++) {
                cout << (*((sshort_int_t *)(&((byte_t *)(succs[info_index].ptr))[i]))) << ',';
            }
            cout << endl;
            cout << *(int *)succs[info_index].ptr << endl;
            print_state(succs[info_index], 0);
            cout << endl;

            int succs_result_1;
            succ_container_t succs_1(*sys);
            succs_result_1 = sys->get_succs(succs[info_index],succs_1);	

            for (std::size_t i=0; i!=succs_1.size(); i++) {
                cout << endl <<"2nd successors" << endl;
                for (int j = 0; j < state.size; j++) {
                    cout << (*((sshort_int_t *)(&((byte_t *)(succs_1[i].ptr))[j]))) << ',';
                }
                cout << endl;
                cout << *(int *)succs_1[i].ptr << endl;
                print_state(succs_1[i], 0);
                cout << endl;
            }
        }
       
    } else {
        char statevalue[] = "1000011";         
        state = new_state(&statevalue[0], sizeof(statevalue));
        print_state(state, 0);

        //int succs_result;
        //succ_container_t succs(*sys);
        //succs_result = sys->get_succs(state,succs);	

        //for (std::size_t info_index=0; info_index!=succs.size(); info_index++) {
            //print_state(state,0);
            //cout <<"\"->\"";

            ////sys.DBG_print_state(succs[info_index],cout,16);
            //print_state(succs[info_index],0);
            //cout <<"\"";
        //}
    }

      
    return 0;
}
