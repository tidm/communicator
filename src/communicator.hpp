#ifndef COMMUNICATIOR_HPP
#define COMMUNICATIOR_HPP
#include"channel.hpp"
#include"com_type.hpp"
#include"cm_stat.hpp"
#include"service_stubs.hpp"
#include <chrono>
#include<common.hpp>
#include<typeinfo>
#include<vector>
#include<time.h>
#include<memory>
#include<boost/lexical_cast.hpp>

#define ZMQ_CONTEXT_IO_THREADS 10

namespace oi
{
    const int SOCKET_TIMEOUT_VALUE = 200;
    const int CHANNEL_SOCKET_RECV_TIMEOUT = 1000;
    const int CHANNEL_SOCKET_SEND_TIMEOUT = 200;
    enum oi_err {OI_SUCCESS, OI_ERROR};

    class communicator;


    template<class T, class R>
        class method_interface
        {
            friend class communicator;
            private:
            std::string _method_name;
            std::string _module_name;
            communicator * _comm;
            bool _is_init;

            protected:
            int _rcv_timeout;
            int _snd_timeout;
            virtual void call(T t, R& r)throw (oi::exception);
            void initialize(const std::string &modulename,
                    const std::string &methodname,
                    communicator * comm, 
                    int snd_timeout, 
                    int rcv_timeout

                    )throw(oi::exception)
            {
                if(modulename.empty())
                {
                    throw oi::exception(__FILE__, __PRETTY_FUNCTION__, (std::string("invalid module name provided: ")+_module_name).c_str());
                }
                if(methodname.empty())
                {
                    throw oi::exception(__FILE__, __PRETTY_FUNCTION__, (std::string("invalid method name provided: ")+_module_name).c_str());
                }
                if(comm == NULL)
                {
                    throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "invalid communicator reference! NULL pointer exception! ");
                }
                _method_name = methodname;
                _module_name = modulename;
                _snd_timeout = snd_timeout;
                _rcv_timeout = rcv_timeout;
                _comm       = comm;
                _is_init = true;
            }
            method_interface()
            {
                _comm = NULL;
                _is_init = false;
                _rcv_timeout = CHANNEL_SOCKET_RECV_TIMEOUT;
                _snd_timeout = CHANNEL_SOCKET_SEND_TIMEOUT;
            }
            public:
            std::string to_string()throw(oi::exception)

            {
                std::string str;
                std::stringstream sstr;
                try{
                    sstr << _module_name << "::" << _method_name << "(" << typeid(T).name() << ", " << typeid(R).name() << "& )";
                    str = sstr.str();
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
                return str;
            }
        };
    template<class T, class R>
        class req_interface: public method_interface<T,R>
    {
        friend class communicator;
        private:
            void initialize(const std::string &modulename,
                    const std::string &methodname,
                    communicator * comm,
                    int snd_timeout,
                    int rcv_timeout) throw(oi::exception)
            {
                method_interface<T,R>::initialize(modulename, methodname, comm, snd_timeout, rcv_timeout);
            }
        public:
            req_interface():method_interface<T,R>(){}
            R call(T t)throw(oi::exception);
    };
    template<class T>
        class put_interface: public method_interface<T,dummy_msg>
    {
        friend class communicator;
        private:
            void initialize(const std::string &modulename,
                    const std::string &methodname,
                    communicator * comm,
                    int snd_timeout,
                    int rcv_timeout) throw (oi::exception)
            {
                    method_interface<T,dummy_msg>::initialize(modulename, methodname, comm, snd_timeout, rcv_timeout);
            }
        public:
            put_interface():method_interface<T, dummy_msg>(){}
            void call(T t)throw(oi::exception);
    };

