/*!\file
 * The main contribution of this file is the class mcrl2_system_t
 */
#ifndef DIVINE_MCRL_SYSTEM_HH
#define DIVINE_MCRL_SYSTEM_HH

#ifdef HAVE_MCRL2

#ifndef DOXYGEN_PROCESSING
#include <fstream>
#include <string>
#include <math.h>
#include <list>
#include "system/system.hh"
#include "system/explicit_system.hh" //because of succ_container_t

#include <mcrl2/lps/specification.h>
#include <mcrl2/lps/nextstate.h>
#include <mcrl2/core/aterm_ext.h>
#include <aterm2.h>

#include "common/array.hh"
#include "common/error.hh"
#ifdef count
 #undef count
#endif
#ifdef max
 #undef max
#endif
#ifdef min
 #undef min
#endif
#ifdef PACKED
 #undef PACKED
#endif

//The main DiVinE namespace - we do not want Doxygen to see it
namespace divine {
#endif //DOXYGEN_PROCESSING

//!Class for mCRL2 LPS.
/*!This class implements the abstract interface system_t
 *
 * It supports only very basic functionality of system_t interface
 * (processes, transition and expressions are not supported, only some basic
 * system transition stuff works)
 * The calls of non-implemented methods throw exceptions.
 */
class mcrl2_system_t: virtual public system_t
 {
  public:

   mcrl2::lps::specification* mcrl2_system;
   NextState* nstate;

  //!A constructor.
  /*!\param estack =
   * the <b>error vector</b>, that will be used by created instance of system_t
   */
  mcrl2_system_t(error_vector_t & evect = gerr);
  //!A destructor.
  virtual ~mcrl2_system_t();

  /*! @name Obligatory part of abstact interface
     These methods have to implemented in each implementation of system_t
  @{*/
  //!Unimpl.
  virtual slong_int_t read(std::istream & ins = std::cin) { UNIMPLEMENTED(slong_int_t); }
  //!Implements system_t::read(const char * const filename) in mCRL2 \sys
  virtual slong_int_t read(const char * const filename);
  //!Unimpl.
  virtual slong_int_t from_string(const std::string str) { UNIMPLEMENTED(slong_int_t); }
  //!Unimpl.
  virtual bool write(const char * const filename) { UNIMPLEMENTED(bool); }
  //!Unimpl.
  virtual void write(std::ostream & outs = std::cout) { UNIMPLEMENTED(void); }
  //!Unimpl.
  virtual std::string to_string() { UNIMPLEMENTED(std::string); }
  /*@}*/

  /*! @name Methods working with property process
    These methods are not implemented and can_property_process() returns false
   @{*/
  //!Unimpl.
  virtual process_t * get_property_process() { UNIMPLEMENTED(process_t *); }
  //!Unimpl.
  virtual const process_t * get_property_process() const { UNIMPLEMENTED(process_t *); }
  //!Unimpl.
  virtual size_int_t get_property_gid() const { UNIMPLEMENTED(size_int_t); }
  //!Unimpl.
  virtual void set_property_gid(const size_int_t gid) { UNIMPLEMENTED(void); }
  /*@}*/

  /*!@name Methods working with processes
    These methods are not implemented and can_processes() returns false.
   @{*/
  //!Returns 0 for compatibility with DVE.
  virtual size_int_t get_process_count() const { return 0; }
  //!Unimpl.
  virtual process_t * get_process(const size_int_t gid) { UNIMPLEMENTED(process_t *); }
  //!Unimpl.
  virtual const process_t * get_process(const size_int_t id) const { UNIMPLEMENTED(process_t *); }
  //!Unimpl.
  virtual property_type_t get_property_type() { UNIMPLEMENTED(property_type_t); }
  /*@}*/


  /*!@name Methods working with transitions
    These methods are not implemented and can_transitions() returns false.
   @{*/
  //!Unimpl.
  virtual size_int_t get_trans_count() const { UNIMPLEMENTED(size_int_t); }
  //!Unimpl.
  virtual transition_t * get_transition(size_int_t gid) { UNIMPLEMENTED(transition_t *); }
  //!Unimpl.
  virtual const transition_t * get_transition(size_int_t gid) const { UNIMPLEMENTED(transition_t *); }
  /*@}*/
  
  /*!@name Methods modifying a system
     These methods are not implemented and can_be_modified() returns false.
   @{*/
  //!Unimpl.
  virtual void add_process(process_t * const process) { UNIMPLEMENTED(void); }
  //!Unimpl.
  virtual void remove_process(const size_int_t process_id) { UNIMPLEMENTED(void); }
  /*@}*/


  /*!@name mCRL2 aterm storage helpers.
   @{*/

  /*! A variable that is consulted whenever we do anything with the storage
   * of a state, since if we're storing aterm pointers, we need to gc protect
   * it. This is a hack! :)
   */
  static bool binstates;

  /*! A caching version of ATwriteToSAFString used to avoid the doubling of
   * the call between partition_function and message_t::append_state.
   *
   * This function has to free the string since no one else will do that.
   * It will leave one last unfreed block at the end of execution. Sorry. :)
   */
  static char *ATwriteToSAFString(ATerm t, int *len) {
    static ATerm t_ = NULL;
    static char *res = NULL;
    static int len_ = 0;

    if (t == t_) {
      *len = len_;
      return res;
    }

    t_ = t;
    if (res) free(res);
    res = ::ATwriteToSAFString(t, &len_);
    *len = len_;
    return res;
  }
  /*@}*/
 };

#ifndef DOXYGEN_PROCESSING  
} //END of namespace DVE
#endif //DOXYGEN_PROCESSING

#else
#define MCRL2_ATERM_INIT(x,y) do{}while(0)
#endif /* HAVE_MCRL2 */
#endif
