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
AC_PROG_MKDIR_P
AC_CHECK_PROG([DIRNAME], dirname, dirname)

# Check for libraries
PKG_CHECK_MODULES([SYSTEMD], [libsystemd >= 221],,\
    AC_MSG_ERROR(["systemd required and not found."]))
PKG_CHECK_MODULES([PHOSPHOR_DBUS_INTERFACES], [phosphor-dbus-interfaces],,\
    AC_MSG_ERROR(["Requires phosphor-dbus-interfaces package."]))
PKG_CHECK_MODULES([SDBUSPLUS], [sdbusplus],,\
    AC_MSG_ERROR(["Requires sdbusplus package."]))
PKG_CHECK_MODULES([PHOSPHOR_LOGGING], [phosphor-logging],,\
    AC_MSG_ERROR(["Requires phosphor-logging package."]))

# Check for sdbus++
AC_PATH_PROG([SDBUSPLUSPLUS], [sdbus++])
AS_IF([test "x$SDBUSPLUSPLUS" == "x"],
    AC_MSG_ERROR(["Requires sdbus++"]))

# Check for sdbus++
AC_PATH_PROG([SDBUSPLUSPLUS], [sdbus++])
AS_IF([test "x$SDBUSPLUSPLUS" == "x"],
    AC_MSG_ERROR(["Requires sdbus++"]))

# Checks for library functions
LT_INIT # Required for systemd linking

# Checks for typedefs, structures, and compiler characteristics.
AX_CXX_COMPILE_STDCXX_14([noext])
AX_APPEND_COMPILE_FLAGS([-Wall -Werror], [CXXFLAGS])

AC_ARG_VAR(DUMP_BUSNAME, [The Dbus busname to own])
AS_IF([test "x$DUMP_BUSNAME" == "x"], [DUMP_BUSNAME="xyz.openbmc_project.Dump.Manager"])
AC_DEFINE_UNQUOTED([DUMP_BUSNAME], ["$DUMP_BUSNAME"], [The DBus busname to own])

AC_ARG_VAR(DUMP_OBJPATH, [The Dump manager Dbus root])
AS_IF([test "x$DUMP_OBJPATH" == "x"], [DUMP_OBJPATH="/xyz/openbmc_project/dump"])
AC_DEFINE_UNQUOTED([DUMP_OBJPATH], ["$DUMP_OBJPATH"], [The dump manager Dbus root])

AC_ARG_VAR(CORE_FILE_DIR, [Directory where core dumps are placed])
AS_IF([test "x$CORE_FILE_DIR" == "x"], [CORE_FILE_DIR="/var/lib/systemd/coredump"])
AC_DEFINE_UNQUOTED([CORE_FILE_DIR], ["$CORE_FILE_DIR"], [Directory where core dumps are placed])

AC_ARG_VAR(OBJ_INTERNAL, [Internal Dump manager Dbus object path])
AS_IF([test "x$OBJ_INTERNAL" == "x"], [OBJ_INTERNAL="/xyz/openbmc_project/dump/internal/manager"])
AC_DEFINE_UNQUOTED([OBJ_INTERNAL], ["$OBJ_INTERNAL"], [Internal Dump manager Dbus object path])

AC_ARG_VAR(OBJ_ENTRY, [The dump entry DBus object path.])
AS_IF([test "x$OBJ_ENTRY" == "x"], [OBJ_ENTRY="/xyz/openbmc_project/dump/entry"])
AC_DEFINE_UNQUOTED([OBJ_ENTRY], ["$OBJ_ENTRY"], [The dump entry DBus object path])

# TODO openbmc/openbmc#1795
# Change the path to Dump Partition path.
AC_ARG_VAR(BMC_DUMP_FILE_DIR, [Directory where bmc dumps are placed])
AS_IF([test "x$BMC_DUMP_FILE_DIR" == "x"], [BMC_DUMP_FILE_DIR="/tmp"])
AC_DEFINE_UNQUOTED([BMC_DUMP_FILE_DIR], ["$BMC_DUMP_FILE_DIR"], [Directory where bmc dumps are placed])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
