#include "container.hpp"
namespace oi
{
    std::ostream& operator << (std::ostream& os, const container & ct)
    {
        os << ct.id;
        if( ct.pp == NULL)
        {
            os << "{data : NULL }";
        }
        else
        {
            os << " data: {" << ct.pp->to_string() << "}" ;
        }
        return os;
    }
    container::container()
    {
        id =0;
        pp = NULL;
    }
    container::container(const container& cn)
    {
        id = cn.id;
        if(cn.pp != NULL)
        {
             pp = cn.pp->clone();
        }
        else
        {
            pp = NULL;
        }
    }
    container & container::operator=(const container& cn)
    {
        id = cn.id;
        if(pp != NULL)
        {
            delete pp;
        }
        if(cn.pp != NULL)
        {
            pp = cn.pp->clone();
        }
        else
        {
            pp = NULL;
        }
        return *this;
    }
    container::~container()
    {
        if(pp != NULL)
        {
            delete pp;
        }
    }
}
