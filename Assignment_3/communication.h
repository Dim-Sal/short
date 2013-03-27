#ifndef COMMUNICATION_H
#define COMMUNICATION_H
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/asio.hpp>
#include "options.h"

/*
 * Network wrapper class for client/server operations
 */
class Communication
{
    public:
        Communication() : all_handles_returned_(false),
                          pending_first_(new boost::condition_variable) {}

        // accessors
        boost::shared_ptr<boost::asio::io_service> io_service() {return io_service_;}
        boost::shared_ptr<boost::condition_variable> pending_first() {return pending_first_;}

        // common funtionality for pubs/subs
        void InitIoService();
        void LaunchThreads(int thread_count);
        void PrepareForExit(std::string role);
        bool Running() {return !io_service_->stopped();}
        bool NoConnections() {return sockets_.size() == 0;}

        // pub only
        void Accept(std::string port);
        void Write(std::string str, int length);

        // sub only
        void Connect(int servers_count, std::vector<Connection> servers);
        void Read(std::string client_id);
        void NoServerReport();


    private:
        // common funtionality for pubs/subs
        void Run();

        // pub only
        void AddClient(const boost::system::error_code& error,
                       boost::shared_ptr<boost::asio::ip::tcp::socket> sock);
        void WriteHandler(const boost::system::error_code& error,
                          std::size_t bytes_transferred,
                          int i);

        // sub only
        void AddServer(const boost::system::error_code& error,
                       boost::shared_ptr<boost::asio::ip::tcp::socket> soc,
                       boost::asio::ip::tcp::endpoint endpoint,
                       std::string server);
        void ReadHandler(const boost::system::error_code& error,
                         std::size_t bytes_transferred,
                         std::string client_id,
                         int i);

        // common boost::asio members
        boost::shared_ptr<boost::asio::io_service> io_service_;
        boost::shared_ptr<boost::asio::io_service::work> work_;
        boost::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor_;
        std::vector< boost::shared_ptr<boost::asio::ip::tcp::socket> > sockets_;

        // common thread-safety members
        boost::thread_group communication_threads_;
        boost::condition_variable pending_iter_;  // pending sockets iteration
        boost::mutex out_mutex_,                  // terminal output mutex
                     com_mutex_;                  // pub/sub mutex

        // common utility flags
        bool iter_pending_,
             all_handles_returned_;

        // common utility vectors
        std::vector<bool> remove_,
                          handle_returned_;

        // common condition variable for the first pending connection
        boost::shared_ptr<boost::condition_variable> pending_first_;

        // sub only
        std::vector< boost::array<char, 30> > buf_;
        std::vector<std::string> servers_;
};

#endif // COMMUNICATION_H
