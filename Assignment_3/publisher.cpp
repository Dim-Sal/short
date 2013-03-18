#include "publisher.h"

/*
 * Runs the publisher's (server) io_service
 */
void Publisher::ServerRun(boost::shared_ptr<boost::asio::io_service> io_service)
{
    io_service_ = io_service;
    io_service_set_ = true;  // let PublishData continue

    // loop until the application is stopped manually
    while(!io_service_->stopped())
    {
        try
        {
            boost::system::error_code ec;
            io_service_->run( ec );

            // report any error
            if( ec )
            {
                out_mutex_.lock();
                std::cout << "[" << boost::this_thread::get_id()
                          << "] Error: " << ec << std::endl;
                out_mutex_.unlock();
            }
        }

        // catch any exception
        catch( std::exception & ex )
        {
            out_mutex_.lock();
            std::cout << "[" << boost::this_thread::get_id()
                      << "] Exception: " << ex.what() << std::endl;
            out_mutex_.unlock();
        }
    }
}

/*
 * Adds a subscriber, listens for another
 *  - infinate loop
 *  - loops recursively
 */
void Publisher::AddSubscriber(const boost::system::error_code& error,
                              boost::shared_ptr< boost::asio::ip::tcp::socket > sock,
                              boost::shared_ptr< boost::asio::ip::tcp::acceptor > acceptor)
{

    if( !error )
    {
        // lock any "sockets_.size()" usage/modification
        sub_mutex_.lock();

        out_mutex_.lock();
        std::cout << "\n[" << boost::this_thread::get_id()
                  << "]  >> Subscriber connected \n" << std::endl;
        out_mutex_.unlock();

        // keep subscriber record
        sockets_.push_back(sock);
        write_returned_.push_back(true);
        remove_.push_back(false);

        // listen for the next subscriber, use another socket
        boost::shared_ptr<boost::asio::ip::tcp::socket> next_sock(new boost::asio::ip::tcp::socket(*io_service_));
        acceptor->async_accept(*next_sock,
                               boost::bind(&Publisher::AddSubscriber,
                                           this,
                                           boost::asio::placeholders::error(),
                                           next_sock,
                                           acceptor));

        // unlock any "sockets_.size()" usage/modification
        sub_mutex_.unlock();
    }

    // report any errors
    else
    {
        out_mutex_.lock();
        std::cout << "[" << boost::this_thread::get_id()
                  << "] Error: " << error << std::endl;
        out_mutex_.unlock();
    }
}

/*
 * Handles asyncronous write operations
 *  - Reports message deliverry for each subscriber's socket
 *  - Signals any subscriber disconnections
 */
void Publisher::WriteHandler(const boost::system::error_code& error,
                             std::size_t bytes_transferred,
                             int i)
{
    // report successful delivery
    if(bytes_transferred > 0)
    {
        out_mutex_.lock();
        std::cout << "[" << boost::this_thread::get_id()
                  <<  "] ---------- message sent ----------" << std::endl;
        out_mutex_.unlock();
    }

    // flag subscriber disconnection
    else
    {
        remove_[i] = true;
    }

    write_returned_[i] = true;  // flag for this socket's read trial completion
}

/*
 * Sends a string message to all connected subscribers
 */
void Publisher::PublishData()
{
    // prepare the message in the form: "From pub_x [thr_ID: xxxxxxxx] "
    std::string message("");
    message.append("From ");
    message.append(id_);
    message.append(" [thr_ID: ");
    message.append(boost::lexical_cast<std::string>(boost::this_thread::get_id()));
    message.append("] ");

    // initialise the sleep duration
    int sleep_duration = 0;

    // wait for the first connection
    while(sockets_.empty() && !io_service_->stopped()) {}

    // publish even if all subscribers are disconnected (they can re-connect)
    while (!io_service_->stopped())
    {
        // lock any "sockets_.size()" usage/modification
        sub_mutex_.lock();

        for (unsigned i=0; i<sockets_.size(); i++)
        {
            if (write_returned_[i])  // if this socket's WriteHandler invocation returned
            {
                write_returned_[i] = false;

                // send message through the sockets_
                boost::asio::async_write(*sockets_[i],
                                         boost::asio::buffer(message, 30),
                                         boost::bind(&Publisher::WriteHandler,
                                                     this,
                                                     boost::asio::placeholders::error,
                                                     boost::asio::placeholders::bytes_transferred,
                                                     i));

            }
        }

        // unlock any "sockets_.size()" usage/modification
        sub_mutex_.unlock();


        // lock any "sockets_.size()" usage/modification
        sub_mutex_.lock();

        // remove disconnected subs sockets (based on the "remove_" flags)
        for (unsigned i=0; i<remove_.size(); i++)
        {
            if (remove_[i])
            {
                out_mutex_.lock();
                std::cout << "\n[" << boost::this_thread::get_id()
                          <<  "] << Subscriber disconnected \n" << std::endl;
                out_mutex_.unlock();

                // erase all pub related elements
                sockets_.erase(sockets_.begin()+i);
                write_returned_.erase(write_returned_.begin()+i);
            }
        }

        // erase the flags too
        remove_.erase(std::remove(remove_.begin(), remove_.end(), true), remove_.end());

        // unlock any "sockets_.size()" usage/modification
        sub_mutex_.unlock();

        // report connection count
        std::cout << "[" << boost::this_thread::get_id()
                  <<  "] Connected sockets = " << sockets_.size() << std::endl;

        // sleep so as to simulate random message emission
        if (rand_intervals_)
        {
            srand(time(NULL));  // random seed initialisation
            sleep_duration = (rand() % upper_bound_ms_ + 1) * 1000;
            usleep(sleep_duration);
        }

        // sleep for a while (fixed intervals)...
        else
        {
            usleep(upper_bound_ms_ * 1000);
        }
    }
}
