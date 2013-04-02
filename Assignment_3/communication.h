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
        // constructor initialisations
        Communication() : is_pending_add_(true),
                          is_picked_message_(true),
                          is_set_message_(false),
                          pending_first_condition_(new boost::condition_variable),
                          setting_message_condition_(new boost::condition_variable),
                          getting_message_condition_(new boost::condition_variable) {}

        // accessors
        boost::shared_ptr<boost::asio::io_service> io_service() {return io_service_;}
        boost::shared_ptr<boost::condition_variable> pending_first_condition() {return pending_first_condition_;}
        boost::shared_ptr<boost::condition_variable> setting_message_condition() {return setting_message_condition_;}
        boost::shared_ptr<boost::condition_variable> getting_message_condition() {return getting_message_condition_;}
        bool is_set_message() {return is_set_message_;}
        std::string message() {return message_;}

        // mutators
        void set_is_picked_message(bool is_read) {is_picked_message_ = is_read;}
        void set_is_set_message(bool is_set) {is_set_message_ = is_set;}

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
        void Read();
        void NoServerReport();


    private:
        // common funtionality for pubs/subs
        void Run();

        // pub only
        void AddClient(const boost::system::error_code& error,
                       boost::shared_ptr<boost::asio::ip::tcp::socket> sock);
        void WriteHandler(const boost::system::error_code& error,
                          std::size_t bytes_transferred,
                          boost::shared_ptr<boost::asio::ip::tcp::socket> soc);

        // sub only
        void AddServer(const boost::system::error_code& error,
                       boost::shared_ptr<boost::asio::ip::tcp::socket> soc,
                       boost::asio::ip::tcp::endpoint endpoint,
                       boost::shared_ptr<std::string> server,
                       boost::shared_ptr<std::string> ip,
                       boost::shared_ptr<std::string> port);
        void ReadHandler(const boost::system::error_code& error,
                         std::size_t bytes_transferred,
                         boost::shared_ptr <boost::array<char, 30> > buffer,
                         boost::shared_ptr <std::string> server_name,
                         boost::shared_ptr<boost::asio::ip::tcp::socket> soc);

        // common boost::asio members
        boost::shared_ptr<boost::asio::io_service> io_service_;
        boost::shared_ptr<boost::asio::io_service::work> work_;
        std::vector< boost::shared_ptr<boost::asio::ip::tcp::socket> > sockets_;

        // common thread-safety members
        boost::thread_group communication_threads_; // io_service threads
        boost::mutex out_mutex_,                    // terminal output mutex
                     com_mutex_;                    // pub/sub mutex

        // pub only
        boost::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor_;

        // sub only
        bool is_pending_add_,           // flag for a pending server add
             is_picked_message_,        // flag for message_ process
             is_set_message_;           // flag for message_ assignment

        std::string message_;           // shared resource for reads
        boost::mutex message_mutex_;    // mutex for message_ access
        std::vector<bool> is_reading_;  // flags for recently added server sockets
        std::vector< boost::shared_ptr <boost::array<char, 30> > > buf_;         // pointers to read buffers
        std::vector< boost::shared_ptr <std::string> > servers_;                 // pointers to server names
        std::vector<Connection> connections_;                                    // connection data for servers
        boost::condition_variable pending_add_condition_;                        // condition for pending add
        boost::shared_ptr<boost::condition_variable> pending_first_condition_,   // condition for first connection
                                                     setting_message_condition_, // condition for setting message
                                                     getting_message_condition_; // condition for getting message

};

#endif // COMMUNICATION_H
