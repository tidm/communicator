#include"communicator.hpp"
#include "container.hpp"
#include "child.hpp"
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
//    try
//    {
//        try
//        {
//            try
//            {
//                try
//                {
//                    try
//                    {
//
//                        oi::exception ox("std", "exception", "1");
//                        ox.error_code(34);
//                        throw ox;
//
//                    }
//                    catch(oi::exception & ox)
//                    {
//                        ox.add_msg(__FILE__, __FUNCTION__, "2");
//                        throw ox;
//                    }
//
//                }
//                catch(oi::exception & ox)
//                {
//                    ox.add_msg(__FILE__, __FUNCTION__, "3");
//                    throw ox;
//                }
//
//            }
//            catch(oi::exception & ox)
//            {
//                ox.add_msg(__FILE__, __FUNCTION__, "4");
//                throw ox;
//            }
//
//        }
//        catch(oi::exception & ox)
//        {
//            ox.add_msg(__FILE__, __FUNCTION__, "5");
//            throw ox;
//        }
//
//    }
//    catch(oi::exception & ox)
//    {
//        ox.add_msg(__FILE__, __FUNCTION__, "6");
//        throw ox;
//    }
   // usleep(4000);
    oi::container ct;
    ii++;
    ct.id = ii;
    oi::child * ch =new oi::child();
    ch->set_p(ii+1);
    ch->set_c(ii+2);
    ct.set_data(ch);
    return ct;
}

void shutdown1(oi::communicator * cm)
{
    cm->shutdown();
}

void exp_handler(const oi::exception & ex)
{
    std::cerr << "****************************************\n" << ex.what() << std::endl;
}

int main()
{
    ii = 1;
    oi::communicator cm;
    cm.initialize("core", exp_handler);
    int count = 10000;

    std::function<oi::com_type<int>(void)> f_get_int;
    f_get_int = std::bind( &get_int<oi::com_type<int> > );
    cm.register_callback<oi::com_type<int> >(f_get_int, "get_int", 5, oi::SRZ_MSGPACK);

    std::function<oi::container(void)> f_get_data = std::bind( &get_data<oi::container> );
    cm.register_callback<oi::container >(f_get_data, "get_data", 5, oi::SRZ_BOOST);

    std::function<void(void)> f = std::bind(&shutdown1,&cm);
    cm.register_callback(f, "shutdown",1,oi::SRZ_MSGPACK);

    std::map<std::string, oi::cm_info> m;
    std::map<std::string, oi::cm_info>::iterator it;
    while(1)
    {
        m = cm.get_service_stat();
        for(it = m.begin(); it != m.end(); it++)
        {
            std::cerr << it->first << ":" << it->second << std::endl;
        }
        std::cerr << "---------------------------------" << std::endl;
        sleep(1);
    }

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
