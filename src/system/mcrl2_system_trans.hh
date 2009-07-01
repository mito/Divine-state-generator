#ifndef DIVINE_MCRL_SYSTEM_TRANS_HH
#define DIVINE_MCRL_SYSTEM_TRANS_HH

#ifdef HAVE_MCRL2

#ifndef DOXYGEN_PROCESSING
#include "system/system_trans.hh"
#include <aterm2.h>

namespace divine { //We want Doxygen not to see namespace `dve'
#endif //DOXYGEN_PROCESSING

class explicit_system_t;

//!Class implementing system trasition in mCRL2
class mcrl2_system_trans_t : virtual public system_trans_t
{
  private:
   ATermAppl trans;
   state_t succ;
  protected:
   void copy_from(const system_trans_t & second);
  public:
   //!A constructor
   mcrl2_system_trans_t() {
       trans = NULL;
       ATprotect((ATerm*) &trans);
   }
   mcrl2_system_trans_t(ATermAppl trans, state_t succ) {
       this->trans = trans;
       ATprotect((ATerm*) &(this->trans));
       this->succ = succ;
   }
   //!A destructor
   virtual ~mcrl2_system_trans_t() {
       ATunprotect((ATerm*) &trans);
   }
   //!An assignment operator
   /*!Makes a hard copy of system transition => takes a time O(1)*/
   virtual system_trans_t & operator=(const system_trans_t & second);
   //!Returns a string representation of enabled transition
   virtual std::string to_string() const;
   //!Prints a string representation of enabled trantition to output stream
   //! `ostr'
   virtual void write(std::ostream & ostr) const;

   /*! @name Methods accessing transitions forming a system transition
    * Mostly unimplemented, except for get_count.
   @{*/

   //!Unimpl.
   virtual void set_count(const size_int_t new_count) { UNIMPLEMENTED(void); }
   //!Returns 0 for compatibility with DVE.
   virtual size_int_t get_count() const { return 0; }
   //!Unimpl.
   virtual transition_t *& operator[](const int i) { UNIMPLEMENTED(transition_t *&); }
   //!Unimpl.
   virtual transition_t * const & operator[](const int i) const { UNIMPLEMENTED(transition_t *&); }
   /*@}*/

   const ATermAppl & get_trans() const { return trans; }
   const state_t & get_succ() const { return succ; }
 };

//!Class storing informations about one enabled transition
/*!Enabled transition = system transition + erroneous property
 *
 * Enabled transitions can be produced by \exp_sys (explicit_system_t),
 * if it can work with enabled transitions.
 *
 * Each enabled transition represents a possible step of a \sys in a given
 * state of the \sys.
 */
class mcrl2_enabled_trans_t: public enabled_trans_t, public mcrl2_system_trans_t
{
 public:
 //!A constructor
 mcrl2_enabled_trans_t() {}
 mcrl2_enabled_trans_t(ATermAppl trans, state_t succ) :
     mcrl2_system_trans_t(trans, succ) {}
 //!An assignment operator
 virtual enabled_trans_t & operator=(const enabled_trans_t & second);
};

#ifndef DOXYGEN_PROCESSING  
} //END of namespace DVE

#endif //DOXYGEN_PROCESSING

#endif /* HAVE_MCRL2 */
#endif

