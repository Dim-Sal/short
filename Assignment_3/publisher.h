#ifndef PUBLISHER_H
#define PUBLISHER_H
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include "subscriber.h"

/*
 * Class representation for publishers
 */
class Publisher
{
    public:
        Publisher() : io_service_set_(false) {}

        // starts the io_service
        void ServerRun(boost::shared_ptr< boost::asio::io_service > io_service);

        // connects subscriber(s)
        void AddSubscriber(const boost::system::error_code & ec,
                           boost::shared_ptr< boost::asio::ip::tcp::socket > sock,
                           boost::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor);

        // writes to the pub's ip:port
        void PublishData(); // sends a string to subscribers

        // accessors
        std::vector< boost::shared_ptr<boost::asio::ip::tcp::socket> > sockets(){return sockets_;}
        bool io_service_set(){return io_service_set_;}

        // mutaturs
        void set_id(std::string id) {id_ = id;}
        void set_rand_intervals(bool is_random){rand_intervals_ = is_random;}
        void set_upper_bound_ms(int up_b){upper_bound_ms_ = up_b;}

    private:
        // handles write operations
        void WriteHandler(const boost::system::error_code& error,
                          std::size_t bytes_transferred,
                          int i);

        // the subscribers' sockets
        std::vector< boost::shared_ptr<boost::asio::ip::tcp::socket> > sockets_;

        // utility vectors
        std::vector<bool> remove_,
                          write_returned_;

        boost::mutex out_mutex_, // terminal output mutex
                     sub_mutex_; // sub add/remove/iterate mutex

        // other private data members
        boost::shared_ptr<boost::asio::io_service> io_service_;
        int upper_bound_ms_;
        std::string id_;
        bool rand_intervals_,
             io_service_set_;
};

#endif // PUBLISHER_H
