#include"communicator.hpp"
#include "container.hpp"
#include <thread>
void mysignal()
{
    std::cerr << "signalled " << std::endl;
}
oi::communicator cm;
void shutdown1()
{
    cm.shutdown();
}
bool is_done;
void get_stat()
{
    std::map<std::string, oi::cm_info> m;
    std::map<std::string, oi::cm_info>::iterator it;
    while(!is_done)
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

void call_core_get_data(int count)
{
    oi::get_interface<oi::container > m_if_cont = cm.create_get_interface<oi::container >("core", "get_data", 100,100,40);

    for(int j=0 ; j< count; j++)
    {
        oi::container rsp;
        try{
            rsp = m_if_cont.call();
          std::cerr << "rsp: " << rsp << std::endl;
        }
        catch(oi::exception & ex)
        {
            std::cerr << ex.what() << std::endl;
            std::cerr << "eeror_cocde: " << ex.error_code()<< std::endl;
          //  std::cerr << ex.what() << std::endl;
        }
        catch(std::exception & ex)

        {
            std::cerr << ex.what() << std::endl;
          //  std::cerr << ex.what() << std::endl;
        }
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

    if(argc < 3)
    {
        std::cerr << "usage: " << argv[0] << " [no. of threads] [no. of requests]" << std::endl;
        exit(1);
    }


    int count = atoi(argv[2]);
    int thread_count= atoi(argv[1]);
    cm.initialize("notification");


    std::function<void()> f_sig = mysignal;
    cm.register_callback(f_sig, "signal", 5, oi::SRZ_BOOST);
    
    std::function<void()> f1 = shutdown1;
    cm.register_callback(f1, "shutdown",1,oi::SRZ_MSGPACK);

    oi::get_interface<oi::com_type<int> > m_if;
    m_if = cm.create_get_interface<oi::com_type<int> >("core", "get_int", 10, 10);
    
    oi::sig_interface m_if_kill;
    m_if_kill = cm.create_sig_interface("core", "shutdown");

    std::thread th(get_stat);
    std::vector<std::thread> thread_list;
    for(int i=0; i< thread_count; i++)
    {
        thread_list.emplace_back(call_core_get_data, count);
    }

    for(int i=0; i< thread_count; i++)
    {
        thread_list[i].join();
    }
    is_done = true;
    th.join();

//    std::cerr << "sending SHUTDOWN to server" << std::endl;
//    m_if_kill.call();
  //  cm.wait();
    cm.finalize();
    return 0;
}
