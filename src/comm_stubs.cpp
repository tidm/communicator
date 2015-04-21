#include "comm_stubs.hpp"
BOOST_CLASS_EXPORT(oi::com_object)
namespace oi
{

    com_object::com_object():__exception(false), __msg("")
    {

    }

    void com_object::set_exception(const char * msg)throw()
    {
        if(msg != NULL)
        {
            __msg = std::string(msg);
        }
        __exception = true;
    }

    void com_object::set_exception(const std::string& msg)throw()
    {
        __msg = msg;
        __exception = true;
    }

    bool com_object::exception_flag() throw()
    {
        return __exception;
    }

    std::string com_object::exception_msg()throw()
    {
        return __msg;
    }

    com_object::~com_object()
    {

    }


    dummy_msg::dummy_msg():com_object()
    {
    }

}
