#include "common/sysopen.hh"

using namespace divine;
using namespace std;

system_description_t::system_description_t()
{
};


system_description_t::~system_description_t()
{
};


divine::explicit_system_t* 
system_description_t::open_system_from_file(char *filename, bool compileDveToDveC, bool verbose)
{
  divine::explicit_system_t* sys=0;

  int filename_length = strlen(filename);
  if (filename_length>=2 && strcmp(filename+filename_length-2,".b")==0)
   {
    if (verbose) cout << "Reading bytecode source ..." << endl;
    input_file_ext = ".b";
    sys = new bymoc_explicit_system_t(gerr);
   }

  if ( (filename_length>=4 && strcmp(filename+filename_length-4,".dve")==0) ||
       (filename_length>=5 && strcmp(filename+filename_length-5,".dveC")==0)
       )
    {
      if (verbose) cout << "Reading ";
     input_file_ext = ".dve";
     if(  compileDveToDveC ||
	  (filename_length>=5 && strcmp(filename+filename_length-5,".dveC")==0)
	  )       
       {
	 sys = new dveC_explicit_system_t(gerr);
	 if (verbose 
	     && filename_length>=4 
	     && strcmp(filename+filename_length-4,".dve")==0)
	   {
	     cout << "and compiling ";
	   }
       }
     else
       sys = new dve_explicit_system_t(gerr);
     if (verbose) cout << "DVE source..." << endl;
    }
  return sys;
};
