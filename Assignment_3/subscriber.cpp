#include "subscriber.h"

/*
 * Runs the subscriber's (client) io_service
 */
void Subscriber::ClientRun(boost::shared_ptr<boost::asio::io_service> io_service)
{
    io_service_ = io_service;
    io_service_set_ = true;  // let DisplayMessage continue

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
 * Connects to a publisher
 *  - on success: adds all pub related elements
 *  - infinate loop
 *  - loops recursively on error condition: connection refused
 */
void Subscriber::AddPublisher(const boost::system::error_code& error,
                              boost::shared_ptr<boost::asio::ip::tcp::socket> soc,
                              boost::asio::ip::tcp::endpoint endpoint,
                              std::string pub_id)
{
    // remote host is now connected
    if (!error)
    {
        // notify about pending pub to be added
        pending_add_ = true;

        // lock any "sockets_.size()" usage/modification
        pub_mutex_.lock();

        // add a pub, along with all related pub elements
        sockets_.push_back(soc);
        pubs_.push_back(pub_id);
        buf_.push_back(*new boost::array<char, 30>);
        read_returned_.push_back(true);
        remove_.push_back(false);

        // report successful connection
        out_mutex_.lock();
        std::cout << "\n[" << boost::this_thread::get_id()
                  << "] >> Connection to " << endpoint << " succeded \n" << std::endl;
        out_mutex_.unlock();

        // no more pending add
        pending_add_ = false;

        // unlock any "sockets_.size()" usage/modification
        pub_mutex_.unlock();
    }

    // remote host is not connected yet
    else if (error.message() == "Connection refused")
    {
        // try again (recursively)
        soc.reset(new boost::asio::ip::tcp::socket(*io_service_));
        soc->async_connect(endpoint,
                           boost::bind(&Subscriber::AddPublisher,
                                       this,
                                       boost::asio::placeholders::error(),
                                       soc,
                                       endpoint,
                                       pub_id));
    }

    else // report error
    {
        out_mutex_.lock();
        std::cout << "[" << boost::this_thread::get_id()
                  << "] Error: " << error.message() << std::endl;
        out_mutex_.unlock();
    }
}

/*
 * Handles read operations
 *  - loops recursively with DisplayMessage member
 */
void Subscriber::ReadHandler(const boost::system::error_code& error,
                             std::size_t bytes_transferred,
                             int i)
{
    // successful read
    if (bytes_transferred > 0)
    {
        // append this object's id and this thread's id to the message
        std::string message = "";
        message.append(" to ");
        message.append(id_);
        message.append(" [thr_ID: ");
        message.append(boost::lexical_cast<std::string>(boost::this_thread::get_id()));
        message.append("] ");

        // display message
        out_mutex_.lock();
        std::cout << buf_[i].data() << message << std::endl;
        out_mutex_.unlock();

        // clear the read buffer
        buf_[i].assign(0);
    }

    else  // publisher (server) is disconnected and will be removed
    {
        remove_[i] = true;
    }

    read_returned_[i] = true;  // flag for this socket's read trial completion
}

/*
 * Displays message(s) from publisher(s)
 *  - loops till all pubs are disconnected or sub exits (manually)
 *  - thread-safe
 */
void Subscriber::DisplayMessage()
{
    // wait for the first remote host to connect
    while (sockets_.empty() && !io_service_->stopped()){}

    all_reads_returned_ = false;

    // read asyncronously from all pubs
    while (!sockets_.empty() && !io_service_->stopped())
    {
        // lock any "sockets_.size()" usage/modification
        pub_mutex_.lock();

        for (unsigned i=0; i<sockets_.size(); i++)
        {
            if (read_returned_[i])  // if this socket's ReadHandler invocation returned
            {    
                read_returned_[i] = false;
                boost::asio::async_read(*sockets_[i],
                                        boost::asio::buffer(buf_[i], 30),
                                        boost::bind(&Subscriber::ReadHandler,
                                                    this,
                                                    boost::asio::placeholders::error,
                                                    boost::asio::placeholders::bytes_transferred,
                                                    i));
            }
        }

        // unlock any "sockets_.size()" usage/modification
        pub_mutex_.unlock();

        // wait for a pending add to finish (if any)
        while (pending_add_) {}

        // lock any "sockets_.size()" usage/modification
        pub_mutex_.lock();

        // wait for all read operations to finish
        while (!all_reads_returned_)
        {
            all_reads_returned_ = true;

            // check if all read operations returned
            for (unsigned i=0; i<read_returned_.size(); i++)
            {
                all_reads_returned_ &= read_returned_[i];
            }
        }

        // unlock any "sockets_.size()" usage/modification
        pub_mutex_.unlock();

        // lock any "sockets_.size()" usage/modification
        pub_mutex_.lock();

        // remove disconnected pubs (based on the "remove_" flags)
        for (unsigned i=0; i<remove_.size(); i++)
        {
            if (remove_[i])
            {

                out_mutex_.lock();
                std::cout << "\n[" << boost::this_thread::get_id()
                          << "] << " << pubs_[i]<< " is now disconnected \n" << std::endl;
                out_mutex_.unlock();

                // erase all pub related elements
                sockets_.erase(sockets_.begin()+i);
                pubs_.erase(pubs_.begin()+i);
                buf_.erase(buf_.begin()+i);
                read_returned_.erase(read_returned_.begin()+i);
            }
        }

        // erase the flags too
        remove_.erase(std::remove(remove_.begin(), remove_.end(), true), remove_.end());

        all_reads_returned_ = false;

        // lock any "sockets_.size()" usage/modification
        pub_mutex_.unlock();
    }

    // waiting for <RETURN> to exit
    out_mutex_.lock();
    std::cout << "\n[" << boost::this_thread::get_id()
              << "] All publishers disconnected - press <RETURN> to exit \n" << std::endl;
    out_mutex_.unlock();
}
