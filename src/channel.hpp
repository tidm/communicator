#ifndef CHANNEL_HPP
#define CHANNEL_HPP
//#define BOOST_ALL_DYN_LINK
//#define DSO
#include"zmq.hpp"
#include<boost/interprocess/sync/interprocess_semaphore.hpp>
#include<boost/asio.hpp>
#include<boost/thread/mutex.hpp>
#include<boost/thread/thread.hpp>
#include<boost/function.hpp>
#include<queue>
#include<map>
#include "channel_base.hpp"
#include <chrono>
namespace oi
{
    const int CHANNEL_SOCKET_LINGER_TIMEOUT_VALUE = 200;
    template<class T, class R>
        class msg_container
        {
            private:
                boost::mutex _mutex;
                bool _is_done;
                boost::condition_variable _cond_var;
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
                    boost::mutex::scoped_lock lock(_mutex);
                    while (!_is_done)
                    {
                        _cond_var.wait(lock);
                    }
                }
                void done()
                {
                    _is_done = true;
                    _cond_var.notify_one();
                }
        };



    template<class T, class R>
        class channel:public channel_base
    {
        private:
            boost::interprocess::interprocess_semaphore* _sem;
            boost::mutex _mutex;
            boost::thread* _thread;
            std::queue<msg_container<T,R>*> _que;
            serializer _srz_tool;

            zmq::context_t* _context;
            std::string _ipc_path;
            int _rcv_timeout;
            int _snd_timeout;


            volatile bool _is_alive;

            void process_thread_function()
            {
                zmq::socket_t * sock = NULL;
                try{
                    sock = new zmq::socket_t(*_context, ZMQ_REQ);
                    sock->setsockopt(ZMQ_LINGER, &CHANNEL_SOCKET_LINGER_TIMEOUT_VALUE, sizeof(CHANNEL_SOCKET_LINGER_TIMEOUT_VALUE));
                    sock->setsockopt(ZMQ_RCVTIMEO, &_rcv_timeout, sizeof(_rcv_timeout));
                    sock->setsockopt(ZMQ_SNDTIMEO, &_snd_timeout, sizeof(_snd_timeout));
                    sock->connect(_ipc_path.c_str());
                    zmq_msg_util util(_srz_tool);
                    timespec t1, t2, t3;//SHOULD BE REMOVED IF only get_ch_stat is used
                    uint64_t t_srz, t_total;
                    bool is_sent = false;
                    bool is_recvd = false;
                    std::chrono::duration<double> total ;
                    std::chrono::duration<double> srz_req;
                    std::chrono::duration<double> process;
                    std::chrono::duration<double> srz_rsp;
                    while(_is_alive)
                    {
                        zmq::message_t* req;

                        _sem->wait();

                        if(!_is_alive)
                            break;
                        
                        msg_container<T,R> *msg = NULL;
                        _mutex.lock();
                        {
                            msg = _que.front();
                            _que.pop();
                        }
                        _mutex.unlock();

                        clock_gettime(CLOCK_REALTIME, &t1);//SHOULD BE REMOVED IF only get_ch_stat is used

                        auto s1 = std::chrono::system_clock::now();
                        req = util.to_zmq_msg<T>(*(msg->req));
                        auto s2 = std::chrono::system_clock::now();
                        
                        clock_gettime(CLOCK_REALTIME, &t2);//SHOULD BE REMOVED IF only get_ch_stat is used
                        t_srz = (t2.tv_sec - t1.tv_sec)*1000000.0 + (t2.tv_nsec - t1.tv_nsec)/1000.0;//SHOULD BE REMOVED IF only get_ch_stat is used

                        is_sent = false;
                        is_recvd = false;

                        try{
                            sock->send(*req);
                            is_sent = true;
                        }
                        catch(zmq::error_t & ex)
                        {
                            msg->rsp->set_exception(std::string("unable to send message due to zmq::exception ") + ex.what());
                        }
                        catch(std::exception & ex)
                        {
                            msg->rsp->set_exception(std::string("unable to send message due to std::exception ") + ex.what());
                        }
                        catch(...)
                        {
                            msg->rsp->set_exception(std::string("unable to send message due to unknown exception ") );
                        }

                        
                        if(is_sent)
                        {
                            zmq::message_t rsp;
                            try{
                                sock->recv(&rsp);
                                auto s3 = std::chrono::system_clock::now();
                                is_recvd = true;
                                clock_gettime(CLOCK_REALTIME, &t2);//SHOULD BE REMOVED IF only get_ch_stat is used
                                util.to_data_msg<R>(rsp, *(msg->rsp));
                                clock_gettime(CLOCK_REALTIME, &t3);//SHOULD BE REMOVED IF only get_ch_stat is used
                                auto s4 = std::chrono::system_clock::now();
                                
                                total   = s4 - s1;
                                srz_req = s2 - s1;
                                process = s3 - s2;
                                srz_rsp = s4 - s3;
                            }
                            catch(zmq::error_t & ex)
                            {
                                msg->rsp->set_exception(std::string("unable to receive message due to zmq::exception ") + ex.what());
                            }
                            catch(std::exception & ex)
                            {
                                msg->rsp->set_exception(std::string("unable to receive message due to std::exception ") + ex.what());
                            }
                            catch(...)
                            {
                                msg->rsp->set_exception("unable to receive message due to unknown exception ");
                            }

                        }

                        if(!is_sent || !is_recvd)
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
                        delete req;

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

                if(sock != NULL)
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
                _sem = NULL;
                _thread = NULL;
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
                if(_thread != NULL)
                    delete _thread;
                if(_sem != NULL)
                    delete _sem;
            }
            void init()throw (oi::exception)
            {
                _is_alive = true;
                try{
                    _sem = new boost::interprocess::interprocess_semaphore(0);
                    _thread  =new boost::thread(boost::bind(&channel::process_thread_function, this));
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

                    _mutex.lock();
                    _que.push(&msgc);
                    _mutex.unlock();

                    _sem->post();

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
                        _sem->post();
                        _thread->join();
                    }
                }
                catch(...)
                {
                    
                }
            }


    };
}
#endif
