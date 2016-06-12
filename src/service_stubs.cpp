#include "service_stubs.hpp"


oi::service_sign::service_sign()
{
    handler = nullptr;
    srz = SRZ_UNKNOWN;  // invalid
    type = MTH_UNKNOWN; // invalid
}

std::string oi::service_sign::to_string()throw(oi::exception)
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

std::ostream& oi::operator<<(std::ostream& os, const oi::service_sign & m)
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

oi::service_info::service_info():oi::com_object()
{

}

bool oi::service_info::has_service(const std::string& str)throw(oi::exception)
{
    if(str.empty())
    {
        return false;
    }
    try
    {
        for(const auto & it : _service_list)
        {
            if( it.ipc_path == str)
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

oi::service_sign oi::service_info::get(const std::string& str)throw(oi::exception)
{
    try
    {
        for(const auto& it : _service_list)
        {
            if( it.ipc_path == str)
            {
                return it;
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
    throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "service '%' is NOT registered!");
}
std::set<std::string> oi::service_info::get_methods()throw()
{
    std::set<std::string> lst;
    try
    {
        for(const auto& it : _service_list)
        {
            lst.insert(it.method);
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

    return lst;
}

void oi::service_info::put( std::string module,
        std::string method,
        method_type type,
        std::string req,
        std::string rsp,
        serializer  srz,
        std::string ipc_path,
        boost::any handler)throw (oi::exception)
{
    oi::service_sign s;
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
            throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "service '%' has already been registered!");
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

std::string oi::service_info::to_string()throw(oi::exception)
{

    std::stringstream sstr;
    std::string str;
    try
    {
        sstr <<  _service_list.size() << " methods : {";
        for(const auto& it:  _service_list)
        {
            sstr << oi::operator<<(sstr, it);
            sstr << ", ";
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

std::ostream& oi::operator<<(std::ostream& os, const oi::service_info& srv)
{
    int i=0;
    for(const auto& it : srv._service_list)
    {
        os << "[" << i << "]" << it;
        os << "\n";
        i++;
    }
    return os;

}

