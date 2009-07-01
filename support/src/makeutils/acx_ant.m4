dnl Checking of presence of Apache Ant, doesn't check the version

AC_DEFUN([ACX_ANT], [
AC_PREREQ(2.50)

AC_CHECK_PROG(ANT, ant, ant)
AC_SUBST(ANT)

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x = x"$ANT"; then
        AC_MSG_ERROR(cannot find Apache Ant - i. e. executable file ant)
fi
       
]) dnl ACX_ANT


