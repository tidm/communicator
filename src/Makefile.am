ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
AM_CFLAGS = --pedantic
AM_CXXFLAGS = --pedantic
AM_CPPFLAGS = \
    -Wall \
    $(OI_EXCEPTION_CFLAGS) \
	$(OI_COMMON_CFLAGS) \
    $(ZMQ_CFLAGS) \
    $(BOOST_SERIALIZATION_CFLAGS) \
    $(BOOST_THREAD_CFLAGS) \
    $(BOOST_SYSTEM_CFLAGS)
lib_LTLIBRARIES = liboi_communicator.la
liboi_communicator_la_SOURCES = \
    channel_base.cpp \
    channel_stat.cpp \
    comm_stubs.cpp \
    communicator.cpp \
    service_stubs.cpp \
    zmq_msg_util.cpp

liboi_communicator_la_LDFLAGS = -version-info $(OI_COMMUNICATOR_SO_VERSION)
liboi_communicator_la_LIBADD = \
    -lmsgpack \
	$(OI_EXCEPTION_LIBS) \
	$(OI_COMMON_LIBS) \
    $(ZMQ_LIBS) \
	$(BOOST_SERIALIZATION_LIBS) \
    $(BOOST_THREAD_LIBS) \
    $(BOOST_SYSTEM_LIBS)

oi_communicator_includedir = $(includedir)/oi/utils/communicator
oi_communicator_include_HEADERS = \
    channel_base.hpp \
    channel.hpp \
    channel_stat.hpp \
    comm_stubs.hpp \
    communicator.hpp \
    com_type.hpp \
    service_stubs.hpp \
    zmq_msg_util.hpp \
    zmq.hpp

