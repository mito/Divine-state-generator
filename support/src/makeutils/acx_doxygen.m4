dnl Checking of presence of Doxygen, doesn't check the version

AC_DEFUN([ACX_DOXYGEN], [
AC_PREREQ(2.50)

AC_CHECK_PROG(DOXYGEN, doxygen, doxygen)
AC_SUBST(DOXYGEN)

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x = x"$DOXYGEN"; then
        AC_MSG_ERROR(cannot find doxygen)
fi
       
]) dnl ACX_DOXYGEN

