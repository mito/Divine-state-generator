#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(DiVinE-cluster, 0.8.2, [divine(a)fi.muni.cz])
AC_SUBST(abs_top_srcdir)           #enables detection of absolute path
AC_CONFIG_AUX_DIR([support/src/makeutils])
AC_LANG(C++)
AM_INIT_AUTOMAKE([tar-pax foreign])

AC_CONFIG_SRCDIR([src/sevine.h])   #only safety check - not too important

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_CPP
AC_PROG_MAKE_SET
AC_PROG_RANLIB
AC_PROG_INSTALL

# Checks for libraries.
#AC_CHECK_LIB([divine], [main])
#AC_CHECK_LIB([sevine], [main])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stddef.h stdlib.h string.h sys/time.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CXX_NAMESPACES
AC_CXX_HAVE_IOS_BASE
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_TYPE_SIZE_T
AX_CREATE_STDINT_H(src/divine-int.hh)

# Checks for library functions.
AC_FUNC_ERROR_AT_LINE
#AC_FUNC_GETLOADAVG
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_REALLOC
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([alarm floor gettimeofday memset strchr strdup])

AC_PROG_JAVA
AC_PROG_JAVAC

# Checking for presence of MPI:
ACX_MPI(,[AC_MSG_WARN([Could not find properly installed MPI. Library libdivine.a cannot be compiled. Only sequential code will be compiled!!!], 1)])

# Checking for presence of mCRL2:
ACX_MCRL2

AC_DIVINE

# Checks for copy from CVS (in distribution not present - reason for silent
# include):
sinclude(ac_all_deps_of_cvs.m4)

#conditional make of support
AM_CONDITIONAL([BUILDSUPPORT], [test "$IS_CVS_COPY" = yes])
AM_CONDITIONAL([CVSDATAPATH], [test "$IS_CVS_COPY" = yes])
AM_CONDITIONAL([HAVEMPI], [test "x$MPI_FOUND" = "x1"])
AM_CONDITIONAL([HAVEMCRL2], [test "x$MCRL2" = "x1"])

#including macros for testing of command line parameters
#there are also set initial values modified by command line parameters
sinclude(support/src/makeutils/ac_parameters.m4)

AM_CONDITIONAL([DONTBUILDGUI], [test -z "$GUIDIR" || test "x$JAVACPROBLEM" = "x1" || test "x$JAVAPROBLEM" = "x1"])
AM_CONDITIONAL([JAVA_OK], [test "x$JAVAPROBLEM" != "x1"])
AM_CONDITIONAL([JAVAC_OK], [test "x$JAVACPROBLEM" != "x1"])
AM_CONDITIONAL([FORCE_CVS_STYLE_COMPILATION], 
               [test "$NON_CVS_STYLE_COMPILATION" = no &&
		test "$IS_CVS_COPY" = yes])

AC_CONFIG_FILES([
                Makefile
                lib/Makefile
		src/Makefile
		tool/Makefile
		tool/simulator/Makefile
		tool/ltl2ba/Makefile
		tool/lmu2ba/Makefile
		tool/reachability/Makefile
		tool/distr_reachability/Makefile
		tool/token_based_ndfs/Makefile
		tool/distr_map/Makefile
		tool/property_driven_ndfs/Makefile
		tool/model_manipulation/Makefile
		tool/model_manipulation/jar/Makefile
		tool/state_spaces/Makefile
		tool/deps_cycle_detection/Makefile
		tool/back_level_edge/Makefile
		tool/owcty_reversed/Makefile
		tool/owcty/Makefile
		tool/negative_cycle_detection/Makefile
		tool/utils/Makefile
		tool/gui_helpers/Makefile
		tool/ltl_mc/Makefile
		tool/ndfs/Makefile
                support/src/makeutils/Makefile
                support/Makefile
                doc/Makefile
                doc/refs/Makefile
                doc/tutorials/Makefile
                doc/refs/library/Makefile
                doc/refs/dwi/Makefile
                doc/tutorials/library/Makefile
                doc/tutorials/dve_language/Makefile
                doc/tutorials/toolset/Makefile
		doc/tutorials/development/Makefile
                support/src/regression_tests/dve/transition/Makefile
                support/src/regression_tests/dve/process/Makefile
                support/src/regression_tests/dve/expression/Makefile
		support/src/regression_tests/dve/Makefile
                support/src/regression_tests/Makefile
                support/src/debug/Makefile
                support/src/Makefile
                support/etc/doxygen/Makefile
                support/etc/Makefile
		support/src/local/Makefile
                dwi/Makefile
	       ])

if test "x$JAVAPROBLEM" = "x1"; then
	echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	echo "!!                                                            !!"
	echo "!!  Properly installed Java not found. NIPS compiler and GUI  !!"
	echo "!!  will not run!                                             !!"
	echo "!!                                                            !!"
	echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
fi

if test "x$JAVACPROBLEM" = "x1"; then
	echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	echo "!!                                                            !!"
	echo "!!  Properly installed Java compiler not found.               !!"
	echo "!!  GUI will not be compiled!                                 !!"
	echo "!!                                                            !!"
	echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
fi



if test "x$MPI_FOUND" != "x1"; then
	echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
	echo "!!                                                            !!"
	echo "!!  MPI not found. No distributed applications and libraries  !!"
	echo "!!  will be compiled!                                         !!"
	echo "!!                                                            !!"
	echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
fi


AC_OUTPUT











