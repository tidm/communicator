#include "comm_stubs.hpp"
#include "child.hpp"
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
        void set_data(std::string ch, int i)
        {
            id = i;
            if(ch == "child1")
            {
                child * cc = new child();
                cc->p = i+1;
                cc->c = i+2;
                pp = cc;
            }
            else
            {
                throw std::runtime_error("invalid child type");
            }
        }
        container(const container&);
        container & operator=(const container&);
        ~container();
        int id;
        parent * pp;
        private:

    };
}

