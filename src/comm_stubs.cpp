#include "comm_stubs.hpp"
BOOST_CLASS_EXPORT(oi::com_object)
oi::com_object::com_object():__msg(""),
    __error_code(0),
    __exception_type(exception_type_val::NO_EXCEPT) {
}

//    void com_object::set_exception(const char * msg)throw()
//    {
//        if(msg != NULL)
//        {
//            __msg = std::string(msg);
//        }
//        __exception = true;
//    }

void oi::com_object::set_exception(const std::string& msg, int error_code, except_type type)throw() {
    __msg = msg;
    __error_code = error_code;
    __exception_type = type;
}

bool oi::com_object::exception_flag() throw() {
    if(__exception_type ==  exception_type_val::NO_EXCEPT) {
        return false;
    }
    else {
        return true;
    }
}

std::string oi::com_object::exception_msg()throw() {
    return __msg;
}

oi::except_type oi::com_object::exception_type()throw() {
    return __exception_type;
}

int oi::com_object::error_code()throw() {
    return __error_code;
}

oi::com_object::~com_object() {
}


oi::dummy_msg::dummy_msg():oi::com_object() {
}
