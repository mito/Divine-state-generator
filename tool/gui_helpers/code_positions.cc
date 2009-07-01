#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <cctype>
#include <sevine.h>


using namespace divine;
using namespace std;

//!Print a brief help to stdout
void print_usage()
{
  cout <<"--------------------------------------------------------------"<<endl;
  cout <<"DiVinE Tool Set"<<endl;
  cout <<"--------------------------------------------------------------"<<endl;
  cout <<"code_positions - auxiliary tool for GUI - computes current positions"
                                                                            "\n"
         "                 in a source code in a given state" << endl;
  cout <<"--------------------------------------------------------------"<<endl;
  
  cout << "Usage: divine.code_positions trail model_source output_file\n" << endl;
}

void trivial_syntax_error(string & trans_str)
{
 gerr << "Syntax error: " << trans_str << " is not a"
      << " string representation of system (enabled) transition" << thr();
}

//!Convert string `str' to (unsigned) integer value
size_int_t str_to_int(string & str)
{
 istringstream str_stream(str);
 size_int_t number;
 str_stream >> number;
 return number;
}

//!Function for parsing of a string representing system transition
void parse_system_trans_str(string & trans_str,
                            array_t<size_int_t> & processes,
                            array_t<size_int_t> & transitions)
{
 enum expect_t { exp_digit, exp_digit_or_dot, exp_digit_or_and };
 bool last_was_dot = false;
 expect_t expect = exp_digit;
 string number = "";
 
 processes.clear();
 transitions.clear();
 if (trans_str.size() < 5 ||
     trans_str[0]!='<' || trans_str[trans_str.size()-1]!='>')
   trivial_syntax_error(trans_str);
 size_int_t i = 1;
 size_int_t bound = trans_str.size() - 1;
 while (i<bound)
  {
   if (isdigit(trans_str[i]))
    {
     number.push_back(trans_str[i]);
     if (last_was_dot) expect = exp_digit_or_and;
     else expect = exp_digit_or_dot;
    }
   else if (trans_str[i]=='.')
    {
     if (expect==exp_digit_or_dot)
      {
       processes.push_back(str_to_int(number));
       number.clear();
       expect = exp_digit;
       last_was_dot = true;
      }
     else trivial_syntax_error(trans_str);
    }
   else if (trans_str[i]=='&')
    {
     if (expect==exp_digit_or_and)
      {
       transitions.push_back(str_to_int(number));
       number.clear();
       expect = exp_digit;
       last_was_dot = false;
      }
     else trivial_syntax_error(trans_str);
    }
   else trivial_syntax_error(trans_str);
   
   ++i;
  }
 if (number.size()==0 || last_was_dot==false) trivial_syntax_error(trans_str);
 else transitions.push_back(str_to_int(number));

 if (processes.size()!=transitions.size()) trivial_syntax_error(trans_str);
}

int main(int argc, char ** argv)
{
 system_t * p_System = 0;
 try
  {
   /* test for a correct numebr of arguments on a command line */
   if (argc!=4) { print_usage(); return 1; }
   

   /* BEGIN of decisions about the type of an input */
   char * filename = argv[argc-2];
   int filename_length = strlen(filename);
   if (filename_length>=2 && strcmp(filename+filename_length-2,".b")==0)
    {
     gerr << "Sorry, Promela not yet supported." << thr();
    }
   else
    {
     cout << "Reading DVE source..." << endl;
     p_System = new dve_explicit_system_t(gerr);
    }
    
   if (p_System->read(filename))
     gerr << "Error: Reading of " << filename << " was unsuccessful." << thr();
   /* END of decisions about the type of an input */
   
   
   //Reading and parsing of a trail
   ifstream trail;
   string trans_str;
   array_t<size_int_t> processes(p_System->get_process_count()),
                       transitions(p_System->get_trans_count());

   ofstream output;

   trail.open(argv[argc-3]);
   if (trail) trail >> trans_str;
   else gerr << "Error: It is not possible to open " << argv[argc-3]
             << " for reading" << thr();
   
   output.open(argv[argc-1]);
   if (!output) gerr << "Error: It is not possible to open " << argv[argc-1]
                     << " for writting" << thr();
   
   process_t * process;
   transition_t * trans;
   dve_transition_t * dve_trans;
   size_int_t fline=0, fcol=0, lline=0, lcol=0;
             
   //while not reaching the end of the trail file...
   while (trail)
    {
     if (trans_str != "================") //divider of a cycle
      {
       parse_system_trans_str(trans_str,processes,transitions);
       for (size_int_t i=0; i!=processes.size(); ++i)
        {
         //cout << processes[i] << "." << transitions[i] << " ";
         if (p_System->get_process_count()>processes[i])
          {
           process = p_System->get_process(processes[i]);
           if (process->get_trans_count()>transitions[i])
            {
             trans = process->get_transition(transitions[i]);
             dve_trans = dynamic_cast<dve_transition_t*>(trans);
             if (dve_trans)
              {
               dve_trans->get_source_pos(fline,fcol,lline,lcol);
               output << fline;
               if (i!=processes.size()-1) output << " ";
               else output << endl;
              }
             else gerr << "Unexpected error: Casting to dve_transition_t * "
                          "not succeeded." << thr();
            }
           else gerr << "Error: " << transitions[i] << " is not a valid LID"
                     << " of transition in a process with GID " << processes[i]
                     << thr();
          }
         else if (processes[i]==p_System->get_process_count()) //prop. process
            output << endl; //at least EOLN
         else gerr << "Error: " << processes[i]
                   << " is not a valid GID of process" << thr();
        }
      }
     else output << "CYCLE" << endl;
     trail >> trans_str;
    }
   output.close();
  }
 catch (ERR_throw_t & err_type)
  {
   cerr << "Program finishes." << endl;
   return err_type.type;
  }
 
}

