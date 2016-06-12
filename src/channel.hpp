#ifndef CHANNEL_HPP
#define CHANNEL_HPP
//#define BOOST_ALL_DYN_LINK
//#define DSO
#include"zmq.hpp"
#include "channel_base.hpp"
#include <semaphore.hpp>
#include <queue>
#include <map>
#include <chrono>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
namespace oi
{
    const int CHANNEL_SOCKET_LINGER_TIMEOUT_VALUE = 200;
    template<class T, class R>
        class msg_container
        {
            private:
                std::mutex _mutex;
                bool _is_done;
                std::condition_variable _cond_var;
            public:
                T* req;
                R* rsp;
                msg_container(T * t, R * r)
                {
                    req = t;
                    rsp = r;
                    _is_done = false;
                }

                void wait()
                {
                    std::unique_lock<std::mutex> lk(_mutex);
                    _cond_var.wait(lk, [this]{return this->_is_done;});
                }
                void done()
                {
                     std::lock_guard<std::mutex> lk(_mutex);
                    _is_done = true;
                    _cond_var.notify_one();
                }
        };



    template<class T, class R>
        class channel:public channel_base
    {
        private:
            oi::semaphore* _sem;
            std::mutex _mutex;
            std::vector<std::thread> _threads;
            std::queue<msg_container<T,R>*> _que;
            serializer _srz_tool;

            zmq::context_t* _context;
            std::string _ipc_path;
            int _rcv_timeout;
            int _snd_timeout;


            volatile bool _is_alive;

            void process_thread_function()
            {
                zmq::socket_t * sock = nullptr;
                try{
                    sock = new zmq::socket_t(*_context, ZMQ_REQ);
                    sock->setsockopt(ZMQ_LINGER, &CHANNEL_SOCKET_LINGER_TIMEOUT_VALUE, sizeof(CHANNEL_SOCKET_LINGER_TIMEOUT_VALUE));
                    sock->setsockopt(ZMQ_RCVTIMEO, &_rcv_timeout, sizeof(_rcv_timeout));
                    sock->setsockopt(ZMQ_SNDTIMEO, &_snd_timeout, sizeof(_snd_timeout));
                    sock->connect(_ipc_path.c_str());
                    zmq_msg_util util(_srz_tool);
                    timespec t1, t2, t3;//SHOULD BE REMOVED IF only get_ch_stat is used
                    uint64_t t_srz, t_total;
                    bool is_success = true;
                    std::chrono::duration<double> total ;
                    std::chrono::duration<double> srz_req;
                    std::chrono::duration<double> process;
                    std::chrono::duration<double> srz_rsp;
                    while(_is_alive)
                    {
                        zmq::message_t* req = nullptr;

                        bool wait_res = false;
                        while(_is_alive== true && wait_res == false)
                        {
                            wait_res = _sem->wait_for(100); //wait_res equal to false means timeout  . 100ms
                        }

                        if(!_is_alive)
                            break;
                        
                        msg_container<T,R> *msg = nullptr;
                        {
                            std::lock_guard<std::mutex> lk(_mutex);
                            msg = _que.front();
                            _que.pop();
                        }

                        clock_gettime(CLOCK_REALTIME, &t1);//SHOULD BE REMOVED IF only get_ch_stat is used

                        auto s1 = std::chrono::system_clock::now();
                        try
                        {
                            req = util.to_zmq_msg<T>(*(msg->req));
                            is_success = true;
                        }
                        catch(oi::exception & ox)
                        {
                            is_success = false;
                            ox.add_msg(__FILE__, __FUNCTION__, "unable to convert `%' to zmq::msg", typeid(T).name());
                            msg->rsp->set_exception(ox.what(), 0, exception_type_val::SRZ_REQ);
                        }
                        auto s2 = std::chrono::system_clock::now();

                        clock_gettime(CLOCK_REALTIME, &t2);//SHOULD BE REMOVED IF only get_ch_stat is used
                        t_srz = (t2.tv_sec - t1.tv_sec)*1000000.0 + (t2.tv_nsec - t1.tv_nsec)/1000.0;//SHOULD BE REMOVED IF only get_ch_stat is used

                        if(is_success == true)//request has been serialized successfully
                        {

                            try{
                                sock->send(*req);
                            }
                            catch(zmq::error_t & ex)
                            {
                                is_success = false;
                                msg->rsp->set_exception(std::string("unable to send message due to zmq::exception ") + ex.what(), 0, exception_type_val::ZMQ_SEND);
                            }
                            catch(std::exception & ex)
                            {
                                is_success = false;
                                msg->rsp->set_exception(std::string("unable to send message due to std::exception ") + ex.what(), 0, exception_type_val::ZMQ_SEND);
                            }
                            catch(...)
                            {
                                is_success = false;
                                msg->rsp->set_exception(std::string("unable to send message due to an unknown exception ") , 0, exception_type_val::ZMQ_SEND);
                            }

                            if(is_success == true)//the message has been sent successfully
                            {
                                zmq::message_t rsp;
                                try{
                                    sock->recv(&rsp);
                                }
                                catch(zmq::error_t & ex)
                                {
                                    is_success = false;
                                    msg->rsp->set_exception(std::string("unable to receive message due to zmq::exception ") + ex.what(), 0, exception_type_val::ZMQ_RCV);
                                }
                                catch(std::exception & ex)
                                {
                                    is_success = false;
                                    msg->rsp->set_exception(std::string("unable to receive message due to std::exception ") + ex.what(), 0, exception_type_val::ZMQ_RCV);
                                }
                                catch(...)
                                {
                                    is_success = false;
                                    msg->rsp->set_exception(std::string("unable to receive message due to an unknown exception ") , 0, exception_type_val::ZMQ_RCV);
                                }
                                auto s3 = std::chrono::system_clock::now();

                                if(is_success == true)
                                {
                                    clock_gettime(CLOCK_REALTIME, &t2);//SHOULD BE REMOVED IF only get_ch_stat is used
                                    try
                                    {
                                        util.to_data_msg<R>(rsp, *(msg->rsp));
                                    }
                                    catch(oi::exception & ox)
                                    {
                                        is_success = false;
                                        ox.add_msg(__FILE__, __FUNCTION__, "unable to convert  received data to `%'", typeid(R).name());
                                        msg->rsp->set_exception(ox.what(), 0, exception_type_val::SRZ_RSP);
                                    }

                                    clock_gettime(CLOCK_REALTIME, &t3);//SHOULD BE REMOVED IF only get_ch_stat is used
                                }
                                auto s4 = std::chrono::system_clock::now();

                                total   = s4 - s1;
                                srz_req = s2 - s1;
                                process = s3 - s2;
                                srz_rsp = s4 - s3;

                            }

                            if(is_success ==false && (msg->rsp->exception_type() == exception_type_val::ZMQ_SEND 
                                                        || 
                                                      msg->rsp->exception_type() == exception_type_val::ZMQ_RCV))
                            {
                                //LOGGGG
                                sock->close();
                                delete sock;
                                sock = new zmq::socket_t(*_context, ZMQ_REQ);
                                sock->setsockopt(ZMQ_LINGER, &CHANNEL_SOCKET_LINGER_TIMEOUT_VALUE, sizeof(CHANNEL_SOCKET_LINGER_TIMEOUT_VALUE));
                                sock->setsockopt(ZMQ_RCVTIMEO, &_rcv_timeout, sizeof(_rcv_timeout));
                                sock->setsockopt(ZMQ_SNDTIMEO, &_snd_timeout, sizeof(_snd_timeout));
                                sock->connect(_ipc_path.c_str());
                                _stat.update(0, 0, false);//SHOULD BE REMOVED IF only get_ch_stat is used
                                _ch_stat.update(0, 0, 0, 0, false);
                            }
                            else
                            {
                                t_srz += (t3.tv_sec - t2.tv_sec)*1000000.0 + (t3.tv_nsec - t2.tv_nsec)/1000.0;//SHOULD BE REMOVED IF only get_ch_stat is used
                                t_total = (t3.tv_sec - t1.tv_sec)*1000000.0 + (t3.tv_nsec - t1.tv_nsec)/1000.0;//SHOULD BE REMOVED IF only get_ch_stat is used
                                _stat.update(t_total, t_srz, true);//SHOULD BE REMOVED IF only get_ch_stat is used

                                _ch_stat.update(total.count() * 1000000.0,
                                        srz_req.count() * 1000000.0, 
                                        srz_rsp.count() * 1000000.0,
                                        process.count() * 1000000.0,
                                        true);
                            }
                        }
                        if(req != nullptr)
                        {
                            delete req;
                        }
                        msg->done();
                    }
                }
                catch(std::exception & e)
                {
                    if(_is_alive)
                    {
                        oi::exception oiex("std", "exception", e.what());
                        oiex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled std::exception in thread cancelation");
                        throw oiex;
                    }
                }
                catch (...)
                {
                    throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "Unhandled unknown exception.");
                }

