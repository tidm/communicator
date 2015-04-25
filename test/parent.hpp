#ifndef PARENT_HPP
#define PARENT_HPP
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <sstream>
namespace oi
{
    class parent
    {
        friend class boost::serialization::access;
        friend std::ostream& operator << (std::ostream& os, const parent & ct);

        
        public:
        virtual  parent * clone() = 0;
        virtual std::string to_string();
        template <class T>
            void serialize(T & ar, const unsigned int ver)
            {
                ar & p;
            }
        parent();
        void set_p(int c)
        {
            p = c;
        }
        virtual ~parent();
        int p;

    };
}
#endif
