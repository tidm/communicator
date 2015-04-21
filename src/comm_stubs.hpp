#ifndef COMM_STUBS_HPP
#define COMM_STUBS_HPP

//#define BOOST_ALL_DYN_LINK
//#define DSO
#include<boost/serialization/base_object.hpp>
#include<boost/serialization/export.hpp>
#include<boost/serialization/extended_type_info.hpp>
#include<boost/any.hpp>
#include"zmq_msg_util.hpp"

#define OI_MSGPACK_DEFINE(...) MSGPACK_DEFINE(__exception, __msg, __VA_ARGS__)
namespace oi
{
    class communicator;
    class com_object
    {
        friend class communicator;
        protected:
        bool __exception;
        std::string __msg;
        public:
        MSGPACK_DEFINE(__exception, __msg)
        template<class T>
            void serialize(T & ar, const unsigned int version)
        {
            ar & __exception;
            ar & __msg;
        }
        com_object();
        void set_exception(const char * msg)throw();
        void set_exception(const std::string& msg)throw();
        bool exception_flag() throw();
        std::string exception_msg()throw();
        virtual ~com_object();
    };

    class dummy_msg: public com_object
    {
        friend class boost::serialization::access;
        public:
        dummy_msg();
        private:
        template<class T>
            void serialize(T & ar, const unsigned int version)
            {
                ar & boost::serialization::base_object<com_object>(*this);
            }
    };
}

#endif
