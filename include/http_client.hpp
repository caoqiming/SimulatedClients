
#ifndef HTTP_CLIENT_HPP_
#define HTTP_CLIENT_HPP_
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <fstream>
#include <iostream>
#include <istream>
#include <ostream>
#include <string>

using boost::asio::ip::tcp;

class HttpClient {
 public:
    ~HttpClient() {}
    HttpClient(boost::asio::io_context &io_context, const std::string &server, const std::string &path,const std::string &request_body, std::string &response_body)
        : resolver_(io_context), socket_(io_context),response_body_(response_body) {
        response_body_.clear();

        std::ostream request_stream(&request_);
        request_stream << "GET " << path << " HTTP/1.0\r\n";
        request_stream << "Host: " << server << "\r\n";
        request_stream << "Accept: */*\r\n";
        request_stream << "Connection: close\r\n\r\n";
        request_stream << request_body;
        resolver_.async_resolve(server, "http",
                                boost::bind(&HttpClient::handle_resolve, this, boost::asio::placeholders::error,
                                            boost::asio::placeholders::results));
    }

 private:
    void handle_resolve(const boost::system::error_code &err, const tcp::resolver::results_type &endpoints) {
        if (!err) {
            // Attempt a connection to each endpoint in the list until we
            // successfully establish a connection.
            boost::asio::async_connect(
                    socket_, endpoints,
                    boost::bind(&HttpClient::handle_connect, this, boost::asio::placeholders::error));
        } else {
            std::cout << "Error: " << err.message() << "\n";
        }
    }

    void handle_connect(const boost::system::error_code &err) {
        if (!err) {
            // The connection was successful. Send the request.
            boost::asio::async_write(
                    socket_, request_,
                    boost::bind(&HttpClient::handle_write_request, this, boost::asio::placeholders::error));
        } else {
            std::cout << "Error: " << err.message() << "\n";
        }
    }

    void handle_write_request(const boost::system::error_code &err) {
        if (!err) {
            // Read the response status line. The response_ streambuf will
            // automatically grow to accommodate the entire line. The growth may
            // be limited by passing a maximum size to the streambuf
            // constructor.
            boost::asio::async_read_until(
                    socket_, response_, "\r\n",
                    boost::bind(&HttpClient::handle_read_status_line, this, boost::asio::placeholders::error));
        } else {
            std::cout << "Error: " << err.message() << "\n";
        }
    }

    void handle_read_status_line(const boost::system::error_code &err) {
        if (!err) {
            // Check that response is OK.
            std::istream response_stream(&response_);
            std::string http_version;
            response_stream >> http_version;
            unsigned int status_code;
            response_stream >> status_code;
            std::string status_message;
            std::getline(response_stream, status_message);
            if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
                std::cout << "Invalid response\n";
                return;
            }
            if (status_code != 200) {
                std::cout << "Response returned with status code ";
                std::cout << status_code << "\n";
                return;
            }

            // Read the response headers, which are terminated by a blank line.
            boost::asio::async_read(
                    socket_, response_, boost::asio::transfer_at_least(1),
                    boost::bind(&HttpClient::handle_read_headers, this, boost::asio::placeholders::error));
        } else {
            std::cout << "Error: " << err << "\n";
        }
    }

    void handle_read_headers(const boost::system::error_code &err) {
        if (!err || err.value() == 2) {
            std::istream response_stream(&response_);
            std::string header;
            while (std::getline(response_stream, header) && header != "\r") {
                 //std::cout <<"header: "<< header << "\n";
            }

            // Start reading remaining data until EOF.
            boost::asio::async_read(
                    socket_, response_, boost::asio::transfer_at_least(1),
                    boost::bind(&HttpClient::handle_read_content, this, boost::asio::placeholders::error));
        } else {
            std::cout << "Error: " << err.message()<< "\n";
        }
    }

    void handle_read_content(const boost::system::error_code &err) {
        if (!err) {
            //反正返回的数据少，直接读完再说
            //别问我为什么这样，我debug了一整天也没想明白为什么一直报错 asio.misc:2
            //访问别的网站没问题，python和postman访问我的server也没问题，但就这个访问我的server就报错，这个就先这样算了，有缘再改
            boost::asio::async_read(
                    socket_, response_, boost::asio::transfer_at_least(1),
                    boost::bind(&HttpClient::handle_read_content, this, boost::asio::placeholders::error));
        }
        else if (err == boost::asio::error::eof){
            boost::asio::streambuf::const_buffers_type cbt = response_.data();
            response_body_ +=std::string(boost::asio::buffers_begin(cbt), boost::asio::buffers_end(cbt)) ;
        }
        else if (err != boost::asio::error::eof) {
            std::cout << "Error: " << err << "\n";
        }
    }

    tcp::resolver resolver_;
    tcp::socket socket_;
    boost::asio::streambuf request_;
    boost::asio::streambuf response_;
    std::string &response_body_;
};

#endif // HTTP_CLIENT_HPP_