    template<class T>
        class get_interface: public method_interface<dummy_msg, T>
    {
        friend class communicator;
        private:
            void initialize(const std::string &modulename,
                    const std::string &methodname,
                    communicator * comm,
                    int snd_timeout,
                    int rcv_timeout)throw(oi::exception)
            {
                    method_interface<dummy_msg, T>::initialize(modulename, methodname, comm, snd_timeout, rcv_timeout);
            }
        public:
            get_interface():method_interface<dummy_msg, T>(){}
            T call()throw(oi::exception);
    };
    class sig_interface: public method_interface<dummy_msg, dummy_msg>
    {
        friend class communicator;
        private:
            void initialize(const std::string &modulename,
                    const std::string &methodname,
                    communicator * comm,
                    int snd_timeout,
                    int rcv_timeout)throw(std::exception)
            {
                 method_interface<dummy_msg, dummy_msg>::initialize(modulename, methodname, comm, snd_timeout, rcv_timeout);
            }
        public:
            sig_interface():method_interface<dummy_msg, dummy_msg>(){}
            void call()throw(oi::exception);
    };

#define SERVICE_INFO_METHOD_NAME  "__GET_SERVICE_INFO"
#define GET_STATE_METHOD_NAME  "__GET_STATE"
    class communicator
    {
        public:
            template<class T, class R> 
                friend class method_interface;
            enum state{NEW, READY, SIGNALED, TERMINATED};
        private:
            
            std::vector<boost::thread*> _worker_thread_list;
            std::vector<boost::thread*> _proxy_thread_list;
            zmq::context_t _context;
            std::vector<zmq::socket_t*>  _clients;
            std::vector<zmq::socket_t*>  _workers;
            boost::mutex _proxy_thread_mutex;
            std::string  _name;
            std::map<std::string, channel_base*> _channel_map;
            boost::mutex _channel_map_mutex;
            std::string _ipc_file_path;

            volatile state _state;

            service_info _service_info;
            boost::shared_mutex _service_info_gaurd;

            std::map<std::string, service_info> _dst_service_list;
            boost::shared_mutex _dst_setvice_list_gaurd;

            std::map<std::string, transmission_stat*> _service_stat_list;
            boost::mutex _service_stat_list_guard;


