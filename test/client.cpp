#include"communicator.hpp"
void signal()
{
    std::cerr << "signalled " << std::endl;
}
void shutdown(oi::communicator * cm)
{
    cm->shutdown();
}
int main()
{
    int count = 10000;
    oi::communicator cm;
    cm.initialize("notification");

    boost::function<void(void)> f_sig = boost::bind( &signal);
    cm.register_callback(f_sig, "signal", 5, oi::SRZ_BOOST);
    
    boost::function<void(void)> f = boost::bind(&shutdown, &cm);
    cm.register_callback(f, "shutdown",1,oi::SRZ_MSGPACK);

    oi::get_interface<oi::com_type<int> > m_if;
    m_if = cm.create_get_interface<oi::com_type<int> >("core", "get_int");
    

    
    oi::sig_interface m_if_kill;
    m_if_kill = cm.create_sig_interface("core", "shutdown");
    
    for(int j=0 ; j< count; j++)
    {
        int rsp;
        try{
            rsp = m_if.call();
            std::cerr << "rsp: " << rsp << std::endl;
        }
        catch(std::exception & ex)
        {
            std::cerr << ex.what() << std::endl;
        }
    }
    std::cerr << "sending SHUTDOWN to server" << std::endl;
    //m_if_kill.call();
    cm.wait();
    cm.finalize();
    return 0;
}
