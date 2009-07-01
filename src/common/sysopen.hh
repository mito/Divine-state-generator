#ifndef _DIVINE_SYSOPEN_HH_
#define _DIVINE_SYSOPEN_HH_

/*!\file sysopen.hh
 * 
 * This header file declare class that should be used for opening models,
 * printing help and version in divine-cluster tools.
 *
 * \author Jiri Barnat
 */

#include <iostream>
#include <cstring>
#include <string>
#include "system/dve/dve_explicit_system.hh"
#include "system/dveC/dveC_explicit_system.hh"
#include "system/bymoc/bymoc_explicit_system.hh"

#ifndef DOXYGEN_PROCESSING
namespace divine { //We want Doxygen not to see namespace `dve'
#endif //DOXYGEN_PROCESSING


  class system_description_t {
  public:
    std::string input_file_ext;

    system_description_t();
    ~system_description_t();

    explicit_system_t* open_system_from_file(char * filename, 
					     bool compileDveToDveC, 
					     bool verbose);    
  };

#ifndef DOXYGEN_PROCESSING
} //namespace divine
#endif


#endif

