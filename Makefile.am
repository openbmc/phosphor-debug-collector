AM_DEFAULT_SOURCE_EXT = .cpp

# Build these headers, don't install them
noinst_HEADERS = \
	dump_entry.hpp \
	core_manager.hpp \
	dump_internal.hpp \
	dump_manager.hpp \
	dump_utils.hpp \
	watch.hpp \
	elog_watch.hpp \
	dump_serialize.hpp

nobase_nodist_include_HEADERS = \
	xyz/openbmc_project/Dump/Internal/Create/server.hpp

# Application test which runs in obmc env (qemu, real hardware)
bin_PROGRAMS = \
    elog-report

sbin_PROGRAMS = \
	phosphor-dump-manager \
	phosphor-dump-monitor

elog_report_SOURCES = \
    elog_report.cpp

phosphor_dump_manager_SOURCES = \
	dump_manager_main.cpp \
	dump_entry.cpp \
	dump_manager.cpp \
	watch.cpp \
	xyz/openbmc_project/Dump/Internal/Create/server.cpp \
	elog_watch.cpp \
	dump_serialize.cpp

phosphor_dump_monitor_SOURCES = \
	watch.cpp \
	core_manager.cpp \
	core_manager_main.cpp

elog_report_LDFLAGS = \
    $(SYSTEMD_LIBS) \
    $(SDBUSPLUS_LIBS) \
    $(PHOSPHOR_LOGGING_LIBS)
elog_report_CXXFLAGS = \
    $(SYSTEMD_CFLAGS) \
    $(SDBUSPLUS_CFLAGS) \
    $(PHOSPHOR_LOGGING_CFLAGS)

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
	xyz/openbmc_project/Dump/Internal/Create/server.hpp

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