                if(sock != nullptr)
                {
                    try
                    {
                        sock->close();
                        delete sock;
                    }
                    catch(...){}
                }
            }
        public:
            channel(const std::string &module,
                    const std::string &method,
                    const std::string & ipc, 
                    zmq::context_t * cntx, 
                    serializer srz, 
                    int snd_timeout, 
                    int rcv_timeout):channel_base(module, method)
        {
            _is_alive = false;
            _ipc_path = ipc;
            _context = cntx;
            _sem = nullptr;
            _srz_tool = srz;
            _snd_timeout = snd_timeout;
            _rcv_timeout = rcv_timeout;
        }

            ~channel()
            {

                if(_is_alive == true)
                {
                    close();
                }
                //TODO joining ....
                if(_sem != nullptr)
                    delete _sem;
            }
            void init(int thread_count)throw (oi::exception)
            {
                _is_alive = true;
                try{
                    _sem = new oi::semaphore(0);
                    for(int i=0; i< thread_count; i++)
                    {
                        _threads.emplace_back(std::bind(&channel::process_thread_function, this));
                    }
                }
                catch(std::exception& ex)
                {
                    oi::exception ox("std", "exception", ex.what());
                    ox.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled std::exception");
                    throw ox;
                }
                catch(...)
                {
                    throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "Unhandled unknown exception.");
                }
            }

            void process(void * t, void * r)throw(oi::exception)
            {
                if(_is_alive == false)
                {
                    throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "invalid use of uninitialized channel. call init() before usage");
                }
                try
                {
                    msg_container<T, R> msgc(static_cast<T*>(t),static_cast<R*>(r));

                    {
                        std::lock_guard<std::mutex> lk(_mutex);
                        _que.push(&msgc);
                    }
                    _sem->notify();

                    msgc.wait();
                }
                catch(std::exception& ex)
                {
                    oi::exception ox("std", "exception", ex.what());
                    ox.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled std::exception");
                    throw ox;
                }
                catch(...)
                {
                    throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "Unhandled unknown exception.");
                }
            }

            void close()throw()
            {
                try{
                    if(_is_alive == true)
                    {
                        _is_alive = false;
                        for(auto & th: _threads)
                        {
                            th.join();
                        }
                    }
                }
                catch(...)
                {

                }
            }


    };
}
#endif
