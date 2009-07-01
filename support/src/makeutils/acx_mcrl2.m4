AC_DEFUN([ACX_MCRL2], [
MCRL2=1

AC_ARG_WITH(
   [mcrl2],
   AS_HELP_STRING([--with-mcrl2=PREFIX],
                  [build with mCRL2 toolset installed into PREFIX]),
   if test x$withval = xno; then
      MCRL2=
   else
      MCRL2PREFIX="${withval}"
   fi,
   MCRL2PREFIX=yes
)

if test xyes = x$MCRL2PREFIX; then
    MCRL2PREFIX=$prefix
fi

MCRL2CFLAGS="-I$MCRL2PREFIX""/include -I$MCRL2PREFIX""/include/mcrl2/aterm"
MCRL2LDFLAGS="-L$MCRL2PREFIX""/lib/mcrl2"
MCRL2LIBS="-lmcrl2"

if test x1 = x$MCRL2; then
    old_LIBS=$LIBS
    old_LDFLAGS=$LDFLAGS
    LIBS="$LIBS $MCRL2LIBS"
    LDFLAGS="$LDFLAGS $MCRL2LDFLAGS"
    AC_CHECK_FUNC(ATwriteToSAFString, [], [MCRL2=])
    # TODO check headers
    LIBS=$old_LIBS
    LDFLAGS=$old_LDFLAGS
fi

if test x1 = x$MCRL2; then
    AC_DEFINE(HAVE_MCRL2,1,[Define if you have the mCRL2 library.])
else
    MCRL2CFLAGS=
    MCRL2LDFLAGS=
    MCRL2LIBS=
fi

AC_SUBST(MCRL2CFLAGS)
AC_SUBST(MCRL2LDFLAGS)
AC_SUBST(MCRL2LIBS)

])dnl ACX_MCRL2
