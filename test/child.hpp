#ifndef CHILD_HPP
#define CHILD_HPP
#include "parent.hpp"
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/base_object.hpp>
namespace oi
{
    class child:public parent
    {
        friend class boost::serialization::access;
        friend std::ostream& operator << (std::ostream& os, const child & ct);

        public:
        parent * clone();
        std::string to_string();
        template <class T>
            void serialize(T & ar, const unsigned int ver)
            {
                ar & boost::serialization::base_object<parent>(*this);
                ar & c;
            }
        void set_c(int x)
        {
            c = x;
        }
        child();
        int c;

    };
}
#endif
