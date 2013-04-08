AC_PREREQ([2.67])
AC_INIT( [sudokusolver], [0.1.0], [snyder.bradleyj@gmail.com])
AM_INIT_AUTOMAKE(sudokusolver, 0.1.0)
AC_OUTPUT(Makefile src/Makefile)

AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB([m], [pow])
AC_CHECK_LIB([rt], [clock_gettime])
AC_CHECK_LIB([ncurses], [initscr])

# Checks for header files.
AC_CHECK_HEADERS([fcntl.h sys/time.h])

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.

AC_OUTPUT
