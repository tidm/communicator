#ifndef COMM_STUBS_HPP
#define COMM_STUBS_HPP

//#define BOOST_ALL_DYN_LINK
//#define DSO
#include<boost/serialization/base_object.hpp>
#include<boost/serialization/export.hpp>
#include<boost/serialization/extended_type_info.hpp>
#include<boost/any.hpp>
#include"zmq_msg_util.hpp"

#define OI_MSGPACK_DEFINE(...) MSGPACK_DEFINE(__msg, __error_code, __exception_type, __VA_ARGS__)
namespace oi {
typedef uint8_t except_type;
class communicator;
class com_object {

    friend class communicator;
  protected:
    std::string __msg;
    int __error_code;
    except_type __exception_type;
  public:
    MSGPACK_DEFINE(__msg, __error_code, __exception_type)
    template<class T>
    void serialize(T& ar, const unsigned int version) {
        ar& __msg;
        ar& __error_code;
        ar& __exception_type;
    }
    com_object();
    //        void set_exception(const char * msg, int error_code, exception_type type )throw();
    void set_exception(const std::string& msg, int error_code, except_type type)throw();
    bool exception_flag() throw();
    std::string exception_msg()throw();
    except_type exception_type()throw();
    int error_code()throw();

    virtual ~com_object();
};

class dummy_msg: public com_object {
    friend class boost::serialization::access;
  public:
    dummy_msg();
  private:
    template<class T>
    void serialize(T& ar, const unsigned int version) {
        ar& boost::serialization::base_object<com_object>(*this);
    }
};
}

#endif
