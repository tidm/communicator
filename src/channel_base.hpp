#ifndef CHANNEL_BASE_H
#define CHANNEL_BASE_H

#include"channel_stat.hpp"
#include"cm_stat.hpp"
#include"zmq_msg_util.hpp"
namespace oi
{
    class channel_base
    {
        
        protected:
            channel_base(const std::string & module, const std::string& method);
            channel_stat _stat;
            transmission_stat _ch_stat;
            std::string _module;
            std::string _method;
        public:
            virtual ~channel_base();
            virtual void process(void* t, void* r) = 0;
            virtual void close() = 0;
            std::string get_module()const;
            std::string get_method()const;
            cm_stat get_stat()throw(oi::exception);
            cm_info get_ch_stat()throw(oi::exception);
    };
}
#endif
