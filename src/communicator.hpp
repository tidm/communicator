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

#include<tracer.hpp>
#include<shared_mutex.hpp>

#define ZMQ_CONTEXT_IO_THREADS 10

namespace oi
{
    const int SOCKET_TIMEOUT_VALUE = 200;
    const int CHANNEL_SOCKET_RECV_TIMEOUT = 1000;
    const int CHANNEL_SOCKET_SEND_TIMEOUT = 200;
//    enum oi_err {OI_SUCCESS, OI_ERROR};

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
            int  _thread_count;
            int _rcv_timeout;
            int _snd_timeout;
            virtual void call(T t, R& r)throw (oi::exception);
            void initialize(const std::string &modulename,
                    const std::string &methodname,
                    communicator * comm, 
                    int snd_timeout, 
                    int rcv_timeout,
                    int thread_count

                    )throw(oi::exception)
            {
                if(modulename.empty())
                {
                    throw oi::exception(__FILE__, __PRETTY_FUNCTION__,  "invalid module name `%'",modulename);
                }
                if(methodname.empty())
                {
                    throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "invalid method name `%'", _module_name);
                }
                if(comm == NULL)
                {
                    throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "invalid communicator reference! NULL pointer exception! ");
                }
                if(thread_count < 1 || thread_count > 200)
                {
                    throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "invalid no of threads `%'! ", thread_count);
                }
                _method_name = methodname;
                _module_name = modulename;
                _snd_timeout = snd_timeout;
                _rcv_timeout = rcv_timeout;
                _thread_count = thread_count;
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
                    int rcv_timeout,
                    int thread_count) throw(oi::exception)
            {
                method_interface<T,R>::initialize(modulename, methodname, comm, snd_timeout, rcv_timeout, thread_count);
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
                    int rcv_timeout,
                    int thread_count) throw (oi::exception)
            {
                    method_interface<T,dummy_msg>::initialize(modulename, methodname, comm, snd_timeout, rcv_timeout, thread_count);
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
                    int rcv_timeout,
                    int thread_count)throw(oi::exception)
            {
                    method_interface<dummy_msg, T>::initialize(modulename, methodname, comm, snd_timeout, rcv_timeout, thread_count);
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
                    int rcv_timeout,
                    int thread_count)throw(std::exception)
            {
                 method_interface<dummy_msg, dummy_msg>::initialize(modulename, methodname, comm, snd_timeout, rcv_timeout, thread_count);
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
//oi::tracer _tracer;
            
            std::vector<std::thread> _worker_thread_list;
            std::vector<std::thread> _proxy_thread_list;
            zmq::context_t _context;
            std::vector<zmq::socket_t*>  _clients;
            std::vector<zmq::socket_t*>  _workers;
            std::mutex _proxy_thread_mutex;
            std::string  _name;
            std::map<std::string, channel_base*> _channel_map;
            std::mutex _channel_map_mutex;
            std::string _ipc_file_path;

            volatile state _state;

            service_info _service_info;
            oi::shared_mutex _service_info_guard;

            std::map<std::string, service_info> _dst_service_list;
            oi::shared_mutex _dst_setvice_list_guard;

            std::map<std::string, transmission_stat*> _service_stat_list;
            std::mutex _service_stat_list_guard;

            std::function<void(const oi::exception&)> _exception_handler;


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
                                std::to_string(static_cast<int>(srz))) +
                                ".ipc"
                                );
                    }
                    catch(std::exception& ex)
                    {
                        oi::exception ox("std", "exception", ex.what());
                        ox.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled std::exception in generating ipc address");
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
                                std::to_string(static_cast<int>(srz)) + 
                                ".inprc"
                                );
                    }
                    catch(std::exception& ex)
                    {
                        oi::exception ox("std", "exception", ex.what());
                        ox.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled std::exception in generating inproc address");
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
                    std::function<R(T)>  h_req;
                    std::function<R(void)>     h_get;
                    std::function<void(T)>      h_put;
                    std::function<void(void)>   h_sig;
                    std::string exception_src("REMOTE");
                    try
                    {
                        {
                            oi::shared_lock<oi::shared_mutex> ml{_service_info_guard};
                            sgn = _service_info.get(ipc_path);
                        }
                        exception_src += "(" + sgn.module + ":" + sgn.method +")";
                        switch(sgn.type)
                        {
                            case MTH_REQ:
                                h_req =  boost::any_cast< std::function<R(T)> >(sgn.handler);
                                try
                                {
                                    r = h_req(t);
                                }
                                catch(oi::exception &ex)
                                {
                                    ex.add_msg(exception_src.c_str(), "oi::exception", "unhandled remote exception");
                                    r.set_exception(ex.what(), ex.error_code(), exception_type_val::SERVICE);
                                }
                                catch(std::exception & ex)
                                {
                                    oi::exception ox(exception_src.c_str(), "std::exception", ex.what());
                                    r.set_exception(ox.what(),0,exception_type_val::SERVICE );
                                }
                                catch(...)
                                {
                                    r.set_exception("unhandled REMOTE unknown exception",0,exception_type_val::SERVICE);
                                }
                                break;
                            case MTH_GET:
                                h_get =  boost::any_cast< std::function<R(void)> >(sgn.handler);
                                try
                                {
                                    r = h_get();
                                }
                                catch(oi::exception &ex)
                                {
                                    ex.add_msg(exception_src.c_str(), "oi::exception", "unhandled REMOTE exception");
                                    r.set_exception(ex.what(), ex.error_code(), exception_type_val::SERVICE);
                                }
                                catch(std::exception & ex)
                                {
                                    oi::exception ox(exception_src.c_str(), "std::exception", ex.what());
                                    r.set_exception(ox.what(),0,exception_type_val::SERVICE );
                                }
                                catch(...)
                                {
                                    r.set_exception("unhandled REMOTE unknown exception",0,exception_type_val::SERVICE);
                                }



                                break;
                            case MTH_PUT:
                                h_put =  boost::any_cast< std::function<void(T)> >(sgn.handler);
                                try
                                {
                                    h_put(t);
                                }
                                catch(oi::exception &ex)
                                {
                                    ex.add_msg(exception_src.c_str(), "oi::exception", "unhandled REMOTE exception");
                                    r.set_exception(ex.what(), ex.error_code(), exception_type_val::SERVICE);
                                }
                                catch(std::exception & ex)
                                {
                                    oi::exception ox(exception_src.c_str(), "std::exception", ex.what());
                                    r.set_exception(ox.what(),0,exception_type_val::SERVICE );
                                }
                                catch(...)
                                {
                                    r.set_exception("unhandled REMOTE unknown exception",0,exception_type_val::SERVICE);
                                }

                                break;
                            case MTH_SIG:
                                h_sig =  boost::any_cast< std::function<void(void)> >(sgn.handler);
                                try
                                {
                                    h_sig();
                                }
                                catch(oi::exception &ex)
                                {
                                    ex.add_msg(exception_src.c_str(), "oi::exception", "unhandled REMOTE exception");
                                    r.set_exception(ex.what(), ex.error_code(), exception_type_val::SERVICE);
                                }
                                catch(std::exception & ex)
                                {
                                    oi::exception ox(exception_src.c_str(), "std::exception", ex.what());
                                    r.set_exception(ox.what(),0,exception_type_val::SERVICE );
                                }
                                catch(...)
                                {
                                    r.set_exception("unhandled REMOTE unknown exception",0,exception_type_val::SERVICE);
                                }

                                break;
                            default:
                                throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "invalid method type `%' for the `%:%' is defined ",sgn.type, sgn.module, sgn.method);
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
                            std::string str = generate_inproc_addr<T,R>(_name, method_name, srz);
                            sock.connect( str.c_str());
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
                        {
                            std::lock_guard<std::mutex> m(_service_stat_list_guard);
                            srv_stat = _service_stat_list[ipc_addr];
                        }


                        while(_state == READY )
                        {
                            //recieving a message

                            try
                            {
                                zmq::message_t req;
                                T t;
                                R r;

                                try
                                {
                                    sock.recv(&req);
                                }
                                catch(zmq::error_t & ex)
                                {
                                    oi::exception ox("zmq", "exception", ex.what());
                                    ox.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unable to receive data from client in `%' (`%',`%')", 
                                            method_name, typeid(T).name(), typeid(R).name());
                                    throw ox;
                                }
                                catch(std::exception& ex)
                                {
                                    oi::exception ox("std", "exception", ex.what());
                                    ox.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unable to receive data from client in `%' (`%',`%')", 
                                            method_name, typeid(T).name(), typeid(R).name());
                                    throw ox;
                                }

                                auto s1 = std::chrono::system_clock::now();
                                //de-serialization the request
                                try
                                {
                                    util.to_data_msg<T>(req, t);
                                }
                                catch(oi::exception& ex)
                                {
                                    oi::exception ox("oi", "exception", ex.what());
                                    ox.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unable to de-serialize received data from client to `%' ",  typeid(T).name());
                                    throw ox;
                                }


                                auto s2 = std::chrono::system_clock::now();
                                //processing the request
                                generic_handler<T,R>(t, std::ref(r), ipc_addr);

                                auto s3 = std::chrono::system_clock::now();
                                //serialization of the response
                                zmq::message_t* rsp = NULL;
                                try
                                {
                                    rsp = util.to_zmq_msg<R>(r);
                                }
                                catch(oi::exception& ex)
                                {
                                    oi::exception ox("oi", "exception", ex.what());
                                    ox.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unable to serialize `%' to send as response to the client", typeid(R).name());
                                    throw ox;
                                }

                                auto s4 = std::chrono::system_clock::now();
                                //sending the response
                                try{
                                    sock.send(*rsp);
                                }
                                catch(zmq::error_t & ex)
                                {
                                    if(rsp != NULL)
                                    {
                                        delete rsp;
                                    }
                                    oi::exception ox("zmq", "exception", ex.what());
                                    ox.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unable to send response data to client `%(%,%)` ", method_name, typeid(T).name(), typeid(R).name());
                                    throw ox;
                                }
                                if(rsp != NULL)
                                {
                                    delete rsp;
                                }
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
                            catch(oi::exception & ox)
                            {
                                if(_state == READY )
                                {
                                    if(_exception_handler)
                                    {
                                        _exception_handler(ox);
                                    }
                                    else
                                    {
                                        throw ox;
                                    }
                                }
                            }
                        }

                    }
                    catch(oi::exception& ex)
                    {
                        if(_state == READY )
                        {
                            if(_exception_handler)
                            {
                                _exception_handler(ex);
                            }
                            else
                            {
                                throw ex;
                            }
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
                            ox.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled zmq::exception. unable to bind client router to %", ipc_addr);
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
                            ox.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled zmq::exception. unable to bind worker dealer to `%'",  inproc_addr);
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

                            {
                                std::lock_guard<std::mutex> m(_proxy_thread_mutex);
                                _clients.push_back(client);
                                _workers.push_back(worker);
                                for(int i=0; i< parallel_degree; i++)
                                {
                                    _worker_thread_list.emplace_back( std::bind( &communicator::worker_thread_function<T,R>, this, ipc_addr, srz, method_name ) );
                                }
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
            service_info get_service_list(const std::string& module, bool use_cache = true);
            //uint64_t get_trace_no();
            //std::mutex _trace_no_lock;
            //uint64_t _trace_no;
            template<typename T, typename R>
                double request(const std::string &module, const std::string& method, T& t, R& r, int snd_timeout, int rcv_timeout, int thread_count )throw(oi::exception)
                {

                    timespec t_s, t_e;
                    std::string ipc_str;
                    serializer srz = SRZ_UNKNOWN;
                    channel<T,R>* chnl  = NULL;
                    //uint64_t trace_no = get_trace_no();
                    //_tracer.update(trace_no, __LINE__);
                    clock_gettime(CLOCK_REALTIME, &t_s);
                    try
                    {
                        std::string ipc_str_mspack= generate_ipc_addr<T,R>( module, method, SRZ_MSGPACK );
                        std::string ipc_str_boost= generate_ipc_addr<T,R>( module, method, SRZ_BOOST );
                        std::string ipc_str_get_service_info = generate_ipc_addr<dummy_msg, service_info>(module, SERVICE_INFO_METHOD_NAME , SRZ_MSGPACK );


                        if(ipc_str_mspack != ipc_str_get_service_info)
                        {
                            //_tracer.update(trace_no, __LINE__);
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
                                    //_tracer.update(trace_no, __LINE__);
                                    throw oi::exception(__FILE__, __PRETTY_FUNCTION__,"requested service (%:%) is not registered on the server. avalable services are %",
                                            module, method, srv_inf.to_string());
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
//_tracer.update(trace_no, __LINE__);


                    try
                    {
//_tracer.update(trace_no, __LINE__);
                        std::lock_guard<std::mutex> m{_channel_map_mutex};
                        if(_channel_map.find(ipc_str) == _channel_map.end())
                        {

                            chnl = new channel<T,R>(module, method, ipc_str, &_context, srz, snd_timeout, rcv_timeout);
                            chnl->init(thread_count);
                            _channel_map[ipc_str] = chnl;
                        }
                        else
                        {
                            chnl = static_cast<channel<T,R>*>(_channel_map[ipc_str]);
                        }
//_tracer.update(trace_no, __LINE__);
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


                    try
                    {
//_tracer.update(trace_no, __LINE__);
                        chnl->process(static_cast<void*>(&t),static_cast<void*>(&r));
                    }
                    catch(oi::exception& ex)
                    {
                        ex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception in channel process");
                        throw ex;
                    }

                    clock_gettime(CLOCK_REALTIME, &t_e);
//_tracer.del(trace_no);
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
                        throw oi::exception(__FILE__, __PRETTY_FUNCTION__,"invalid use of un-initilized communicator! call 'initialize()' before any callback registration");
                    }
                    if(parallel_degree < 1)
                    {
                        throw oi::exception(__FILE__, __PRETTY_FUNCTION__,"invalid parallel value: `%'", parallel_degree);
                    }
                    if(method_name.empty())
                    {
                        throw oi::exception(__FILE__, __PRETTY_FUNCTION__,"invalid method name: `%'", method_name);
                    }
                    if(_name.empty())
                    {
                        throw oi::exception(__FILE__, __PRETTY_FUNCTION__,"invalid module name: `%'", _name);
                    }
                    if(!zmq_msg_util::valid_serializer(srz) )
                    {
                        throw oi::exception(__FILE__, __PRETTY_FUNCTION__,"invalid serializer: `%'", srz);
                    }

                    try
                    {
                        std::string ipc_path = generate_ipc_addr<T,R>(_name, method_name, srz);
                        {
                            std::lock_guard<std::mutex> m(_service_stat_list_guard);
                            _service_stat_list[ipc_path] = new transmission_stat();
                        }


                        {
                            std::lock_guard<oi::shared_mutex> lk{_service_info_guard};
                            _service_info.put( _name, method_name, mth_type, typeid(T).name(), typeid(R).name(), srz, ipc_path, f);
                        }
                                
                        _proxy_thread_list.emplace_back(std::bind( &communicator::proxy_thread_function<T,R>, this,method_name, srz, parallel_degree ));
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
                    oi::shared_lock<oi::shared_mutex> ml{_service_info_guard};
                    return _service_info;
                }
        public:
            communicator()throw();
            void initialize(const std::string &me, const std::function<void(const oi::exception&)> & exception_handler = NULL)throw(oi::exception);
            void wait()throw(oi::exception);
            bool is_remote_ready(const std::string& module, const std::string& method)throw(oi::exception);
            std::map<std::string, cm_stat> get_channel_stat()throw(oi::exception);
            std::map<std::string, cm_info> get_service_stat()throw(oi::exception);
            std::map<std::string, cm_info> get_interface_stat()throw(oi::exception);
            void finalize()throw();
            void shutdown()throw();
            ~communicator()throw();
            sig_interface create_sig_interface(const std::string &module,
                    const std::string &method, 
                    int snd_timeout = CHANNEL_SOCKET_SEND_TIMEOUT, 
                    int rcv_timeout = CHANNEL_SOCKET_RECV_TIMEOUT,
                    int thread_count = 1
                    )throw (oi::exception);

            void register_callback(std::function<void(void)> f,
                    const std::string &mth,
                    int parallel= 1,
                    serializer srz= SRZ_BOOST
                    )throw(oi::exception);//NOT THREAD SAFE;

            template<typename T, typename R>
                req_interface<T,R> create_req_interface(const std::string &module, 
                        const std::string &method , 
                        int snd_timeout = CHANNEL_SOCKET_SEND_TIMEOUT, 
                        int rcv_timeout = CHANNEL_SOCKET_RECV_TIMEOUT,
                        int thread_count = 1
                        )throw(oi::exception)
                {
                    req_interface<T,R> f;
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

            template<typename T>
                put_interface<T> create_put_interface(const std::string &module,
                        const std::string &method, 
                        int snd_timeout = CHANNEL_SOCKET_SEND_TIMEOUT, 
                        int rcv_timeout = CHANNEL_SOCKET_RECV_TIMEOUT,
                        int thread_count = 1
                        )throw(oi::exception)
                {
                    put_interface<T> f;
                    try
                    {
                        f.initialize(module, method, this, snd_timeout, rcv_timeout, thread_count);
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
                        int rcv_timeout = CHANNEL_SOCKET_RECV_TIMEOUT,
                        int thread_count = 1

                        )throw(oi::exception)
                {
                    get_interface<T> f;
                    try
                    {
                        f.initialize(module, method, this, snd_timeout, rcv_timeout, thread_count);
                    }
                    catch(oi::exception& ex)
                    {
                        ex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception. unable to initialize get_interface");
                        throw ex;
                    }
                    return f;
                }

            template<typename T>
                void register_callback(std::function<void(T)> f,
                        const std::string &mth,
                        int parallel= 1,
                        serializer srz= SRZ_BOOST
                        )throw(oi::exception)//NOT THREAD SAFE
                {
                    if(!f)
                    {
                        throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "invalid callback handler for method `%'", mth );
                    }
                    try{
                        register_callback_driver<T, dummy_msg>(f, mth, parallel, srz, MTH_PUT);
                    }
                    catch(oi::exception& ex)
                    {
                        ex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception. unable to register callback for `%'", mth);
                        throw ex;
                    }

                }

            template<typename R>
                void register_callback(std::function<R(void)> f,
                        const std::string &mth,
                        int parallel= 1,
                        serializer srz= SRZ_BOOST
                        )throw(oi::exception)//NOT THREAD SAFE
                {
                    if(!f)
                    {
                        throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "invalid callback handler for method `%'", mth);
                    }
                    try{
                        register_callback_driver<dummy_msg,R>(f, mth, parallel, srz, MTH_GET);
                    }
                    catch(oi::exception& ex)
                    {
                        ex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception. unable to register callback for `%'", mth);
                        throw ex;
                    }
                }


            template<typename T, typename R>
                void register_callback(std::function<R(T)> f,
                        const std::string &mth,
                        int parallel = 1,
                        serializer srz= SRZ_BOOST
                        )throw(oi::exception)//NOT THREAD SAFE
                {
                    if(!f)
                    {
                        throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "invalid callback handler for method `%'", mth);
                    }
                    try
                    {
                        register_callback_driver<T,R>(f, mth, parallel, srz, MTH_REQ);
                    }
                    catch(oi::exception& ex)
                    {
                        ex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception. unable to register callback for `%'", mth);
                        throw ex;
                    }
                }
    };

    std::ostream& operator<< (std::ostream& os, const oi::communicator::state & s);

    template <class T, class R>
        void method_interface<T, R>::call(T t, R& r) throw(oi::exception)
        {
            if(_is_init == false)
            {
                throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "invalid use of not initialized method interface! use communicator::get_interface_interface");
            }

            try
            {
                _comm->request(_module_name, _method_name, t, r, _snd_timeout, _rcv_timeout, _thread_count);
            }
            catch(oi::exception& ex)
            {
                ex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled oi::exception in calling request method" );
                throw ex;
            }
            if(r.exception_flag())
            {
                oi::exception ox( ("REMOTE:" + _module_name).c_str(), _method_name.c_str(), r.exception_msg());
                ox.error_code(r.error_code());
                throw ox;
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
