#include "channel_base.hpp"
oi::channel_base::channel_base(const std::string& module, const std::string& method) {
    _method = method;
    _module = module;
}

oi::channel_base::~channel_base() {
}

std::string oi::channel_base::get_module()const {
    return _module;
}
std::string oi::channel_base::get_method()const {
    return _method;
}
oi::cm_stat oi::channel_base::get_stat()throw(oi::exception) {
    oi::cm_stat cs;
    try {
        cs = _stat.get_stat();
    }
    catch (oi::exception& e) {
        e.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception");
        throw e;
    }
    return cs;
}
oi::cm_info oi::channel_base::get_ch_stat()throw(oi::exception) {
    oi::cm_info cs;
    try {
        cs = _ch_stat.get_stat();
    }
    catch (oi::exception& e) {
        e.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception");
        throw e;
    }
    return cs;
}
