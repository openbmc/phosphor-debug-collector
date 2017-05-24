AM_DEFAULT_SOURCE_EXT = .cpp

# Build these headers, don't install them
noinst_HEADERS = \
	dump_entry.hpp \
	dump_watch.hpp

sbin_PROGRAMS = \
	phosphor-dump-manager \
	phosphor-dump-monitor

phosphor_dump_manager_SOURCES = \
	dump_manager_main.cpp \
	dump_entry.cpp

phosphor_dump_monitor_SOURCES = \
	dump_watch_main.cpp \
	dump_watch.cpp

phosphor_dump_manager_CXXFLAGS = \
	$(PHOSPHOR_DBUS_INTERFACES_CFLAGS) \
	$(SDBUSPLUS_CFLAGS) \
	$(PHOSPHOR_LOGGING_CFLAGS)

phosphor_dump_monitor_CXXFLAGS = \
	$(PHOSPHOR_LOGGING_CFLAGS)

phosphor_dump_manager_LDADD = \
	$(PHOSPHOR_DBUS_INTERFACES_LIBS) \
	$(SDBUSPLUS_LIBS) \
	$(PHOSPHOR_LOGGING_LIBS)

phosphor_dump_monitor_LDADD = \
	$(PHOSPHOR_LOGGING_LIBS) \
	-lstdc++fs	
