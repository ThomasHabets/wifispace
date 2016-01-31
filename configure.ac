AC_PREREQ(2.61)
# Other names:
#   wifispace
AC_INIT(wifiscanner, 0.01, thomas@habets.se)
AC_CANONICAL_SYSTEM
AC_CONFIG_SRCDIR([src/main.cc])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE
AC_CONFIG_HEADER(config.h)

AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AC_CHECK_LIB([rt], [clock_gettime])
AC_CHECK_LIB([boost_system], [_ZN5boost6system16generic_categoryEv])
AC_CHECK_LIB([boost_iostreams], [_ZN5boost9iostreams4zlib16best_compressionE])
# AC_CHECK_LIB([boost_program_options], [_ZN5boost15program_options29options_description_easy_initclEPKcS3_])

PKG_CHECK_MODULES([GNURADIO], [gnuradio-digital gnuradio-blocks gnuradio-uhd gnuradio-runtime uhd])
CXXFLAGS="-std=gnu++0x -Wall $GNURADIO_CFLAGS $CXXFLAGS"
LDFLAGS="$GNURADIO_LIBS $LDFLAGS"

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

echo "
  $PACKAGE_NAME version $PACKAGE_VERSION
  Prefix.........: $prefix
  Debug Build....: $debug
  C++ Compiler...: $CXX $CXXFLAGS $CPPFLAGS
  Linker.........: $LD $LDFLAGS $LIBS
"
