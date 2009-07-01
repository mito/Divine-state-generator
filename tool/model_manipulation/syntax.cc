#include <cstdio>
#include <iostream>
#include "sevine.h"

using namespace std;
using namespace divine;

int main(int argc, char* argv[]) {  

  if (argc<2) //no parameter given
    {
      cerr << "No input file given!" << endl;
      return system_t::ERR_FILE_NOT_OPEN;
    }
  else
    {

      bool recognized = false;
      int file_opening=0;
      
      char * filename = argv[optind];
      int filename_length = strlen(filename);
      if (filename_length>=8 && strcmp(filename+filename_length-8,".probdve")==0)
	{
	  dve_prob_explicit_system_t *sys = new dve_prob_explicit_system_t(gerr);
	  file_opening = sys->read(argv[1]);
	  recognized = true;
	}

      if (filename_length>=4 && strcmp(filename+filename_length-4,".dve")==0)
	{
	  dve_system_t *sys = new dve_system_t(gerr);
	  file_opening = sys->read(argv[1]);
	  recognized = true;
	}
	  
      if (filename_length>=2 && strcmp(filename+filename_length-2,".b")==0)
	{
	  bymoc_explicit_system_t *sys = new bymoc_explicit_system_t(gerr);
	  file_opening = sys->read(argv[1]);
	  recognized = true;
	}

      if (!recognized)
	{
	  cerr << "File not recognized. Supported extensions are .b, .dve, and .probdve" << endl;
	  return 1;
	}

      if (file_opening==system_t::ERR_FILE_NOT_OPEN)
	cerr << "File not opened" << endl;
      else if (file_opening)
	cerr << "Syntax error" << endl;
      else
	cout << "OK" << endl;
    
      return file_opening;
    }
}
