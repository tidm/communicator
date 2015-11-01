#include "comm_stubs.hpp"
BOOST_CLASS_EXPORT(oi::com_object)
namespace oi
{

    com_object::com_object():__msg(""), 
                             __error_code(0), 
                             __exception_type(exception_type_val::NO_EXCEPT)
    {

    }

//    void com_object::set_exception(const char * msg)throw()
//    {
//        if(msg != NULL)
//        {
//            __msg = std::string(msg);
//        }
//        __exception = true;
//    }

    void com_object::set_exception(const std::string& msg, int error_code, except_type type)throw()
    {
        __msg = msg;
        __error_code = error_code;
        __exception_type = type;
    }

    bool com_object::exception_flag() throw()
    {
        if(__exception_type ==  exception_type_val::NO_EXCEPT)
            return false;
        else
            return true;
    }

    std::string com_object::exception_msg()throw()
    {
        return __msg;
    }

    except_type com_object::exception_type()throw()
    {
        return __exception_type;
    }

    int com_object::error_code()throw()
    {
        return __error_code;
    }

    com_object::~com_object()
    {

    }


    dummy_msg::dummy_msg():com_object()
    {
    }

}
