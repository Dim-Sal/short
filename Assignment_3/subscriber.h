#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>

/*
 * Class representation for subscribers
 */
class Subscriber
{
    public:
        Subscriber() : io_service_set_(false),
                       all_reads_returned_(true),
                       pending_add_(true) {}

        // starts the io_service
        void ClientRun(boost::shared_ptr<boost::asio::io_service> io_service);

        // connects to one or more publishers
        void AddPublisher(const boost::system::error_code& error,
                       boost::shared_ptr<boost::asio::ip::tcp::socket> soc,
                       boost::asio::ip::tcp::endpoint endpoint,
                       std::string pub_id);

        // displays message(s) from publisher(s)
        void DisplayMessage();

        // accessors
        bool io_service_set(){return io_service_set_;}
        std::vector< boost::shared_ptr<boost::asio::ip::tcp::socket> > sockets(){return sockets_;}

        // id mutator
        void set_id(std::string id) {id_ = id;}

    private:
        // handles read operations
        void ReadHandler(
              const boost::system::error_code& error,  // Result of operation.
              std::size_t bytes_transferred,           // Number of bytes sent.
              int i);

        // the publishers' sockets
        std::vector< boost::shared_ptr<boost::asio::ip::tcp::socket> > sockets_;

        std::vector< boost::array<char, 30> > buf_;  // read buffers
        std::vector<std::string> pubs_;              // pub identifiers

        // utility vectors
        std::vector<bool> remove_,
                          read_returned_;

        boost::mutex out_mutex_,  // terminal output mutex
                     pub_mutex_,  // pub add/remove/iterate mutex
                     add_mutex_;  // pending add mutex

        // other private data members
        boost::shared_ptr<boost::asio::io_service> io_service_;
        std::string id_;
        bool io_service_set_,
             pending_add_,         // flags a pending pub to be added
             all_reads_returned_;  // flags completion of reading from all pubs
};

#endif // SUBSCRIBER_H
