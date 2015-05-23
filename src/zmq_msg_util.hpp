#ifndef PACK_UTIL_HPP
#define PACK_UTIL_HPP
///boost serializer header files
//#define BOOST_ALL_DYN_LINK
//#define DSO
#include<boost/serialization/array.hpp>
#include<boost/serialization/bitset.hpp>
#include<boost/serialization/deque.hpp>
#include<boost/serialization/list.hpp>
#include<boost/serialization/map.hpp>
#include<boost/serialization/set.hpp>
#include<boost/serialization/string.hpp>
#include<boost/serialization/vector.hpp>
#include<boost/function.hpp>
#include<boost/archive/binary_oarchive.hpp>
#include<boost/archive/binary_iarchive.hpp>
#include<boost/iostreams/device/array.hpp>
#include<boost/iostreams/stream.hpp>
#include<sstream>
#include"zmq.hpp"
//message_pack serializer header files
#include<msgpack.hpp>
#include<exception.hpp>

namespace oi
{
    typedef int serializer;

    const serializer SRZ_UNKNOWN = 0 ;
    const serializer SRZ_MSGPACK = 1;
    const serializer SRZ_BOOST = 2;

    //enum serializer{SRZ_UNKNOWN = 0, SRZ_MSGPACK = 1, SRZ_BOOST = 2};

    class zmq_msg_util
    {
        private:
            serializer _tool;
        public:

            zmq_msg_util()throw();
            zmq_msg_util(serializer tool)throw();
            static bool valid_serializer(const serializer & s);
            template<typename T>
                zmq::message_t* to_zmq_msg(const T& t)throw(oi::exception)
                {
                    zmq::message_t* rsp = NULL;

                    try
                    {
                        std::ostringstream ss(std::ios::out|std::ios::binary);
                        boost::archive::binary_oarchive out_archive(ss);
                        msgpack::sbuffer buffer;
                        switch(_tool)
                        {
                            case SRZ_BOOST:
                                {
                                    out_archive << t;

                                    rsp = new zmq::message_t(ss.str().size());
                                    memcpy((void*)rsp->data(), ss.str().c_str(), ss.str().size());
                                    //                            out_archive.delete_created_pointers();
                                }
                                break;
                            case SRZ_MSGPACK:
                                {
                                    msgpack::pack(&buffer, t);

                                    rsp = new zmq::message_t(buffer.size());
                                    memcpy((void*)rsp->data(), buffer.data(), buffer.size());
                                }
                                break;
                            default:
                                std::stringstream sstr;
                                sstr << "invalid serialization option " << _tool;
                                throw oi::exception(__FILE__, __PRETTY_FUNCTION__, sstr.str().c_str() );
                        };
                    }
                    catch(zmq::error_t er)
                    {
                        if(rsp != NULL)
                        {
                            delete rsp;
                            rsp = NULL;
                        }
                        oi::exception oiex("zmq", "exception", er.what());
                        oiex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled  zmq exception");
                        throw oiex;
                    }
                    catch(std::exception& ex)
                    {
                        if(rsp != NULL)
                        {
                            delete rsp;
                            rsp = NULL;
                        }
                        oi::exception oiex("std", "exception", ex.what());
                        oiex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled std::exception");
                        throw oiex;
                    }
                    catch(...)
                    {
                        if(rsp != NULL)
                        {
                            delete rsp;
                            rsp = NULL;
                        }
                        throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "Unhandled unknown exception.");
                    }
                    return rsp;
                }
            template<typename T>
                void to_data_msg(zmq::message_t & msg, T& t)throw (oi::exception)
                {
                    msgpack::unpacked result;
                    char * data = NULL;

                    try
                    { 

                        switch(_tool)
                        {
                            case SRZ_BOOST:
                                {
                                    data = static_cast<char*>(msg.data());
                                    boost::iostreams::array_source source(data, msg.size());
                                    boost::iostreams::stream<boost::iostreams::array_source> stream(source);
                                    boost::archive::binary_iarchive in_archive(stream);

                                    in_archive >> t;

                                    //                           in_archive.delete_created_pointers();
                                }
                                break;
                            case SRZ_MSGPACK:

                                {
                                    msgpack::unpack(&result,(char*)(msg.data()), msg.size());

                                    msgpack::object obj = result.get();
                                    t = obj.as<T>();
                                }
                                break;
                            default:
                                std::stringstream sstr;
                                sstr << "invalid serialization option " << _tool;
                                throw oi::exception(__FILE__, __PRETTY_FUNCTION__, sstr.str().c_str() );
                        };
                    }

                    catch(zmq::error_t er)
                    {
                        oi::exception oiex("zmq", "exception", er.what());
                        oiex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled  zmq exception");
                        throw oiex;
                    }
                    catch(std::exception& ex)
                    {
                        oi::exception oiex("std", "exception", ex.what());
                        oiex.add_msg(__FILE__, __PRETTY_FUNCTION__, "Unhandled std::exception");
                        throw oiex;
                    }
                    catch(...)
                    {
                        throw oi::exception(__FILE__, __PRETTY_FUNCTION__, "Unhandled unknown exception.");
                    }
                }

    };
}
#endif
