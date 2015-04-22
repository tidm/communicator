#include "parent.hpp"
namespace oi
{
    std::ostream& operator << (std::ostream& os, const parent & ct)
    {
        os << "P: " << ct.p;
        return os;
    }
    std::string parent::to_string()
    {
        std::stringstream sstr;
        sstr << *this;
        return sstr.str();
    }
    parent::parent()
    {
        p = 0;
    }
    parent::~parent()
    {
    }
}
