AC_PREREQ([2.69])
AC_INIT([phosphor-debug-collector], [1.0], [https://github.com/openbmc/phosphor-debug-collector/issues])
AC_LANG([C++])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([subdir-objects -Wall -Werror foreign dist-xz])
AM_SILENT_RULES([yes])

# Checks for programs
AC_PROG_CXX
AC_PROG_INSTALL #Checks/sets the install variable to be used
AC_PROG_MAKE_SET

# Check for libraries
PKG_CHECK_MODULES([PHOSPHOR_DBUS_INTERFACES], [phosphor-dbus-interfaces],,\
    AC_MSG_ERROR(["Requires phosphor-dbus-interfaces package."]))
PKG_CHECK_MODULES([SDBUSPLUS], [sdbusplus],,
    AC_MSG_ERROR(["Requires sdbusplus package."]))

#PKG_CHECK_MODULES([PHOSPHOR_DEBUG_COLLECTOR], [phosphor-debug-collector],,\
#    AC_MSG_ERROR(["Requires phosphor-debug-collector package."]))

# Checks for library functions
LT_INIT # Required for systemd linking

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_14([noext])
AX_APPEND_COMPILE_FLAGS([-Wall -Werror], [CXXFLAGS])

AC_ARG_VAR(DUMP_BUSNAME, [The Dbus busname to own])
AS_IF([test "x$DUMP_BUSNAME" == "x"], [DUMP_BUSNAME="xyz.openbmc_project.Dump"])
AC_DEFINE_UNQUOTED([DUMP_BUSNAME], ["$DUMP_BUSNAME"], [The DBus busname to own])

AC_ARG_VAR(DUMP_OBJPATH, [The Dump manager Dbus root])
AS_IF([test "x$DUMP_OBJPATH" == "x"], [DUMP_OBJPATH="/xyz/openbmc_project/dump"])
AC_DEFINE_UNQUOTED([DUMP_OBJPATH], ["$DUMP_OBJPATH"], [The dump manager Dbus root])

# Check for header files.
AC_CHECK_HEADER(sdbusplus/server.hpp, ,[AC_MSG_ERROR([Could not find sdbusplus/server.hpp...openbmc/sdbusplus package required])])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
