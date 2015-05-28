#include "communicator.hpp"
#include<config.h>
namespace oi
{
    std::ostream& operator<<(std::ostream& os, const oi_err& o)
    {
        switch(o)
        {
            case OI_SUCCESS:
                os << " SUCCESS";
                break;
            case OI_ERROR:
                os << "ERROR";
                break;
            default:
                os << "Unknown error!";
                break;
        };
        return os;
    }


    void communicator::lock()
    {
        _proxy_thread_mutex.lock();
    }
    void communicator::unlock()
    {
        _proxy_thread_mutex.unlock();
    }

    service_info communicator::get_service_list(const std::string& module, bool use_cache)
    {

        dummy_msg d;
        service_info srv;
        bool sw = false;
        if(use_cache == true)
        {
            _dst_setvice_list_gaurd.lock_shared();
            {
                std::map<std::string, service_info>::iterator it = _dst_service_list.begin();
                it = _dst_service_list.find(module);
                if(it != _dst_service_list.end())
                {
                    srv = it->second;
                    sw = true;
                }
            }
            _dst_setvice_list_gaurd.unlock_shared();
        }
        if(sw == false)
        {
            request<dummy_msg, service_info>(module, SERVICE_INFO_METHOD_NAME , d, srv, CHANNEL_SOCKET_SEND_TIMEOUT, CHANNEL_SOCKET_RECV_TIMEOUT);
            _dst_setvice_list_gaurd.lock();
            {
                _dst_service_list[module] = srv; 
            }
            _dst_setvice_list_gaurd.unlock();
        }
        return srv;
    }

    communicator::communicator()throw()
        :_context(ZMQ_CONTEXT_IO_THREADS)
    {
        _state = NEW;
        _name = "";
        _ipc_file_path = IPC_FILE_PATH;
    }

    std::map<std::string, cm_info> communicator::get_service_stat()throw(oi::exception)
    {
        std::map<std::string, cm_info> stat;
        std::map<std::string, transmission_stat*>::iterator it;

        service_sign sgn ;

        _service_stat_list_guard.lock();
        try
        {
            for(it = _service_stat_list.begin(); it != _service_stat_list.end(); it++)
            {
                _service_info_gaurd.lock_shared();
                sgn = _service_info.get(it->first);
                _service_info_gaurd.unlock_shared();
                stat[sgn.module + ":" + sgn.method] = it->second->get_stat();
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

    std::map<std::string, cm_info> communicator::get_interface_stat()throw(oi::exception)
    {
        std::map<std::string, cm_info> stat;
        std::string module;
        std::string method;
        _channel_map_mutex.lock();
        {
            try
            {
                std::map<std::string, channel_base*>::iterator it;
                for(it = _channel_map.begin();it != _channel_map.end(); it++)
                {
                    module = it->second->get_module();
                    method = it->second->get_method();

                    stat[module + ":" + method] = it->second->get_ch_stat();
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
    std::map<std::string, cm_stat> communicator::get_channel_stat()throw(oi::exception)
    {
        std::map<std::string, cm_stat> stat;

        _channel_map_mutex.lock();
        {
            try
            {
                std::map<std::string, channel_base*>::iterator it;
                for(it = _channel_map.begin();it != _channel_map.end(); it++)
                {
                    stat[it->first] = it->second->get_stat();
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

    void communicator::wait()throw(oi::exception)
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
    bool communicator::is_remote_ready(const std::string & remote_module, const std::string & method_name)throw()
    {
        bool is_ready = false;

        service_info srv; 
        try{
            srv = get_service_list(remote_module, false);
            std::set<std::string> lst= srv.get_methods();
            std::set<std::string>::iterator it = lst.find(method_name);
            if(it != lst.end())
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


    void communicator::initialize(const std::string &me)throw(oi::exception)
    {
        if(_state != NEW)
        {
            throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "multiple 'init' call!");
        }
        if(me.empty())
        {
            throw oi::exception(__FILE__, __PRETTY_FUNCTION__, (std::string("invalid module name : '")+ me + "'").c_str());
        }
        _name = me;

        _state = READY;
        try
        {
            boost::function<service_info(void)> f = boost::bind(&communicator::get_service_info<service_info>, this);
            register_callback<service_info>(f, SERVICE_INFO_METHOD_NAME, 1, SRZ_MSGPACK);
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

    void communicator::shutdown()throw()
    {
        _state = SIGNALED;
    }

    void communicator::finalize()throw()
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
                channel_base * c;
                _channel_map_mutex.lock();
                {
                    std::map<std::string, channel_base*>::iterator it;
                    for(it = _channel_map.begin();it != _channel_map.end(); it++)
                    {
                        c = it->second;
                        if(c != NULL)
                        {
                            c->close();
                            delete c;
                        }
                    }
                }
                _channel_map_mutex.unlock();
            }

            _context.close();

            for(size_t i = 0; i< _worker_thread_list.size(); i++)
            {
                _worker_thread_list[i]->join();
                delete _worker_thread_list[i];
            }
            for(size_t i = 0; i< _proxy_thread_list.size(); i++)
            {
                _proxy_thread_list[i]->join();
                delete _proxy_thread_list[i];
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

    communicator::~communicator()throw()
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

    sig_interface communicator::create_sig_interface(const std::string &module,
            const std::string &method, 
            int snd_timeout , 
            int rcv_timeout 

            )throw (oi::exception)
    {
        sig_interface f;
        try
        {
            f.initialize(module, method, this, snd_timeout, rcv_timeout);
        }
        catch(oi::exception& ex)
        {
            ex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception. unable to initialize req_interface");
            throw ex;
        }
        return f;
    }

    void communicator::register_callback(boost::function<void(void)> f,
            const std::string &mth,
            int parallel,
            serializer srz
            )throw(oi::exception)//NOT THREAD SAFE
    {
        try{
            if(f.empty())
            {
                throw oi::exception(__FILE__, __PRETTY_FUNCTION__, (std::string("invalid callback handler for method") + mth).c_str());
            }
            register_callback_driver<dummy_msg, dummy_msg>(f, mth, parallel, srz, MTH_SIG);
        }
        catch(oi::exception& ex)
        {
            ex.add_msg(__FILE__, __PRETTY_FUNCTION__, (std::string("Unhandled oi::exception. unable to register callback for ") + mth).c_str());
            throw ex;
        }

    }

    void sig_interface::call() throw(oi::exception)
    {
        dummy_msg d;
        method_interface<dummy_msg, dummy_msg>::call(d, d);
    }


}
