#include "channel_base.hpp"
namespace oi
{
    channel_base::channel_base()
    {
    }

    channel_base::~channel_base()
    {
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
}
