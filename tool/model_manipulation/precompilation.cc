#include <sevine.h>
#include <getopt.h>
#include <sstream>
#include <fstream>
#include <string>
#include <math.h>
#include <getopt.h>


using namespace divine;
using namespace std;


static void print_usage(const char * const program_name);

static struct option longopts[] = {
  { "help",          no_argument, 0, 'h'},
  { NULL, 0, NULL, 0 }
};


static void parse_command_line(int argc, char ** argv)
{
  if (argc<2) 
    {
      print_usage(argv[0]);
      exit(0);
    }
 char c;
 while ((c = getopt_long(argc, argv, "ho:", longopts, NULL)) != -1)
  {
   switch (c)
    {
    case 'h':
        {
	  print_usage(argv[0]);
	  exit(0);
        }
      break;
    }
  }
// compute_file_name_base(argv[argc-1]);
}

static void print_version()
{
  cout <<"version 1.0 build 2"<<endl;
}


static void print_usage(const char * const program_name)
{
  cout <<"--------------------------------------------------------------"<<endl;
  cout <<"DiVinE Tool Set"<<endl;
  cout <<"--------------------------------------------------------------"<<endl;
  cout <<"This is a tool to precomipile DVE models, ";
  print_version();
  cout <<"--------------------------------------------------------------"<<endl;

  cout << "Usage:\t\tdivine.precompile [options] "
    "input_file.dve" << endl;
  cout <<
    "Options:"<<endl<<"\t\t-h, --help\tshow this help" << endl<<endl;
  cout <<"Description: This tool takes a DVE model file, and produces .dveC file of the same name."<<endl;
  cout <<"             The dveC file is a dynamically linked library with transition function of "<<endl;
  cout <<"             the original DVE file. The file .dveC may be used as an input file for other"<<endl;
  cout <<"             tools in the Toolset, e.g. divine.owcty, divine. generator, etc."<<endl;
}



int main(int argc, char ** argv)
{

 parse_command_line(argc,argv);

 dveC_explicit_system_t* p_System;

 char * filename = argv[argc-1];
 int filename_length = strlen(filename);
 if (filename_length>=2 && strcmp(filename+filename_length-2,".b")==0)
 {
   cout << "Bytecode is not allowed" << endl;
   exit(1);
 }
 else
 {
    cout << "Reading DVE source..." << endl;
    p_System = new dveC_explicit_system_t(gerr);
  }

  if (p_System->pure_read(filename))
  {
    cout << "Reading of " << filename << " was unsuccessful." << endl;
    exit(1);
  }
  ofstream ofstr;

  string cc_filename = filename;
  string dveC_filename = cc_filename.substr(0,strlen(filename)-3) + "dveC";
  cc_filename = cc_filename.substr(0,strlen(filename)-3) + "cc";

  cout << "Compiling DVE source..."<<flush;

  ofstr.open(cc_filename.c_str());
  if (ofstr)
  {
      p_System->print_include(ofstr);
      p_System->print_state_struct(ofstr);
      p_System->print_print_state(ofstr);
      p_System->print_dveC_compiler(ofstr);
      ofstr.close();
  }
  else
  {
    cout << endl<<"Unable to create a file " + cc_filename << endl;
    exit(2);
  }

#define CPP_MakeString_aux(s) #s
#define CPP_MakeString(s) CPP_MakeString_aux(s)

  system((string("g++ -O2 -g -shared -fPIC ")
#ifdef INSTALL_PREFIX
	  + string("-I ") + string(CPP_MakeString(INSTALL_PREFIX)) + string("/include/divine-cluster ")
#else
	  + string("-I ~/divine/src/ ")
#endif
	  + cc_filename + " -o " + dveC_filename + " -lc " 
#ifdef INSTALL_PREFIX
	  + string("-L ") + string(CPP_MakeString(INSTALL_PREFIX)) + string("/lib/ ")
#else
	  + string("-L ~/divine/lib/ ")
#endif
	  + string("-lsevine -lbymoc_vm")).c_str());
  cout <<" done, "<<dveC_filename<<" created."<<endl;
  return 0;


}


