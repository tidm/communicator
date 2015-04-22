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
        virtual ~parent();
        int p;

    };
}

