ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
AM_CFLAGS = --pedantic
AM_CXXFLAGS = --pedantic
AM_CPPFLAGS = -I../src/  -DDSO -DBOOST_ALL_DYN_LINK


LDADD=../src/.libs/liboi_communicator.a \
    -lmsgpack \
	$(OI_EXCEPTION_LIBS) \
    $(ZMQ_LIBS) \
	$(BOOST_SERIALIZATION_LIBS) \
    $(BOOST_THREAD_LIBS) \
    $(BOOST_SYSTEM_LIBS) 
check_PROGRAMS = client server 
check_SCRIPTS = run_client_server.sh
server_SOURCES = server.cpp
client_SOURCES = client.cpp
TESTS = $(check_SCRIPTS)



