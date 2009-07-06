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

        state = sys->get_initial_state();
        for (int i = 0; i < state.size; i++) {
            cout << (*((int *)(&((byte_t *)(state.ptr))[i]))) << ',';
        }
        cout << endl;

       
    } else {
        state = sys->get_initial_state();
        cout << argv[2] << endl;;
        cout << strlen(argv[2]) << endl; 

        int size = 0;

        for (int i = 0; i < strlen(argv[2]); i++) {
            if (argv[2][i] == ',') {
                size++;
            } 
        }

        string str;
        cout << size << endl;
        char data[size]; 
        int pos = 0;
        int value;
        for (int i = 0; i < strlen(argv[2]); i++) {
            if (argv[2][i] == ',') {
                cout << str << endl;    
                value = atoi(str.c_str());
                (*((int *)(&((byte_t *)(data))[pos]))) = value;
                pos++;
                str = "";
            } else {
                str.append(1, argv[2][i]);
            }
        }

        for (int i = 0; i < state.size; i++) {
            cout << (*((int *)(&((byte_t *)(state.ptr))[i]))) << ',';
        }
        cout << endl;
        for (int i = 0; i < state.size; i++) {
            cout << (*((int *)(&((byte_t *)(data))[i]))) << ',';
        }
        cout << endl;

        //for (int i; i < strlen(state.ptr); i++) {
            //if (data[i] != state.ptr[i]) {
                //cout << "Error!";
            //} else {
                //cout << "ok!";
            //}
        //}
        //for (int i = 0; i < sizeof(sshort_int_t); i++) {
            //(*((sshort_int_t *)(&((byte_t *)(data))[i]))) = sshort_int_t[i]; 
        //}

        //state = new_state(data, sizeof(data));
        //int succs_result;
        //succ_container_t succs(*sys);
        //succs_result = sys->get_succs(state,succs);	

        //for (int i = 0; i < state.size; i++) {
            //cout << (*((sshort_int_t *)(&((byte_t *)(state.ptr))[i]))) << ',';
        //}
        //cout << "result: " << succs_result;
        //for (std::size_t info_index=0; info_index!=succs.size(); info_index++) {
            ////sys.DBG_print_state(succs[info_index],cout,16);
            //cout << endl <<"1st successors" << endl;
            //for (int i = 0; i < state.size; i++) {
                //cout << (*((sshort_int_t *)(&((byte_t *)(succs[info_index].ptr))[i]))) << ',';
            //}
            //cout << endl;
            //cout << *(int *)succs[info_index].ptr << endl;
            //print_state(succs[info_index], 0);
            //cout << endl;
        //}
    }

      
    return 0;
}
