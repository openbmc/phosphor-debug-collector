AM_DEFAULT_SOURCE_EXT = .cpp

# Build these headers, don't install them
noinst_HEADERS = \
	dump_entry.hpp 

nobase_nodist_include_HEADERS = \
	xyz/openbmc_project/Dump/Internal/Create/server.hpp

sbin_PROGRAMS = \
	phosphor-dump-manager

phosphor_dump_manager_SOURCES = \
	dump_manager_main.cpp \
	dump_entry.cpp \
	xyz/openbmc_project/Dump/Internal/Create/server.cpp

phosphor_dump_manager_CXXFLAGS = \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(SDBUSPLUS_CFLAGS)

phosphor_dump_manager_LDADD = \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(SDBUSPLUS_LIBS)

# Be sure to build needed files before compiling
BUILT_SOURCES = \
        xyz/openbmc_project/Dump/Internal/Create/server.cpp \
        xyz/openbmc_project/Dump/Internal/Create/server.hpp

CLEANFILES = \
        xyz/openbmc_project/Dump/Internal/Create/server.cpp \
        xyz/openbmc_project/Dump/Internal/Create/server.hpp

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
