#include "zmq_msg_util.hpp"
namespace oi
{
//    std::ostream& operator<<(std::ostream& os, const serializer& s)
//    {
//        switch(s)
//        {
//            case SRZ_MSGPACK:
//                os << "MessagePack";
//                break;
//            case SRZ_BOOST:
//                os << "Boost Serialization";
//                break;
//            case SRZ_UNKNOWN:
//                os << "Unknown!";
//                break;
//            default:
//                os << "invalid serializer!";
//                break;
//        };
//        return os;
//    }
    zmq_msg_util::zmq_msg_util()throw()
    {
        _tool = SRZ_BOOST;
    }
    zmq_msg_util::zmq_msg_util(serializer tool)throw()
    {
        _tool = tool;
    }
    bool zmq_msg_util::valid_serializer(const serializer & s)
    {
        switch(s)
        {
            case SRZ_MSGPACK:
            case SRZ_BOOST:
                return true;
            default:
                return false;
        };
        return false;
    }
}
