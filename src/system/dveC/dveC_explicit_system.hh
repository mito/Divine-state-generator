/*!\file
 * The main contribution of this file is the class dveC_explicit_system_t
 */
#ifndef DIVINE_DVEC_EXPLICIT_SYSTEM_HH
#define DIVINE_DVEC_EXPLICIT_SYSTEM_HH

#ifndef DOXYGEN_PROCESSING
#include "system/dve/dve_explicit_system.hh"
#include "system/explicit_system.hh"
#include "system/state.hh"
#include "common/types.hh"
#include "common/deb.hh"
#include <sstream>
#include <fstream>
#include <string>

namespace divine { //We want Doxygen not to see namespace `dve'
using std::cerr; using std::endl;
#endif //DOXYGEN_PROCESSING

class succ_container_t;
class enabled_trans_container_t;

//!This class is used for compiled execution of DVE models. In particular, it
//!has methods to generate C source of a function to generate successors from a
//!state representation, and it provides the necessary interface to use this C
//!source as runtime compiled dynamically linked library.
/*!dveC_explicit_system_t is the immediate descendant of a class
 * dve_explicit_system_t.  It redefines methods for reading the DVE model, and
 * methods that are implemented within the dynamically linked library.
 */
class dveC_explicit_system_t : public dve_explicit_system_t
{

 protected:
  //!Auxiliary method to prints C-source of a the current system.
  static void write_C(dve_expression_t &, std::ostream &, std::string);
    
 public:
  slong_int_t read(const char * const filename);
 
  //!A constructor
  /*!\param evect = \evector used for reporting of error messages*/
  dveC_explicit_system_t(error_vector_t & evect);
 
  //!Function to test whether the given state is accepting. 
  /*!Calls dynamically linked library for Buchi acceptance, and dve_explicit_system_t::is_accepting otherwise.
   */
  virtual bool is_accepting(state_t state, size_int_t acc_group=0, size_int_t pair_member=1);

  //   virtual void print_state(state_t state, std::ostream & outs);
  //   virtual state_t get_initial_state();

  //!Function to generate immediate successors of the given !state.
  /*!Calls dynamically linked library to generate successors.
   */
  virtual int get_succs(state_t state, succ_container_t & succs);

  //!A destructor
  virtual ~dveC_explicit_system_t();//!<A destructor.
  
  //!Auxiliary method to print and compile C-source of a the current system.
  void print_dveC_compiler(std::ostream & ostr);
  //!Auxiliary method to print and compile C-source of a the current system.
  void print_state_struct(std::ostream & ostr);
  //!Auxiliary method to print and compile of a the current system.
  void print_include(std::ostream & ostr);
  //!Auxiliary method to print and compile C-source of a the current system.
  void print_print_state(std::ostream & ostr);

  //!Auxiliary method to print and compile C-source of a the current system.
  slong_int_t pure_read(const char * const filename);
   
}; //END of class dveC_explicit_system_t



#ifndef DOXYGEN_PROCESSING  
} //END of namespace DVE
#include "common/undeb.hh"

#endif //DOXYGEN_PROCESSING

#endif
