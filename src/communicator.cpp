#include "communicator.hpp"
#include<config.h>
//    uint64_t communicator::get_trace_no()
//    {
//        std::lock_guard<std::mutex> m(_trace_no_lock);
//        _trace_no++;
//        return _trace_no;
//    }
//    std::ostream& oi::operator<<(std::ostream& os, const oi::oi_err& o)
//    {
//        switch(o)
//        {
//            case oi::OI_SUCCESS:
//                os << " SUCCESS";
//                break;
//            case oi::OI_ERROR:
//                os << "ERROR";
//                break;
//            default:
//                os << "Unknown error!";
//                break;
//        };
//        return os;
//    }

    oi::service_info oi::communicator::get_service_list(const std::string& module, bool use_cache)
    {

        oi::dummy_msg d;
        oi::service_info srv;
        bool sw = false;
        if(use_cache == true)
        {
            oi::shared_lock<oi::shared_mutex> lk{_dst_setvice_list_guard};
            std::map<std::string, oi::service_info>::iterator it = _dst_service_list.begin();
            it = _dst_service_list.find(module);
            if(it != _dst_service_list.end())
            {
                srv = it->second;
                sw = true;
            }
        }
        if(sw == false)
        {
            request<oi::dummy_msg, oi::service_info>(module, SERVICE_INFO_METHOD_NAME , d, srv, CHANNEL_SOCKET_SEND_TIMEOUT, CHANNEL_SOCKET_RECV_TIMEOUT, 1);
            {
                std::lock_guard<oi::shared_mutex> lk{_dst_setvice_list_guard};
                _dst_service_list[module] = srv; 
            }
        }
        return srv;
    }

    oi::communicator::communicator()throw()
        :_context(ZMQ_CONTEXT_IO_THREADS)
    {
        _state = NEW;
        _name = "";
        _ipc_file_path = IPC_FILE_PATH;
//_trace_no = 0;
    }

    std::map<std::string, oi::cm_info> oi::communicator::get_service_stat()throw(oi::exception)
    {
        std::map<std::string, oi::cm_info> stat;
//        std::map<std::string, oi::transmission_stat*>::iterator it;

        oi::service_sign sgn ;

        _service_stat_list_guard.lock();
        try
        {
            //for(it = _service_stat_list.begin(); it != _service_stat_list.end(); it++)
            for(const auto & it : _service_stat_list)
            {
                {
                    oi::shared_lock<oi::shared_mutex> lk{_service_info_guard};
                    sgn = _service_info.get(it.first);
                }
                stat[sgn.module + ":" + sgn.method] = it.second->get_stat();
            }
        }
        catch(std::exception& ex)
        {
            oi::exception ox("std", "exception", ex.what());
            ox.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled std::exception");
            _service_stat_list_guard.unlock();
            throw ox;
        }
        catch(...)
        {
            _service_stat_list_guard.unlock();
            throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "Unhandled unknown exception.");
        }
        _service_stat_list_guard.unlock();
        return stat;
    }

    std::map<std::string, oi::cm_info> oi::communicator::get_interface_stat()throw(oi::exception)
    {
        
//std::string ss = _tracer.get_stat(std::chrono::seconds(5));
//if(!ss.empty())
//{
//    std::cerr << ss << std::endl;            
//}

        std::map<std::string, oi::cm_info> stat;
        std::string module;
        std::string method;
        _channel_map_mutex.lock();
        {
            try
            {
              //  std::map<std::string, oi::channel_base*>::iterator it;
                //for(it = _channel_map.begin();it != _channel_map.end(); it++)
                for(const auto& it  : _channel_map)
                {
                    module = it.second->get_module();
                    method = it.second->get_method();

                    stat[module + ":" + method] = it.second->get_ch_stat();
                }
            }
            catch(std::exception& ex)
            {
                oi::exception ox("std", "exception", ex.what());
                ox.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled std::exception");
                _channel_map_mutex.unlock();
                throw ox;
            }
            catch(...)
            {
                _channel_map_mutex.unlock();
                throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "Unhandled unknown exception.");
            }
        }
        _channel_map_mutex.unlock();
        return stat;
    }
    std::map<std::string, oi::cm_stat> oi::communicator::get_channel_stat()throw(oi::exception)
    {
        std::map<std::string, oi::cm_stat> stat;

        _channel_map_mutex.lock();
        {
            try
            {
               // std::map<std::string, oi::channel_base*>::iterator it;
                //for(it = _channel_map.begin();it != _channel_map.end(); it++)
                for(const auto& it : _channel_map)
                {
                    stat[it.first] = it.second->get_stat();
                }
            }
            catch(std::exception& ex)
            {
                oi::exception ox("std", "exception", ex.what());
                ox.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled std::exception");
                _channel_map_mutex.unlock();
                throw ox;
            }
            catch(...)
            {
                _channel_map_mutex.unlock();
                throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "Unhandled unknown exception.");
            }
        }
        _channel_map_mutex.unlock();
        return stat;
    }

    void oi::communicator::wait()throw(oi::exception)
    {
        if(_state == NEW)
        {
            throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "use of un-initialized communicator! call communicator::initialize before invokation of other methods");
        }
        while(_state == READY)
        {
            usleep(200000);
        }
    }
    bool oi::communicator::is_remote_ready(const std::string & remote_module, const std::string & method_name)throw(oi::exception)
    {
        bool is_ready = false;
        if(_state != oi::communicator::state::READY)
        {
            throw oi::exception(__FILE__, __FUNCTION__, "invalid use of `%' communicator object", _state);
        }

        oi::service_info srv; 
        try{
            srv = get_service_list(remote_module, false);
            std::set<std::string> lst= srv.get_methods();
            if(lst.find(method_name) != lst.end())
            {
                is_ready = true;
            }
        }
        catch(...)
        {
            is_ready  = false;
        }

        //        oi::get_interface<oi::com_type<int> > m_if;
        //
        //        try
        //        {
        //            m_if = create_get_interface<oi::com_type<int> >(remote_module, GET_STATE_METHOD_NAME , 50, 50);
        //            int res = m_if.call();
        //            if(static_cast<state>(res) == REGISTERED)
        //            {
        //                is_ready = true;
        //            }
        //        }
        //        catch(...)
        //        {
        //            is_ready = false;
        //        }
        return is_ready;
    }


    void oi::communicator::initialize(const std::string &me, const std::function<void(const oi::exception&)> & exception_handler)throw(oi::exception)
    {
        if(_state != NEW)
        {
            throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "multiple 'init' call!");
        }
        if(me.empty())
        {
            throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "invalid module name : '%'", me );
        }
        _name = me;
        _exception_handler = exception_handler;

        _state = READY;
        try
        {
            std::function<oi::service_info(void)> f = std::bind(&oi::communicator::get_service_info<oi::service_info>, this);
            register_callback<oi::service_info>(f, SERVICE_INFO_METHOD_NAME, 1, SRZ_MSGPACK);
        }
        catch(oi::exception& ex)
        {
            ex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception registering service_info servive");
            throw ex;
        }
        catch(std::exception& ex)
        {
            oi::exception ox("std", "exception", ex.what());
            ox.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled std::exception registering service_info servive");
            throw ox;
        }
        catch(...)
        {
            throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "Unhandled unknown exception registering service_info servive.");
        }
    }

    void oi::communicator::shutdown()throw()
    {
        _state = SIGNALED;
    }

    void oi::communicator::finalize()throw()
    {
        try
        {
            if(_state == TERMINATED)
            {
                return;
            }
            if(_state == READY || _state == SIGNALED)
            {
                _state = SIGNALED;
                usleep(500000);
                oi::channel_base * c;
                {
                    std::lock_guard<std::mutex> m{_channel_map_mutex};
                  //  std::map<std::string, oi::channel_base*>::iterator it;
                    //for(it = _channel_map.begin();it != _channel_map.end(); it++)
                    for(const auto & it : _channel_map)
                    {
                        c = it.second;
                        if(c != nullptr)
                        {
                            c->close();
                            delete c;
                        }
                    }
                }
            }

            _context.close();

            for(auto & th : _worker_thread_list)
            {
                th.join();
            }
            for(auto & th : _proxy_thread_list)
            {
                th.join();
            }
        }
        catch(...)
        {}
        // _proxy_thread_list.push_back(th);
        //_clients.push_back(client);
        //_workers.push_back(worker);
        //  _worker_thread_list.push_back(th);
        _state = TERMINATED;
    }

    oi::communicator::~communicator()throw()
    {
        try
        {
            finalize();
        }
        catch(...)
        {
        }
        ////////CANCEL Service  ALL THREADS
    }

    oi::sig_interface oi::communicator::create_sig_interface(const std::string &module,
            const std::string &method, 
            int snd_timeout , 
            int rcv_timeout ,
            int thread_count

            )throw (oi::exception)
    {
        oi::sig_interface f;
        try
        {
            f.initialize(module, method, this, snd_timeout, rcv_timeout, thread_count);
        }
        catch(oi::exception& ex)
        {
            ex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception. unable to initialize req_interface");
            throw ex;
        }
        return f;
    }

    void oi::communicator::register_callback(std::function<void(void)> f,
            const std::string &mth,
            int parallel,
            oi::serializer srz
            )throw(oi::exception)//NOT THREAD SAFE
    {
        try{
            if(!f)
            {
                throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "invalid callback handler for method `%'", mth);
            }
            register_callback_driver<oi::dummy_msg, oi::dummy_msg>(f, mth, parallel, srz, oi::MTH_SIG);
        }
        catch(oi::exception& ex)
        {
            ex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception. unable to register callback for `%'");
            throw ex;
        }

    }

    void oi::sig_interface::call() throw(oi::exception)
    {
        oi::dummy_msg d;
        oi::method_interface<oi::dummy_msg, oi::dummy_msg>::call(d, d);
    }

    std::ostream& oi::operator<< (std::ostream& os, const oi::communicator::state & s)
    {
       switch(s)
       {
           case oi::communicator::NEW: 
               os << "NEW(not initialized)"; 
               break;

           case oi::communicator::READY: 
               os << "READY"; 
               break;

           case oi::communicator::SIGNALED: 
               os << "SIGNALED(shutdown)"; 
               break;

           case oi::communicator::TERMINATED: 
               os << "TERMINATED(finalized)"; 
               break;
       };
       return os;
    }
