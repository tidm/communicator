#ifndef CM_STAT_HPP
#define CM_STAT_HPP
#include<iostream>
#include<iomanip>
#include<mutex>
#include<exception.hpp>
namespace oi {
struct stat {
    double avg;
    uint64_t min;
    uint64_t max;
    double std;
    long double sum;
    long double sum_2;
    stat();
    void reset();
    void set(uint64_t val);
};
std::ostream& operator<<(std::ostream& os, const stat& s);

struct cm_info {
    stat total;
    stat srz_req;
    stat srz_rsp;
    stat process;
    uint64_t success;
    uint64_t failed;
    cm_info();
    void reset();
};
std::ostream& operator<<(std::ostream& os, const cm_info& s);

class transmission_stat {
  private:
    cm_info _info;
    bool    _is_active;
    std::mutex _lock;
  public:
    transmission_stat();
    void update(uint64_t t_total,
                uint64_t t_srz_req,
                uint64_t t_srz_rsp,
                uint64_t t_process,
                bool success) throw(oi::exception);//micro second
    cm_info get_stat()throw(oi::exception);
};
}

#endif
