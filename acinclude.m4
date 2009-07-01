#including custom macros of DiVinE
m4_include([support/src/makeutils/acdivine.m4])

#including downloaded macros for testing MPI
m4_include([support/src/makeutils/acx_mpi.m4])

#including ourselves-made macro for testing of presence of doxygen
m4_include([support/src/makeutils/acx_doxygen.m4])

#including ourselves-made macro for testing of presence of Apache Ant
m4_include([support/src/makeutils/acx_ant.m4])

#including a test of namespaces
m4_include([support/src/makeutils/ac_cxx_namespaces.m4])

#including ourselves-made macro for testing of presence of ios_base
m4_include([support/src/makeutils/ac_ios_base.m4])

# for dwi
m4_include([support/src/makeutils/ac_prog_java_works.m4])
m4_include([support/src/makeutils/ac_prog_java.m4])
m4_include([support/src/makeutils/ac_prog_javac_works.m4])
m4_include([support/src/makeutils/ac_prog_javac.m4])

#for integer types
m4_include([support/src/makeutils/ax_create_stdint_h.m4])

#including ourselves-made macro for testing of presence of mcrl2
m4_include([support/src/makeutils/acx_mcrl2.m4])
