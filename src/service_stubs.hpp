#ifndef SERVICE_STUBS_HPP
#define SERVICE_STUBS_HPP

#include "comm_stubs.hpp"
#include<exception.hpp>
namespace oi
{
    typedef int method_type ;

    const method_type MTH_UNKNOWN = 0;
    const method_type MTH_REQ = 1;
    const method_type MTH_GET = 2;
    const method_type MTH_PUT = 3;
    const method_type MTH_SIG = 4;
    
    class service_sign
    {
        public:
            std::string module;
            std::string method;
            method_type type;
            std::string req;
            std::string rsp;
            serializer  srz;
            std::string ipc_path;
            boost::any  handler;
            MSGPACK_DEFINE(module, method, type, req, rsp, srz, ipc_path)
            service_sign();
            std::string to_string()throw(oi::exception);

    };

    class service_info: public com_object
    {
        friend std::ostream& operator<<(std::ostream& os, const service_info& srv);
        private:
            std::vector<service_sign> _service_list;
        public:
       //     template<class T>
       //         void serialize(T & ar, const unsigned int version)
       //         {
       //             ar & boost::serialization::base_object<com_object>(*this);
       //             //ar & _service_list; 
       //         }
            service_info();
            OI_MSGPACK_DEFINE(_service_list)
            bool has_service(const std::string& str)throw(oi::exception);
            service_sign get(const std::string& str)throw(oi::exception);
            std::set<std::string> get_methods()throw();
            void put( std::string module,
                    std::string method,
                    method_type type,
                    std::string req,
                    std::string rsp,
                    serializer  srz,
                    std::string ipc_path,
                    boost::any handler)throw (oi::exception);
            std::string to_string()throw(oi::exception);
    };
    std::ostream& operator<<(std::ostream& os, const service_info& srv);
    std::ostream& operator<<(std::ostream& os, const service_sign & m);
}
#endif
