#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/export.hpp>

#include "child.hpp"

BOOST_CLASS_EXPORT_GUID(oi::child, "child_one")

namespace oi
{
    std::ostream& operator << (std::ostream& os, const child & ct)
    {

        os << static_cast<const parent&>(ct);
        os << " C: "  << ct.c;
        return os;
    }
    std::string child::to_string()
    {
        std::stringstream sstr;
        sstr << *this;
        return sstr.str();
    }
    child::child():parent()
    {
        c = 0;
    }
    parent * child::clone()
    {
        return new child(*this);
    }
}
