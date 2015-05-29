#include "channel_base.hpp"
namespace oi
{
    channel_base::channel_base(const std::string & module, const std::string& method)
    {
        _method = method;
        _module = module;
    }

    channel_base::~channel_base()
    {
    }

    std::string channel_base::get_module()const
    {
        return _module;
    }
    std::string channel_base::get_method()const
    {
        return _method;
    }
    cm_stat channel_base::get_stat()throw(oi::exception)
    {
        cm_stat cs;
        try
        {
            cs = _stat.get_stat();
        }
        catch (oi::exception& e)
        {
            e.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception");
            throw e;
        }
        return cs;
    }
    cm_info channel_base::get_ch_stat()throw(oi::exception)
    {
        cm_info cs;
        try
        {
            cs = _ch_stat.get_stat();
        }
        catch (oi::exception& e)
        {
            e.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception");
            throw e;
        }
        return cs;
    }
}
