#include"communicator.hpp"
#include "container.hpp"
int ii ;
    template<typename T>
T get_int()
{
    ii+=2;
    usleep( rand()%20 *1000);
    std::cerr <<  "ass:" << ii << std::endl;
    return ii;
}

    template<typename T>
T get_data()
{
    oi::container ct;
    ii++;
    ct.set_data("child1",ii);
    return ct;
}

void shutdown(oi::communicator * cm)
{
    cm->shutdown();
}

int main()
{
    ii = 1;
    oi::communicator cm;
    cm.initialize("core");
    int count = 10000;
    
    boost::function<oi::com_type<int>(void)> f_get_int;
    f_get_int = boost::bind( &get_int<oi::com_type<int> > );
    cm.register_callback<oi::com_type<int> >(f_get_int, "get_int", 5, oi::SRZ_MSGPACK);

    boost::function<oi::container(void)> f_get_data = boost::bind( &get_data<oi::container> );
    cm.register_callback<oi::container >(f_get_data, "get_data", 5, oi::SRZ_BOOST);

    boost::function<void(void)> f = boost::bind(&shutdown,&cm);
    cm.register_callback(f, "shutdown",1,oi::SRZ_MSGPACK);



//    oi::sig_interface m_if;
//    m_if = cm.create_sig_interface("notification", "signal");
//	for(int j=0 ; j< count; j++)
//	{
//        try{
//            m_if.call();
//        }
//        catch(std::exception & ex)
//        {
//            std::cerr << ex.what() << std::endl;
//        }
//	}
//
    cm.wait();
    cm.finalize();
    return 0;
}
