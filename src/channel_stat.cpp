#include"channel_stat.hpp"
namespace oi
{
    cm_stat::cm_stat()
    {
        avg_total=0;
        avg_srz=0;
        std_total=0;
        std_srz=0;
        success=0;
        failed=0;
    }
    std::ostream& operator<<(std::ostream & os, const cm_stat& s)
    {
        os << std::fixed << std::setprecision(2) <<  "  success:"<< s.success <<  " failed:" << s.failed << 
            " total:("<< s.avg_total << "us, " << s.std_total << "us) "
            " srz:("<< s.avg_srz << "us, " << s.std_srz << "us) ";
    }
    channel_stat::channel_stat()
    {
        is_active = false;
        _success = 0;
        _failed = 0;
        _sum_time_total = 0;
        _sum_time2_total = 0;            
        _sum_time_srz = 0;
        _sum_time2_srz = 0;
    }
    void channel_stat::update(uint64_t t_total, uint64_t t_srz, bool success) throw(oi::exception)//micro second
    {
        if(is_active)
        {
            try
            {
                _lock.lock();
                {
                    if(success)
                    {
                        _success++;
                        _sum_time_total += t_total;
                        _sum_time2_total += t_total * t_total;
                        _sum_time_srz += t_srz;
                        _sum_time2_srz += t_srz * t_srz;
                    }
                    else
                    {
                        _failed++;
                    }
                }
                _lock.unlock();
            }
            catch(std::exception & e)
            {
                oi::exception oiex("std", "exception", e.what());
                oiex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled std::exception");
                throw oiex;
            }
            catch(...)
            {
                throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "Unhandled unknown exception.");
            }

        }
    }
    cm_stat channel_stat::get_stat()throw(oi::exception)
    {
        is_active = true;
        cm_stat c;
        try
        {
            _lock.lock();
            {
                c.success = _success;
                c.failed  = _failed;
                if(_success > 0)
                {
                    c.avg_total = _sum_time_total/_success;
                    c.avg_srz = _sum_time_srz /_success ;
                    c.std_total = sqrt(_sum_time2_total/_success - c.avg_total * c.avg_total);
                    c.std_srz= sqrt(_sum_time2_srz / _success - c.avg_srz * c.avg_srz);
                }
                else
                {
                    c.avg_total = 0;
                    c.avg_srz = 0;
                    c.std_total = 0;
                    c.std_srz= 0;
                }
                _success = 0;
                _failed = 0;
                _sum_time_total = 0;
                _sum_time2_total = 0;            
                _sum_time_srz = 0;
                _sum_time2_srz = 0;
            }
            _lock.unlock();
        }
        catch(std::exception & e)
        {
            oi::exception oiex("std", "exception", e.what());
            oiex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled std::exception");
            throw oiex;
        }
        catch(...)
        {
            throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "Unhandled unknown exception.");
        }
        return c;
    }
}
