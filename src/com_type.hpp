#ifndef COMM_TYPE_HPP
#define COMM_TYPE_HPP
#include "comm_stubs.hpp"
namespace oi
{
    template<class R>
        class com_type: public com_object
    {
        friend class boost::serialization::access;
        private:
        R _val;
        public:
        OI_MSGPACK_DEFINE(_val)

        template<class T>
            void serialize(T & ar, const unsigned int version)
            {
                ar & boost::serialization::base_object<com_object>(*this);
                ar & _val;
            }
        com_type()
        {}
        com_type(const R& v)
        {
            _val = v;
        }
        operator R&()
        {
            return _val;
        }
        com_type& operator =(const R& v)
        {
            _val = v;
            return *this;
        }
    };

}
#endif
