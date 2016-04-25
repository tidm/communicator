AC_PREREQ([2.67])
AC_INIT([oi_communicator], [7.0], [oi@tidm.ir])
LT_INIT
LT_LANG([C++])
AC_LANG([C++])
AX_CXX_COMPILE_STDCXX_11([noext], [mandatory])


AM_INIT_AUTOMAKE([foreign subdir-objects -Wall -Werror])
AM_SILENT_RULES([yes])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config.h])

AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB

BOOST_REQUIRE([1.49])
BOOST_SERIALIZATION([mt])
BOOST_SYSTEM([mt])
PKG_CHECK_MODULES([ZMQ], [libzmq >= 4.0])
PKG_CHECK_MODULES([OI_EXCEPTION], [oi_exception >= 7.0])
PKG_CHECK_MODULES([OI_COMMON], [oi_common >= 7.0])

AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_INT32_T
AC_TYPE_SIZE_T
AC_TYPE_UINT16_T
AC_TYPE_UINT64_T

AC_CHECK_FUNCS([clock_gettime sqrt])

AC_CONFIG_FILES([Makefile
                src/Makefile
                test/Makefile
                oi_communicator.pc])

AC_SUBST([OI_COMMUNICATOR_SO_VERSION], [7:0:0])
AC_SUBST([OI_COMMUNICATOR_API_VERSION], [7.0.0])

AC_DEFINE_UNQUOTED(IPC_FILE_PATH, "${prefix}/var/run/", [directory in with ipc socket files will be created])

AC_OUTPUT
