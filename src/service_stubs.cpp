#include "service_stubs.hpp"
namespace oi
{

    std::ostream& operator<<(std::ostream& os, const method_type & m)
    {
        switch(m)
        {
            case MTH_REQ:
                os << "Request-Response";
                break;
            case MTH_GET:
                os << "Getter";
                break;
            case MTH_PUT:
                os << "Putter";
                break;
            case MTH_SIG:
                os << "Signal";
                break;
            case MTH_UNKNOWN :
                os << "Unknown";
                break;
            default:
                os << "Invalid Method!";
        };
        return os;
    }


    service_sign::service_sign()
    {
        handler = NULL;
        srz = SRZ_UNKNOWN;  // invalid
        type = MTH_UNKNOWN; // invalid
    }

    std::string service_sign::to_string()throw(oi::exception)
    {
        std::string str("");
        try
        {
            std::stringstream sstr;
            sstr << module << ":" << method;
            switch(type)
            {
                case MTH_REQ:
                    sstr << "(" << req << ", " << rsp << ") ";
                    break;
                case MTH_GET:
                    sstr << "(" << rsp << "&) ";
                    break;
                case MTH_PUT:
                    sstr << "(" << req << ") ";
                    break;
                case MTH_SIG:
                    sstr << "() ";
                    break;
                default:
                    sstr << "(!!UNKOWN!!!) ";

            };

            sstr << " type:" << type;
            sstr << " srz:" << srz;// << " ipc_path:" << ipc_path ;
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

    std::ostream& operator<<(std::ostream& os, const service_sign & m)
    {
        os << m.module << ":" << m.method;
        switch(m.type)
        {
            case MTH_REQ:
                os << "(" << m.req << ", " << m.rsp << ") ";
                break;
            case MTH_GET:
                os << "(" << m.rsp << "&) ";
                break;
            case MTH_PUT:
                os << "(" << m.req << ") ";
                break;
            case MTH_SIG:
                os << "() ";
                break;
            default:
                os << "(!!UNKOWN!!!) ";

        };

        os << " type:" << m.type;
        os << " srz:" << m.srz;// << " ipc_path:" << m.ipc_path ;
        return os;
    }

    service_info::service_info():com_object()
    {

    }

    bool service_info::has_service(const std::string& str)throw(oi::exception)
    {
        if(str.empty())
        {
            return false;
        }
        try
        {
            std::vector<service_sign>::iterator it;
            for(it = _service_list.begin(); it != _service_list.end(); it++)
            {
                if( it->ipc_path == str)
                {
                    return true;
                }
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

        return false;
    }

    service_sign service_info::get(const std::string& str)throw(oi::exception)
    {
        bool flag = false;
        std::vector<service_sign>::iterator it;
        try
        {
            for(it = _service_list.begin(); it != _service_list.end(); it++)
            {
                if( it->ipc_path == str)
                {
                    flag = true;
                    break;
                }
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

        if(!flag)
        {
            throw oi::exception(__FILE__, __PRETTY_FUNCTION__, std::string("service '"+ str + "' is NOT registered!").c_str());
        }

        return *it;
    }

    void service_info::put( std::string module,
            std::string method,
            method_type type,
            std::string req,
            std::string rsp,
            serializer  srz,
            std::string ipc_path,
            boost::any handler)throw (oi::exception)
    {
        service_sign s;
        s.module  = module;
        s.method  = method;
        s.type    = type;
        s.req     = req;
        s.rsp     = rsp;
        s.srz     = srz;
        s.ipc_path = ipc_path;
        s.handler = handler;
        try
        {
            if(has_service(ipc_path))
            {
                throw oi::exception(__FILE__, __PRETTY_FUNCTION__, std::string("service '"+ s.to_string() + "' has already been registered!").c_str());
            }
            _service_list.push_back(s);
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

    std::string service_info::to_string()throw(oi::exception)
    {

        std::stringstream sstr;
        std::string str;
        try
        {
            sstr <<  _service_list.size() << " methods : {";
            std::vector<service_sign>::iterator it;
            for(it = _service_list.begin(); it != _service_list.end(); it++)
            {
                sstr << *it << ", ";
            }
            sstr << "}";
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

    std::ostream& operator<<(std::ostream& os, const service_info& srv)
    {
        std::vector<service_sign>::const_iterator it;
        int i=0;
        for(it = srv._service_list.begin(); it != srv._service_list.end(); it++)
        {
            os << "[" << i << "]" << *it;
            os << "\n";
            i++;
        }

    }

} // EON
