AM_DEFAULT_SOURCE_EXT = .cpp

# Build these headers, don't install them
noinst_HEADERS = \
	dump_entry.hpp \
        dump_manager.hpp

sbin_PROGRAMS = \
	phosphor-dump-manager

phosphor_dump_manager_SOURCES = \
	dump_manager_main.cpp \
        dump_entry.cpp \
	dump_manager.cpp

generic_cxxflags = \
	$(SYSTEMD_CFLAGS) \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(SDBUSPLUS_CFLAGS)

generic_ldflags = \
	$(SYSTEMD_LIBS) \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(SDBUSPLUS_LIBS)

phosphor_dump_manager_CXXFLAGS = $(generic_cxxflags)
phosphor_dump_manager_LDFLAGS = $(generic_ldflags)
