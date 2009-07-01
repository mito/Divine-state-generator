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
  cout <<"reformat_ce_states - auxiliary tool for GUI - reformats states\n"
         "                     in counterexample to be readable for GUI\n"
         "                     client" << endl;
  cout <<"--------------------------------------------------------------"<<endl;
  
  cout << "Usage: divine.reformat_ce_states ce_states output_file\n" << endl;
}

void trivial_syntax_error(string & line)
{
 gerr << "Syntax error: " << line << " has not an"
      << " expected format" << thr();
}

int main(int argc, char ** argv)
{
 try
  {
   /* test for a correct numebr of arguments on a command line */
   if (argc!=3) { print_usage(); return 1; }
   

   //Reading and parsing of a counterexample states
   ifstream input;
   string line;

   ofstream output;

   input.open(argv[argc-2]);
   if (input) getline(input,line);
   else gerr << "Error: It is not possible to open " << argv[argc-3]
             << " for reading" << thr();
   
   output.open(argv[argc-1]);
   if (!output) gerr << "Error: It is not possible to open " << argv[argc-1]
                     << " for writting" << thr();
   
   //while not reaching the end of the counterexample states file...
   string prefix="";
   string variable_value="";
   bool first_in_line = true;
   while (input)
    {
     if (line == "") { output << endl; first_in_line = true; }
     else if (line != "================") //divider of a cycle
      {
       if (first_in_line) first_in_line = false;
       else output << ",";
       size_int_t i=0;
       if (line[0]=='[') prefix = "";
       else
        {
         while (i<line.size() && line[i]!=':') i++;
         if (i>=line.size()) trivial_syntax_error(line);
         prefix = line.substr(0,i);
         ++i;
         if (i>=line.size()) trivial_syntax_error(line);
        }
       if (line[i]!='[' || i>=line.size()) trivial_syntax_error(line);
       i++;
       size_int_t start = i;
       while (line[start]!=']')
        {
         while (i<line.size() && line[i]!=',' && line[i]!=']') i++;
         if (i>=line.size()) trivial_syntax_error(line);
         variable_value = line.substr(start,i-start);
         if (prefix.size())
          {
           std::size_t pos = variable_value.find(':',0);
           if (pos == string::npos) output << prefix << ":";
           else output << prefix << ".";
          }
         output << variable_value;
         if (line[i]==',')
          {
           output << ",";
           i++;
           if (i>=line.size()) trivial_syntax_error(line);
           while (i<line.size() && line[i]==' ') ++i;
           if (i>=line.size()) trivial_syntax_error(line);
          }
         start = i;
        }
      }
     else output << "CYCLE";
     getline(input,line);
    }
   output.close();
  }
 catch (ERR_throw_t & err_type)
  {
   cerr << "Program finishes." << endl;
   return err_type.type;
  }
}

