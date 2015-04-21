#ifndef CHANNEL_STAT_HPP
#define CHANNEL_STAT_HPP
#include<iostream>
#include<iomanip>
#include<boost/thread/mutex.hpp>
#include<exception.hpp>
namespace oi
{
    struct cm_stat
    {
        double avg_total;
        double avg_srz;
        double std_total;
        double std_srz;
        double success;
        double failed;
        cm_stat();
    };
    std::ostream& operator<<(std::ostream & os, const cm_stat& s);

    class channel_stat 
    {
        private:
            uint64_t _success;
            uint64_t _failed;
            long double _sum_time_total;
            long double _sum_time2_total;
            long double _sum_time_srz;
            long double _sum_time2_srz;
            bool is_active;
            boost::mutex _lock; 
        public:
            channel_stat();
            void update(uint64_t t_total, uint64_t t_srz, bool success) throw(oi::exception);//micro second
            cm_stat get_stat()throw(oi::exception);
    };
}

#endif
