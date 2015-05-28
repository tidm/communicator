#include"communicator.hpp"
#include "container.hpp"
#include <thread>
void signal()
{
    std::cerr << "signalled " << std::endl;
}
void shutdown(oi::communicator * cm)
{
    cm->shutdown();
}
oi::communicator cm;
void get_stat()
{
    std::map<std::string, oi::cm_info> m;
    std::map<std::string, oi::cm_info>::iterator it;
    while(1)
    {
        m = cm.get_interface_stat();
        for(it = m.begin(); it != m.end(); it++)
        {
            std::cerr << it->first << ":" << it->second << std::endl;
        }
        std::cerr << "---------------------------------" << std::endl;
        sleep(1);
    }
}
int main(int argc, char* argv[])
{
//   oi::com_type< std::vector<int> > v;
//   ((std::vector<int>&)v).push_back(123);;
//   ((std::vector<int>&)v).push_back(123);;
//   ((std::vector<int>&)v).push_back(123);;
//   ((std::vector<int>)v).push_back(123);;
//   ((std::vector<int>)v).push_back(123);;
//   ((std::vector<int>)v).push_back(123);;
//   static_cast<std::vector<int>&>(v).push_back(20);
//   std::cerr << ((std::vector<int>&)v).size() << std::endl;
//
//return 0;

    if(argc < 2)
    {
        std::cerr << "usage: " << argv[0] << " [no. of requests]" << std::endl;
        exit(1);
    }


    int count = atoi(argv[1]);
    cm.initialize("notification");

    boost::function<void(void)> f_sig = boost::bind( &signal);
    cm.register_callback(f_sig, "signal", 5, oi::SRZ_BOOST);
    
    boost::function<void(void)> f = boost::bind(&shutdown, &cm);
    cm.register_callback(f, "shutdown",1,oi::SRZ_MSGPACK);

    oi::get_interface<oi::com_type<int> > m_if;
    m_if = cm.create_get_interface<oi::com_type<int> >("core", "get_int", 10, 10);
    
    oi::sig_interface m_if_kill;
    m_if_kill = cm.create_sig_interface("core", "shutdown");
    

    oi::get_interface<oi::container > m_if_cont = cm.create_get_interface<oi::container >("core", "get_data");

    std::thread th(get_stat);


    for(int j=0 ; j< count; j++)
    {
        oi::container rsp;
        try{
            rsp = m_if_cont.call();
//          std::cerr << "rsp: " << rsp << std::endl;
        }
        catch(std::exception & ex)
        {
            std::cerr << ex.what() << std::endl;
        }
    }

    std::cerr << "sending SHUTDOWN to server" << std::endl;
    m_if_kill.call();
    cm.wait();
    cm.finalize();
    return 0;
}
