AM_DEFAULT_SOURCE_EXT = .cpp

# Build these headers, don't install them
noinst_HEADERS = \
	dump_entry.hpp \
	dump_watch.hpp \
	dump_internal.hpp \
	dump_manager.hpp
	dump_manager.hpp \
	dump_utils.hpp

nobase_nodist_include_HEADERS = \
	xyz/openbmc_project/Dump/Monitor/error.hpp \
	xyz/openbmc_project/Dump/Internal/Create/server.hpp

sbin_PROGRAMS = \
	phosphor-dump-manager \
	phosphor-dump-monitor

phosphor_dump_manager_SOURCES = \
	dump_manager_main.cpp \
	dump_entry.cpp \
	dump_manager.cpp \
	xyz/openbmc_project/Dump/Internal/Create/server.cpp \
	xyz/openbmc_project/Dump/Monitor/error.cpp

phosphor_dump_monitor_SOURCES = \
	dump_watch_main.cpp \
	dump_watch.cpp \
	xyz/openbmc_project/Dump/Monitor/error.cpp

phosphor_dump_manager_CXXFLAGS = \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(SDBUSPLUS_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS)

phosphor_dump_monitor_CXXFLAGS = \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS)

phosphor_dump_manager_LDADD = \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(SDBUSPLUS_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	-lstdc++fs

phosphor_dump_monitor_LDADD = \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS) \
	-lstdc++fs

# Be sure to build needed files before compiling
BUILT_SOURCES = \
	xyz/openbmc_project/Dump/Internal/Create/server.cpp \
	xyz/openbmc_project/Dump/Internal/Create/server.hpp \
	xyz/openbmc_project/Dump/Monitor/error.cpp \
	xyz/openbmc_project/Dump/Monitor/error.hpp

CLEANFILES=${BUILT_SOURCES}

xyz/openbmc_project/Dump/Internal/Create/server.cpp: \
xyz/openbmc_project/Dump/Internal/Create.interface.yaml \
xyz/openbmc_project/Dump/Internal/Create/server.hpp
	@mkdir -p `dirname $@`
	$(SDBUSPLUSPLUS) -r $(srcdir) interface server-cpp \
xyz.openbmc_project.Dump.Internal.Create > $@

xyz/openbmc_project/Dump/Internal/Create/server.hpp: \
xyz/openbmc_project/Dump/Internal/Create.interface.yaml
	@mkdir -p `dirname $@`
	$(SDBUSPLUSPLUS) -r $(srcdir) interface server-header \
xyz.openbmc_project.Dump.Internal.Create > $@

xyz/openbmc_project/Dump/Monitor/error.hpp: \
${top_srcdir}/xyz/openbmc_project/Dump/Monitor.errors.yaml
	@mkdir -p `dirname $@`
	$(SDBUSPLUSPLUS) -r $(srcdir) error exception-header xyz.openbmc_project.Dump.Monitor > $@

xyz/openbmc_project/Dump/Monitor/error.cpp: ${top_srcdir}/xyz/openbmc_project/Dump/Monitor.errors.yaml
	@mkdir -p `dirname $@`
	$(SDBUSPLUSPLUS) -r $(srcdir) error exception-cpp xyz.openbmc_project.Dump.Monitor > $@
