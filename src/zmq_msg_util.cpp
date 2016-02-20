#include "zmq_msg_util.hpp"
oi::zmq_msg_util::zmq_msg_util()throw() {
    _tool = oi::SRZ_BOOST;
}
oi::zmq_msg_util::zmq_msg_util(oi::serializer tool)throw() {
    _tool = tool;
}
bool oi::zmq_msg_util::valid_serializer(const oi::serializer& s) {
    switch(s) {
    case oi::SRZ_MSGPACK:
    case oi::SRZ_BOOST:
        return true;
    default:
        return false;
    };
    return false;
}
