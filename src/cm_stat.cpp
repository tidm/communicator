#include"cm_stat.hpp"
#include <algorithm>
namespace oi
{
    stat::stat()
    {
        reset();
    }
    void stat::reset()
    {
        avg = 0;
        min = 0;
        max = 0;
        std = 0;
        sum = 0;
        sum_2 = 0;
    }
    void stat::set(uint64_t val)
    {
        if(min == 0)
        {
            min = val;
        }
        else
        {
            min = std::min(val, min);
        }
        max = std::max(val, max);
        sum += val;
        sum_2 += val * val;
    }

    std::ostream& operator<<(std::ostream & os, const stat & s)
    {
        os << std::fixed << std::setprecision(2) 
            << " avg:" << s.avg    
            << " min:" << s.min 
            << " max:" << s.max 
            << " std:" << s.std 
            << " sum:" << s.sum 
            << " sum_2:" << s.sum_2;
            return os;
    }

    cm_info::cm_info()
    {
        reset();
    }

    void cm_info::reset()
    {
        total.reset();
        srz_req.reset();
        srz_rsp.reset();
        process.reset();
        success = 0;
        failed = 0;
    }

    std::ostream& operator<<(std::ostream & os, const cm_info& s)
    {
        os << std::fixed << std::setprecision(2) 
            << "\n\tsuccess:" << s.success    
            << "\n\tfailed:" << s.failed
            << "\n\ttotal:{" << s.total << "}"
            << "\n\treq srz:{" << s.srz_req  << "}"
            << "\n\trsp srz:{" << s.srz_rsp << "}"
            << "\n\tprocess:{" << s.process << "}";
            return os;
    }

    transmission_stat::transmission_stat()
    {
        _is_active  = false;
    }

    void transmission_stat::update(uint64_t t_total, 
                                   uint64_t t_srz_req, 
                                   uint64_t t_srz_rsp, 
                                   uint64_t t_process,
                                   bool success) throw(oi::exception)//micro second
    {
        if(_is_active)
        {
            try
            {
                _lock.lock();
                {
                    if(success)
                    {
                       _info.success ++;
                       _info.total.set(t_total);
                       _info.srz_req.set(t_srz_req);
                       _info.srz_rsp.set(t_srz_rsp);
                       _info.process.set(t_process);
                    }
                    else
                    {
                        _info.failed++;
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
    cm_info transmission_stat::get_stat()throw(oi::exception)
    {
        _is_active = true;
        cm_info c;
        try
        {
            _lock.lock();
            {
                c = _info;
                _info.reset();
            }
            _lock.unlock();
            if(c.success > 0)
            {
                c.total.avg   = c.total.sum / c.success ;
                c.srz_req.avg = c.srz_req.sum / c.success ;
                c.srz_rsp.avg = c.srz_rsp.sum / c.success ;
                c.process.avg = c.process.sum / c.success ;

                c.total.std   = sqrt(c.total.sum_2 /   c.success - c.total.avg   * c.total.avg);
                c.srz_req.std = sqrt(c.srz_req.sum_2 / c.success - c.srz_req.avg * c.srz_req.avg);
                c.srz_rsp.std = sqrt(c.srz_rsp.sum_2 / c.success - c.srz_rsp.avg * c.srz_rsp.avg);
                c.process.std = sqrt(c.process.sum_2 / c.success - c.process.avg * c.process.avg);
            }
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
