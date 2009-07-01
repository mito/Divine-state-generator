/*!\file
 * The main contribution of this file is the class mcrl2_explicit_system_t
 */
#ifndef DIVINE_MCRL_EXPLICIT_SYSTEM_HH
#define DIVINE_MCRL_EXPLICIT_SYSTEM_HH

#ifdef HAVE_MCRL2

#ifndef DOXYGEN_PROCESSING
#include "system/explicit_system.hh"
#include "system/mcrl2_system.hh"
#include "system/dve/dve_explicit_system.hh"
#include "system/state.hh"
#include "storage/explicit_storage.hh"
#include "common/types.hh"
#include "common/deb.hh"
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

namespace divine { //We want Doxygen not to see namespace `dve'
using std::cerr; using std::endl;
#endif //DOXYGEN_PROCESSING

class succ_container_t;

class mcrl2_explicit_system_t : public explicit_system_t, public mcrl2_system_t
{
 public:

 //!A constructor
 /*!\param evect = \evector used for reporting of error messages*/
 mcrl2_explicit_system_t(error_vector_t & evect);
 //!A destructor
 virtual ~mcrl2_explicit_system_t();//!<A destructor.

 /*! @name Obligatory part of abstact interface
    These methods have to implemented in each implementation of
    explicit_system_t  */

 //!Returns false for compatibility with DVE.
 virtual bool is_erroneous(state_t state) { return false; }
 //!Returns false for compatibility with DVE.
 virtual bool is_accepting(state_t state, size_int_t acc_group, size_int_t pair_member)
 { return false; }
 //!Returns false for compatibility with DVE.
 virtual bool violates_assertion(const state_t state) const { return false; }
 //!Returns 0 for compatibility with DVE.
 virtual size_int_t violated_assertion_count(const state_t state) const
 { return 0; }
 //!Returns empty string for compatibility with DVE.
 virtual std::string violated_assertion_string(const state_t state,
                                               const size_int_t index) const
 { return std::string(""); }

 /*!This methods always returns 10000. No better estimation is implemented. */
 virtual size_int_t get_preallocation_count() const { return 10000; }
 //!Implements explicit_system_t::print_state() in mCRL2 \sys
 virtual void print_state(state_t state, std::ostream & outs = std::cout);
 //!Implements explicit_system_t::get_initial_state() in mCRL2 \sys
 virtual state_t get_initial_state();
 //!Implements explicit_system_t::get_succs() in mCRL2 \sys
 virtual int get_succs(state_t state, succ_container_t & succs)
 { return get_succs(state, &succs, NULL); }
 //!Unimpl.
 virtual int get_ith_succ(state_t state, const int i, state_t & succ) { UNIMPLEMENTED(int); }
 /*@}*/

 //!Primary get_succs function that fills the containers only if given.
 virtual int get_succs(state_t state, succ_container_t * succs,
               enabled_trans_container_t * etc);

 //!Implements explicit_system_t::get_succs() in mCRL2 \sys
 virtual int get_succs(state_t state, succ_container_t & succs,
               enabled_trans_container_t & etc)
 { return get_succs(state, &succs, &etc); }
 //!Implements explicit_system_t::get_enabled_trans() in mCRL2 \sys
 virtual int get_enabled_trans(const state_t state,
                       enabled_trans_container_t & enb_trans)
 { return get_succs(state, NULL, &enb_trans); }
 //!Unimpl.
 virtual int get_enabled_trans_count(const state_t state, size_int_t & count) { UNIMPLEMENTED(int); }
 //!Unimpl.
 virtual int get_enabled_ith_trans(const state_t state,
                                 const size_int_t i,
                                 enabled_trans_t & enb_trans)
 { UNIMPLEMENTED(int); }
 //!Implements explicit_system_t::get_enabled_trans_succ() in mCRL2 \sys
 virtual bool get_enabled_trans_succ
   (const state_t state, const enabled_trans_t & enabled,
    state_t & new_state);
 //!Unimpl.
 virtual bool get_enabled_trans_succs
   (const state_t state, succ_container_t & succs,
    const enabled_trans_container_t & enabled_trans) { UNIMPLEMENTED(bool); }
 //!Implements explicit_system_t::new_enabled_trans() in mCRL2 \sys
 virtual enabled_trans_t * new_enabled_trans() const;
 /*@}*/

 /*! @name Methods for expression evaluation
   These methods are not implemented and can_evaluate_expressions() returns
   false
  @{*/
 //!Unimpl.
 virtual bool eval_expr(const expression_t * const expr,
                        const state_t state,
                        data_t & data) const { UNIMPLEMENTED(bool); }
 /*@}*/
}; //END of class mcrl2_explicit_system_t

#ifndef DOXYGEN_PROCESSING  
} //END of namespace DVE
#include "common/undeb.hh"

#endif //DOXYGEN_PROCESSING

#endif /* HAVE_MCRL2 */
#endif
