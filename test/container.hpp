#include "comm_stubs.hpp"
#include "parent.hpp"
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/base_object.hpp>
namespace oi
{
    class container:public com_object
    {
        friend class boost::serialization::access;
        friend std::ostream& operator << (std::ostream& os, const container & ct);

        public:
        template <class T>
            void serialize(T & ar, const unsigned int ver)
            {
                ar & boost::serialization::base_object<com_object>(*this);
                ar & id;
                ar & pp;
            }
        container();
        void set_data(parent * pr)
        {
            pp = pr;
        }
        container(const container&);
        container & operator=(const container&);
        ~container();
        int id;
        parent * pp;
        private:

    };
}