            template<typename T, typename R>
                std::string generate_ipc_addr(const std::string &module,
                        const std::string& method,
                        serializer srz)throw(oi::exception)
                {

                    std::string addr;
                    try{

                        addr = std::string( "ipc://" + _ipc_file_path +
                                common_utils::get_md5(
                                module + "_" +
                                method + "_" +
                                std::string(typeid(T).name()) + "_" +
                                std::string(typeid(R).name()) + "_" +
                                boost::lexical_cast<std::string>(static_cast<int>(srz))) +
                                ".ipc"
                                );
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
                    return addr;
                }

            template<typename T, typename R>
                std::string generate_inproc_addr(const std::string &module,
                        const std::string& method,
                        serializer srz
                        )
                {
                    std::string addr;
                    try{
                        addr = std::string( "inproc://" +
                                module + "_" +
                                method + "_" +
                                std::string(typeid(T).name()) + "_" +
                                std::string(typeid(R).name()) + "_" +
                                boost::lexical_cast<std::string>(static_cast<int>(srz)) + 
                                ".inprc"
                                );
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
                    return addr;
                }
            template<typename T, typename R>
                void generic_handler(T t, R & r, const std::string& ipc_path)
                {
                    service_sign sgn ;
                    boost::function<R(T)>  h_req;
                    boost::function<R(void)>     h_get;
                    boost::function<void(T)>      h_put;
                    boost::function<void(void)>   h_sig;
                    try
                    {
                        _service_info_gaurd.lock_shared();
                        sgn = _service_info.get(ipc_path);
                        _service_info_gaurd.unlock_shared();

                        switch(sgn.type)
                        {
                            case MTH_REQ:
                                h_req =  boost::any_cast< boost::function<R(T)> >(sgn.handler);
                                try
                                {
                                    r = h_req(t);
                                }
                                catch(oi::exception &ex)
                                {
                                    r.set_exception(std::string("unhandled REMOTE oi::exception ") +  ex.what());
                                }
                                catch(std::exception & ex)
                                {
                                    r.set_exception(std::string("unhandled REMOTE std::exception ") +  ex.what());
                                }
                                catch(...)
                                {
                                    r.set_exception("unhandled REMOTE unknown exception");
                                }
                                break;
                            case MTH_GET:
                                h_get =  boost::any_cast< boost::function<R(void)> >(sgn.handler);
                                try
                                {
                                    r = h_get();
                                }
                                catch(oi::exception &ex)
                                {
                                    r.set_exception(std::string("unhandled REMOTE oi::exception ") +  ex.what());
                                }
                                catch(std::exception & ex)
                                {
                                    r.set_exception(std::string("unhandled REMOTE std::exception ") +  ex.what());
                                }
                                catch(...)
                                {
                                    r.set_exception("unhandled REMOTE unknown exception");
                                }
                                break;
                            case MTH_PUT:
                                h_put =  boost::any_cast< boost::function<void(T)> >(sgn.handler);
                                try
                                {
                                    h_put(t);
                                }
                                catch(oi::exception &ex)
                                {
                                    r.set_exception(std::string("unhandled REMOTE oi::exception ") +  ex.what());
                                }
                                catch(std::exception & ex)
                                {
                                    r.set_exception(std::string("unhandled REMOTE std::exception ") +  ex.what());
                                }
                                catch(...)
                                {
                                    r.set_exception("unhandled REMOTE unknown exception");
                                }
                                break;
                            case MTH_SIG:
                                h_sig =  boost::any_cast< boost::function<void(void)> >(sgn.handler);
                                try
                                {
                                    h_sig();
                                }
                                catch(oi::exception &ex)
                                {
                                    r.set_exception(std::string("unhandled REMOTE oi::exception ") +  ex.what());
                                }
                                catch(std::exception & ex)
                                {
                                    r.set_exception(std::string("unhandled REMOTE std::exception ") +  ex.what());
                                }
                                catch(...)
                                {
                                    r.set_exception("unhandled REMOTE unknown exception");
                                }
                                break;
                            default:
                                throw oi::exception(__FILE__, __PRETTY_FUNCTION__, (std::string("invalid type for the '") + ipc_path + "' is defined!").c_str());
                        };
                    }
                    catch(oi::exception& ex)
                    {
                        ex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception");
                        throw ex;
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

            //THIS THREAD MAY THROW EXCEPTION WHICH CANNOT BE CAUGHT! SHOULD BE CONSIDERED REVISION
            template<typename T, typename R>
                void worker_thread_function(const std::string& ipc_addr,
                        serializer srz,
                        const std::string &method_name)throw()
                {
                    try{
                        

                        zmq::socket_t sock(_context, ZMQ_REP);
                        sock.setsockopt(ZMQ_LINGER, &SOCKET_TIMEOUT_VALUE, sizeof(SOCKET_TIMEOUT_VALUE));
                        try{
                            sock.connect( generate_inproc_addr<T,R>(_name, method_name, srz).c_str());
                        }
                        catch(zmq::error_t & ex)
                        {
                            oi::exception ox("std", "exception", ex.what());
                            ox.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled zmq::exception");
                            throw ox;
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

                        zmq_msg_util util(srz);
                        std::chrono::duration<double> total ;
                        std::chrono::duration<double> srz_req;
                        std::chrono::duration<double> process;
                        std::chrono::duration<double> srz_rsp;
                        transmission_stat * srv_stat;
                        _service_stat_list_guard.lock();
                            srv_stat = _service_stat_list[ipc_addr];
                        _service_stat_list_guard.unlock();


                        try
                        {
                            while(_state == READY )
                            {
                                zmq::message_t req;
                                T t;
                                R r;
                                //recieving a message
                                sock.recv(&req);

                                auto s1 = std::chrono::system_clock::now();
                                //de-serialization the request
                                util.to_data_msg<T>(req, t);

                                auto s2 = std::chrono::system_clock::now();
                                //processing the request
                                generic_handler<T,R>(t, boost::ref(r), ipc_addr);

                                auto s3 = std::chrono::system_clock::now();
                                //serialization of the response
                                zmq::message_t* rsp;
                                rsp = util.to_zmq_msg<R>(r);

                                auto s4 = std::chrono::system_clock::now();
                                //sending the response
                                sock.send(*rsp);
                                delete rsp;

                                 total   = s4 - s1;
                                 srz_req = s2 - s1;
                                 process = s3 - s2;
                                 srz_rsp = s4 - s3;
                                 srv_stat->update(total.count() * 1000000.0,
                                                                    srz_req.count() * 1000000.0, 
                                                                    srz_rsp.count() * 1000000.0,
                                                                    process.count() * 1000000.0,
                                                                    true);
                            }
                        }
                        catch(zmq::error_t & ex)
                        {
                            oi::exception ox("std", "exception", ex.what());
                            ox.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled zmq::exception");
                            throw ox;
                        }
                        catch(oi::exception& ex)
                        {
                            ex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception");
                            throw ex;
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
                    catch(oi::exception& ex)
                    {
                        if(_state == READY )
                        {
                            ex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception");
                            throw ex;
                        }
                    }
                }

            //THIS THREAD MAY THROW EXCEPTION WHICH CANNOT BE CAUGHT! SHOULD BE CONSIDERED REVISION
            template<typename T, typename R>
                void proxy_thread_function(const std::string &method_name,
                        serializer srz,
                        int parallel_degree)throw()
                {
                    std::string ipc_addr;
                    std::string inproc_addr;
                    zmq::socket_t * client = NULL;
                    zmq::socket_t * worker=  NULL;
                    try{

                        try
                        {
                            client = new zmq::socket_t(_context, ZMQ_ROUTER);
                            client->setsockopt(ZMQ_LINGER, &SOCKET_TIMEOUT_VALUE, sizeof(SOCKET_TIMEOUT_VALUE));
                        }
                        catch(zmq::error_t & ex)
                        {
                            oi::exception ox("std", "exception", ex.what());
                            ox.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled zmq::exception. unable to create zmq socket");
                            throw ox;
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

                        try
                        {
                            worker= new zmq::socket_t(_context, ZMQ_DEALER);
                            worker->setsockopt(ZMQ_LINGER, &SOCKET_TIMEOUT_VALUE, sizeof(SOCKET_TIMEOUT_VALUE));
                        }
                        catch(zmq::error_t & ex)
                        {
                            oi::exception ox("std", "exception", ex.what());
                            ox.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled zmq::exception. unable to create worker zmq socket(dealer)");
                            throw ox;
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

                        try{
                            ipc_addr =    generate_ipc_addr<T,R>(_name, method_name, srz);
                            inproc_addr = generate_inproc_addr<T,R>(_name, method_name, srz);
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


                        try{
                            client->bind( ipc_addr.c_str());
                        }
                        catch(zmq::error_t & ex)
                        {
                            oi::exception ox("std", "exception", ex.what());
                            ox.add_msg(__FILE__, __PRETTY_FUNCTION__, (std::string("Unhandled zmq::exception. unable to bind client router to ") + ipc_addr).c_str());
                            throw ox;
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


                        try{
                            worker->bind( inproc_addr.c_str());
                        }
                        catch(zmq::error_t & ex)
                        {
                            oi::exception ox("std", "exception", ex.what());
                            ox.add_msg(__FILE__, __PRETTY_FUNCTION__, (std::string("Unhandled zmq::exception. unable to bind worker dealer to ") + inproc_addr).c_str());
                            throw ox;
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


                        try{

                            lock();
                            _clients.push_back(client);
                            _workers.push_back(worker);
                            unlock();

                            //_is_run = true;
                            boost::thread* th;
                            for(int i=0; i< parallel_degree; i++)
                            {
                                th = new boost::thread(
                                        boost::bind(
                                            &communicator::worker_thread_function<T,R>,
                                            this,
                                            ipc_addr,
                                            srz,
                                            method_name
                                            )
                                        );
                                lock();
                                _worker_thread_list.push_back(th);
                                unlock();
                            }
                            zmq::proxy(*client, *worker, NULL);
                        }
                        catch(zmq::error_t & ex)
                        {
                            oi::exception ox("std", "exception", ex.what());
                            ox.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled zmq::exception");
                            throw ox;
                        }
                        catch(oi::exception& ex)
                        {
                            ex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception");
                            throw ex;
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
                    catch(oi::exception& ex)
                    {
                        if(_state == READY )
                        {
                            ex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception");
                            throw ex;
                        }
                    }

                    try
                    {
                        if(client != NULL)
                        {
                            client->close();
                            delete client;
                        }
                        if(worker !=  NULL)
                        {
                            worker->close();
                            delete worker;
                        }
                    }
                    catch(...)
                    {}
                }
            void lock();
            void unlock();
            service_info get_service_list(const std::string& module, bool use_cache = true);

            template<typename T, typename R>
                double request(const std::string &module, const std::string& method, T& t, R& r, int snd_timeout, int rcv_timeout )throw(oi::exception)
                {
                    timespec t_s, t_e;
                    std::string ipc_str;
                    serializer srz = SRZ_UNKNOWN;
                    channel<T,R>* chnl  = NULL;

                    clock_gettime(CLOCK_REALTIME, &t_s);
                    try
                    {
                        std::string ipc_str_mspack= generate_ipc_addr<T,R>( module, method, SRZ_MSGPACK );
                        std::string ipc_str_boost= generate_ipc_addr<T,R>( module, method, SRZ_BOOST );
                        std::string ipc_str_get_service_info = generate_ipc_addr<dummy_msg, service_info>(module, SERVICE_INFO_METHOD_NAME , SRZ_MSGPACK );


                        if(ipc_str_mspack != ipc_str_get_service_info)
                        {
                            service_info srv_inf = get_service_list(module);
                            if(srv_inf.has_service(ipc_str_mspack))
                            {
                                srz = SRZ_MSGPACK;
                                ipc_str = ipc_str_mspack;
                            }
                            else
                            {
                                if(srv_inf.has_service(ipc_str_boost))
                                {
                                    srz = SRZ_BOOST;
                                    ipc_str = ipc_str_boost;
                                }
                                else
                                {
                                    throw oi::exception(__FILE__, __PRETTY_FUNCTION__,("requested service not registered on the server. avalable services are" + srv_inf.to_string()).c_str());
                                }
                            }
                        }
                        else
                        {
                            srz = SRZ_MSGPACK;
                            ipc_str = ipc_str_get_service_info;
                        }
                    }
                    catch(oi::exception& ex)
                    {
                        ex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception");
                        throw ex;
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


                    _channel_map_mutex.lock();
                    try
                    {
                        if(_channel_map.find(ipc_str) == _channel_map.end())
                        {

                            chnl = new channel<T,R>(module, method, ipc_str, &_context, srz, snd_timeout, rcv_timeout);
                            chnl->init();
                            _channel_map[ipc_str] = chnl;
                        }
                        else
                        {
                            chnl = static_cast<channel<T,R>*>(_channel_map[ipc_str]);
                        }
                    }
                    catch(oi::exception& ex)
                    {
                        ex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception");
                        _channel_map_mutex.unlock();
                        throw ex;
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

                    _channel_map_mutex.unlock();

                    try
                    {
                        chnl->process(static_cast<void*>(&t),static_cast<void*>(&r));
                    }
                    catch(oi::exception& ex)
                    {
                        ex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception in channel process");
                        throw ex;
                    }

                    clock_gettime(CLOCK_REALTIME, &t_e);
                    return (t_e.tv_sec - t_s.tv_sec) * 1000000 + (t_e.tv_nsec - t_s.tv_nsec) / 1000.0;
                }

            template<typename T, typename R>
                void register_callback_driver(boost::any f,
                        const std::string &method_name,
                        int parallel_degree ,
                        serializer srz,
                        method_type mth_type
                        )throw(oi::exception)
                {
                    if(_state != READY )
                    {
                        throw oi::exception(__FILE__, __PRETTY_FUNCTION__,"invalid use of not initilized communicator! call 'initialize()' before any callback registration");
                    }
                    if(parallel_degree < 1)
                    {
                        throw oi::exception(__FILE__, __PRETTY_FUNCTION__,(std::string("invalid parallel value: ") + boost::lexical_cast<std::string>(parallel_degree)).c_str());
                    }
                    if(method_name.empty())
                    {
                        throw oi::exception(__FILE__, __PRETTY_FUNCTION__,(std::string("invalid method name: ") + method_name).c_str());
                    }
                    if(_name.empty())
                    {
                        throw oi::exception(__FILE__, __PRETTY_FUNCTION__,(std::string("invalid module name: ") + _name).c_str());
                    }
                    if(!zmq_msg_util::valid_serializer(srz) )
                    {
                        throw oi::exception(__FILE__, __PRETTY_FUNCTION__,(std::string("invalid serializer: ") + boost::lexical_cast<std::string>(srz)).c_str());
                    }

                    try
                    {
                        std::string ipc_path = generate_ipc_addr<T,R>(_name, method_name, srz);

                        _service_stat_list_guard.lock();
                        _service_stat_list[ipc_path] = new transmission_stat();
                        _service_stat_list_guard.unlock();


                        _service_info_gaurd.lock();
                        _service_info.put( _name,
                                method_name,
                                mth_type,
                                typeid(T).name(),
                                typeid(R).name(),
                                srz,
                                ipc_path,
                                f);
                        _service_info_gaurd.unlock();

                        boost::thread* th;
                        th = new boost::thread(
                                boost::bind(
                                    &communicator::proxy_thread_function<T,R>,
                                    this,
                                    method_name,
                                    srz,
                                    parallel_degree
                                    )
                                );
                        _proxy_thread_list.push_back(th);
                    }
                    catch(oi::exception& ex)
                    {
                        ex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception");
                        throw ex;
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
            template< typename T>
                T get_service_info() /// should be check of throwing exception
                {
                    T srv_info;
                    _service_info_gaurd.lock_shared();
                    srv_info = _service_info;
                    _service_info_gaurd.unlock_shared();
                    return srv_info;
                }
        public:
            communicator()throw();
            std::map<std::string, cm_stat> get_channel_stat()throw(oi::exception);
            std::map<std::string, cm_info> get_service_stat()throw(oi::exception);
            std::map<std::string, cm_info> get_interface_stat()throw(oi::exception);
            void initialize(const std::string &me)throw(oi::exception);
            void wait()throw(oi::exception);
            bool is_remote_ready(const std::string& module, const std::string& method)throw();
            void finalize()throw();
            void shutdown()throw();
            ~communicator()throw();
            sig_interface create_sig_interface(const std::string &module,
                    const std::string &method, 
                    int snd_timeout = CHANNEL_SOCKET_SEND_TIMEOUT, 
                    int rcv_timeout = CHANNEL_SOCKET_RECV_TIMEOUT
                    )throw (oi::exception);

            void register_callback(boost::function<void(void)> f,
                    const std::string &mth,
                    int parallel= 1,
                    serializer srz= SRZ_BOOST
                    )throw(oi::exception);//NOT THREAD SAFE;

            template<typename T, typename R>
                req_interface<T,R> create_req_interface(const std::string &module, 
                        const std::string &method , 
                        int snd_timeout = CHANNEL_SOCKET_SEND_TIMEOUT, 
                        int rcv_timeout = CHANNEL_SOCKET_RECV_TIMEOUT
                        )throw(oi::exception)
                {
                    req_interface<T,R> f;
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

            template<typename T>
                put_interface<T> create_put_interface(const std::string &module,
                        const std::string &method, 
                        int snd_timeout = CHANNEL_SOCKET_SEND_TIMEOUT, 
                        int rcv_timeout = CHANNEL_SOCKET_RECV_TIMEOUT

                        )throw(oi::exception)
                {
                    put_interface<T> f;
                    try
                    {
                        f.initialize(module, method, this, snd_timeout, rcv_timeout);
                    }
                    catch(oi::exception& ex)
                    {
                        ex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception. unable to initialize put_interface");
                        throw ex;
                    }
                    return f;
                }

            template<typename T>
                get_interface<T> create_get_interface(const std::string &module,
                        const std::string &method, 
                        int snd_timeout = CHANNEL_SOCKET_SEND_TIMEOUT, 
                        int rcv_timeout = CHANNEL_SOCKET_RECV_TIMEOUT

                        )throw(oi::exception)
                {
                    get_interface<T> f;
                    try
                    {
                        f.initialize(module, method, this, snd_timeout, rcv_timeout);
                    }
                    catch(oi::exception& ex)
                    {
                        ex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception. unable to initialize get_interface");
                        throw ex;
                    }
                    return f;
                }

            template<typename T>
                void register_callback(boost::function<void(T)> f,
                        const std::string &mth,
                        int parallel= 1,
                        serializer srz= SRZ_BOOST
                        )throw(oi::exception)//NOT THREAD SAFE
                {
                    if(f.empty())
                    {
                        throw oi::exception(__FILE__, __PRETTY_FUNCTION__, (std::string("invalid callback handler for method") + mth).c_str());
                    }
                    try{
                        register_callback_driver<T, dummy_msg>(f, mth, parallel, srz, MTH_PUT);
                    }
                    catch(oi::exception& ex)
                    {
                        ex.add_msg(__FILE__, __PRETTY_FUNCTION__, (std::string("Unhandled oi::exception. unable to register callback for ") + mth).c_str());
                        throw ex;
                    }

                }

            template<typename R>
                void register_callback(boost::function<R(void)> f,
                        const std::string &mth,
                        int parallel= 1,
                        serializer srz= SRZ_BOOST
                        )throw(oi::exception)//NOT THREAD SAFE
                {
                    if(f.empty())
                    {
                        throw oi::exception(__FILE__, __PRETTY_FUNCTION__, (std::string("invalid callback handler for method") + mth).c_str());
                    }
                    try{
                        register_callback_driver<dummy_msg,R>(f, mth, parallel, srz, MTH_GET);
                    }
                    catch(oi::exception& ex)
                    {
                        ex.add_msg(__FILE__, __PRETTY_FUNCTION__, (std::string("Unhandled oi::exception. unable to register callback for ") + mth).c_str());
                        throw ex;
                    }
                }


            template<typename T, typename R>
                void register_callback(boost::function<R(T)> f,
                        const std::string &mth,
                        int parallel = 1,
                        serializer srz= SRZ_BOOST
                        )throw(oi::exception)//NOT THREAD SAFE
                {
                    if(f.empty())
                    {
                        throw oi::exception(__FILE__, __PRETTY_FUNCTION__, (std::string("invalid callback handler for method") + mth).c_str());
                    }
                    try
                    {
                        register_callback_driver<T,R>(f, mth, parallel, srz, MTH_REQ);
                    }
                    catch(oi::exception& ex)
                    {
                        ex.add_msg(__FILE__, __PRETTY_FUNCTION__, (std::string("Unhandled oi::exception. unable to register callback for ") + mth).c_str());
                        throw ex;
                    }
                }
    };

    template <class T, class R>
        void method_interface<T, R>::call(T t, R& r) throw(oi::exception)
        {
            if(_is_init == false)
            {
                throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "invalid use of not initialized method interface! use communicator::get_interface_interface");
            }

            try
            {
                _comm->request(_module_name, _method_name, t, r, _snd_timeout, _rcv_timeout);
            }
            catch(oi::exception& ex)
            {
                ex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception in calling request method" );
                throw ex;
            }
            if(r.exception_flag())
            {
                throw oi::exception( _module_name.c_str(), (_method_name+"(Remote)").c_str() , r.exception_msg().c_str());
            }
        }
    template <class T, class R>
        R req_interface<T, R>::call(T t) throw(oi::exception)
        {
            R r;
            method_interface<T,R>::call(t, r);
            return r;
        }
    template <class T>
        void put_interface<T>::call(T t) throw(oi::exception)
        {
            dummy_msg d;
            method_interface<T,dummy_msg>::call(t,d);
        }
    template <class R>
        R get_interface<R>::call() throw(oi::exception)
        {
            dummy_msg d;
            R r;
            method_interface<dummy_msg, R>::call(d, r);
            return r;
        }


}
#endif
