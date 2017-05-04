AM_DEFAULT_SOURCE_EXT = .cpp

# Build these headers, don't install them
noinst_HEADERS = \
	dump_entry.hpp

sbin_PROGRAMS = \
	phosphor-dump-manager

phosphor_dump_manager_SOURCES = \
	dump_manager_main.cpp \
	dump_entry.cpp

phosphor_dump_manager_CXXFLAGS = \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(SDBUSPLUS_CFLAGS)

phosphor_dump_manager_LDADD = \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(SDBUSPLUS_LIBS)
