ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
AM_CFLAGS = --pedantic
AM_CXXFLAGS = --pedantic
AM_CPPFLAGS = -I../src/ \
    $(OI_EXCEPTION_CFLAGS) \
	$(OI_COMMON_CFLAGS) \
    $(ZMQ_CFLAGS) \
    $(BOOST_SERIALIZATION_CFLAGS) \
    $(BOOST_SYSTEM_CFLAGS)


LDADD=../src/.libs/liboi_communicator.a \
    -lmsgpack \
	$(OI_EXCEPTION_LIBS) \
	$(OI_COMMON_LIBS) \
    $(ZMQ_LIBS) \
	$(BOOST_SERIALIZATION_LIBS) \
    $(BOOST_SYSTEM_LIBS) 
check_PROGRAMS = client server 
check_SCRIPTS = run_client_server.sh
server_SOURCES = server.cpp container.cpp parent.cpp child.cpp
client_SOURCES = client.cpp container.cpp  parent.cpp child.cpp 
TESTS = $(check_SCRIPTS)



