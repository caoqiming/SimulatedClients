#include <http_client.hpp>
#include <fstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <util.hpp>


class SimulatedClients{

public:
    SimulatedClients(){
        data_path = "./data/data.txt";
        data_file.open(data_path, std::ios::in );
    }

    template <class T> void log(T s) {
        time_t now = time(0);
        tm *gmtm = gmtime(&now);
        std::lock_guard<std::mutex> lck(log_mutex_);
        std::ofstream outfile("./log/clientlog.txt", std::ios::out | std::ios::app);
        std::string time;
        time = format("%d.%d.%d %d:%d ",gmtm->tm_year+1900,gmtm->tm_mon,gmtm->tm_mday,(gmtm->tm_hour+8)%24,gmtm->tm_min);
        while(time.size()<14){
            time += ' ';
        }
        outfile << time;
        outfile << s << std::endl;
        outfile.close();
    }

    template <class T> void task_log(T s) {
        time_t now = time(0);
        tm *gmtm = gmtime(&now);
        std::lock_guard<std::mutex> lck(task_log_mutex_);
        std::ofstream outfile("./log/tasklog.txt", std::ios::out | std::ios::app);
        outfile << s << std::endl;
        outfile.close();
    }


	bool json_decode(std::string strResponse,std::shared_ptr<boost::property_tree::ptree>pt){
		try{
			std::stringstream sstream(strResponse);
			boost::property_tree::json_parser::read_json(sstream, *pt);
		}
		catch (std::exception& e){
			std::cout << "exception: " << e.what() << "\n";
			return false;
		}
		return true;
	}

    void run(){
        std::string data;
        time_t time_now,time_start;//精确到秒 用于发送请求
        float time_client; //用户发送的时间不一定是整数
        std::string url,request_body,response_body;
        int client;
        time_start = time(NULL);//用于按时间发送请求
        unsigned long long time_send,time_spend; //精确到毫秒 用于计算任务用时
        while(getline(data_file,data) && !data.empty()){
            std::cout << data << std::endl;
		    auto pt = std::make_shared<boost::property_tree::ptree>();
		    if(!json_decode(data,pt)){
                log(format("data:  %s is not json", data.c_str()));
                continue;
		    }
		    if(pt->find("time")==pt->not_found()){
                continue;
		    }
		    if(pt->find("url")==pt->not_found()){
                continue;
		    }
		    if(pt->find("client")==pt->not_found()){
                continue;
		    }
            time_client=pt->get<float>("time");
            url=pt->get<std::string>("url");
            client=pt->get<int>("client");
            time_now = time(NULL) - time_start;
            while(time_now<time_client){
                Sleep(100);
                time_now = time(NULL) - time_start;
            }
            
            // send request
			boost::property_tree::ptree request_pt;
            request_pt.put<std::string>("type","get_resource_info");
			request_pt.put<std::string>("url", url);
            request_pt.put<int>("client", client);
			std::stringstream wos;
			boost::property_tree::write_json(wos, request_pt);
			request_body = wos.str();
            try {
                boost::asio::io_context io_context;
                HttpClient c(io_context, "127.0.0.1", "/buffer",request_body, response_body);
                time_send= ::GetTickCount64(); // start count time
                io_context.run();
            }catch (std::exception &e) {
                std::cout << "http Exception: " << e.what() << "\n";
                continue;
            }
		    auto response_pt = std::make_shared<boost::property_tree::ptree>();
		    if(!json_decode(response_body,response_pt)){
                log(format("response_body: \n%s\n is not json, url:\"%s\"", response_body.c_str(),url.c_str()));
                continue;
		    }
		    if(response_pt->find("status")==response_pt->not_found()){
                log(format("response_body: \n%s\n do not have status, url:\"%s\"", response_body.c_str(),url.c_str()));
                continue;
		    }
            if(response_pt->get<int>("status")==0){ // success
                time_spend = ::GetTickCount64() - time_send;
                task_log(format("{\"delay\": %d , \"url\": \"%s\"}",time_spend, url.c_str()));
            }
            else{
                 if(response_pt->find("error")!=response_pt->not_found()){
                    log(format("Task failed, url: \"%s\" status: %d , error: %s", url.c_str()
                        ,response_pt->get<int>("status"),response_pt->get<std::string>("error").c_str()));
                 }
                 else{
                    log(format("Task failed, url: \"%s\" status: %d", url.c_str()
                        ,response_pt->get<int>("status")));
                 }
            }
        }

    }

private:
    std::string data_path;
    std::ifstream  data_file;
    std::mutex log_mutex_;
    std::mutex task_log_mutex_;
};




int main(){
    SimulatedClients s;
    s.run();

 
    return false;
}