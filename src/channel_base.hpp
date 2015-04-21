#ifndef CHANNEL_BASE_H
#define CHANNEL_BASE_H

#include"channel_stat.hpp"
#include"zmq_msg_util.hpp"
namespace oi
{
    class channel_base
    {
        
        protected:
            channel_base();
            channel_stat _stat;
        public:
            virtual ~channel_base();
            virtual void process(void* t, void* r) = 0;
            virtual void close() = 0;
            cm_stat get_stat()throw(oi::exception);
    };
}
#endif
