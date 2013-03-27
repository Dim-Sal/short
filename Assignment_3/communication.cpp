#include "communication.h"

/*
 * Initialises the io_service
 */
void Communication::InitIoService()
{
    boost::shared_ptr<boost::asio::io_service> io_service(new boost::asio::io_service);
    io_service_ = io_service;

    boost::shared_ptr<boost::asio::io_service::work> work(new boost::asio::io_service::work(*io_service_));
    work_ = work;

    std::cout << "[" << boost::this_thread::get_id()
              << "] Press <RETURN> to exit." << std::endl;
}

/*
 * Launches the communication threads
 *  - @thread_count: the number of threads to be used
 */
void Communication::LaunchThreads(int thread_count)
{
    // create and launch the communication threads
    for (int i=0; i<thread_count; i++)
    {
        communication_threads_.create_thread(boost::bind(&Communication::Run,
                                                         this));
    }
}

/*
 * Sets up listening for and accepting subs
 *  - @port: the communication port on localhost
 */
void Communication::Accept(std::string port)
{
    boost::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor(new boost::asio::ip::tcp::acceptor(*io_service_));
    boost::shared_ptr<boost::asio::ip::tcp::socket> sock(new boost::asio::ip::tcp::socket(*io_service_));

    acceptor_ = acceptor;

    try
    {
        boost::asio::ip::tcp::resolver resolver(*io_service_);
        boost::asio::ip::tcp::resolver::query query("127.0.0.1", port);
        boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve(query);

        acceptor_->open(endpoint.protocol());
        acceptor_->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor_->bind(endpoint);
        acceptor_->listen(boost::asio::socket_base::max_connections);

        std::cout << "[" << boost::this_thread::get_id()
                  << "] Listening on: " << endpoint << std::endl;

        acceptor_->async_accept(*sock, boost::bind(&Communication::AddClient,
                                                   this,
                                                   boost::asio::placeholders::error(),
                                                   sock));
    }

    // report any exceptions
    catch (std::exception & ex)
    {
        std::cout << "[" << boost::this_thread::get_id()
                  << "] Exception: " << ex.what() << std::endl;
    }
}

/*
 * Writes to the localhost
 *  - @str: the data to be writen
 *  - @length: the string length
 */
void Communication::Write(std::string str, int length)
{
    // lock any "sockets_.size()" usage/modification
    com_mutex_.lock();
    iter_pending_ = true;

    for (unsigned i=0; i<sockets_.size(); i++)
    {
        if (handle_returned_[i])  // if this socket's WriteHandler invocation returned
        {
            handle_returned_[i] = false;

            // send message through the sockets_
            boost::asio::async_write(*sockets_[i],
                                     boost::asio::buffer(str, length),
                                     boost::bind(&Communication::WriteHandler,
                                                 this,
                                                 boost::asio::placeholders::error,
                                                 boost::asio::placeholders::bytes_transferred,
                                                 i));

        }
    }

    iter_pending_ = false;
    pending_iter_.notify_one();

    // unlock any "sockets_.size()" usage/modification
    com_mutex_.unlock();

    // wait for all write operations to finish
    while (!all_handles_returned_)
    {
        all_handles_returned_ = true;

        // check if all write operations returned
        for (unsigned i=0; i<handle_returned_.size(); i++)
        {
            all_handles_returned_ &= handle_returned_[i];
        }
    }

    all_handles_returned_ = false;

    // lock any "sockets_.size()" usage/modification
    com_mutex_.lock();

    // remove disconnected clients' elements (based on the "remove_" flags)
    for (unsigned i=0; i<remove_.size(); i++)
    {
        if (remove_[i])
        {
            out_mutex_.lock();
            std::cout << "\n[" << boost::this_thread::get_id()
                      <<  "] << Subscriber disconnected \n" << std::endl;
            out_mutex_.unlock();

            // erase all client related elements
            sockets_[i]->close();
            sockets_.erase(sockets_.begin()+i);
            handle_returned_.erase(handle_returned_.begin()+i);
        }
    }

    // erase the flags too
    remove_.erase(std::remove(remove_.begin(), remove_.end(), true), remove_.end());

    // unlock any "sockets_.size()" usage/modification
    com_mutex_.unlock();

    // report connection count
    out_mutex_.lock();
    std::cout << "[" << boost::this_thread::get_id()
              <<  "] Connected sockets = " << sockets_.size() << std::endl;
    out_mutex_.unlock();
}

/*
 * Connects to specified server(s)
 *  - @servers_count: the number of servers to be connected to
 *  - @servers: a vector of Connection objects (for ip, port, pub_id)
 */
void Communication::Connect(int servers_count, std::vector<Connection> servers)
{
    // connect with the server(s)
    for (int i=0; i<servers_count; i++)
    {
        boost::shared_ptr<boost::asio::ip::tcp::socket> sock(new boost::asio::ip::tcp::socket(*io_service_));

        try
        {
            // get the pub from the options
            Connection con = servers[i];

            // try to connect
            boost::asio::ip::tcp::resolver resolver(*io_service_);
            boost::asio::ip::tcp::resolver::query query(con.ip, con.port);
            boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve(query);
            boost::asio::ip::tcp::endpoint endpoint = *iterator;

            sock->async_connect(endpoint,
                                boost::bind(&Communication::AddServer,
                                            this,
                                            boost::asio::placeholders::error(),
                                            sock,
                                            endpoint,
                                            con.pub_id));

            std::cout << "[" << boost::this_thread::get_id()
                      << "] Connecting to: " << endpoint << std::endl;
        }

        // catch any exception
        catch (std::exception & ex)
        {
            std::cout << "[" << boost::this_thread::get_id()
                      << "] Exception: " << ex.what() << std::endl;
        }
    }
}

/*
 * Reads from server(s)' remote host
 *  - @client_id: the client's identifier
 */
void Communication::Read(std::string client_id)
{
    // lock any "sockets_.size()" usage/modification
    com_mutex_.lock();
    iter_pending_ = true;

    for (unsigned i=0; i<sockets_.size(); i++)
    {
        if (handle_returned_[i])  // if this socket's ReadHandler invocation returned
        {
            handle_returned_[i] = false;
            boost::asio::async_read(*sockets_[i],
                                    boost::asio::buffer(buf_[i], 30),
                                    boost::bind(&Communication::ReadHandler,
                                                this,
                                                boost::asio::placeholders::error,
                                                boost::asio::placeholders::bytes_transferred,
                                                client_id,
                                                i));
        }
    }

    iter_pending_ = false;
    pending_iter_.notify_one();

    // unlock any "sockets_.size()" usage/modification
    com_mutex_.unlock();

    // wait for all read operations to finish
    while (!all_handles_returned_)
    {
        all_handles_returned_ = true;

        // check if all read operations returned
        for (unsigned i=0; i<handle_returned_.size(); i++)
        {
            all_handles_returned_ &= handle_returned_[i];
        }
    }

    all_handles_returned_ = false;

    // lock any "sockets_.size()" usage/modification
    com_mutex_.lock();

    // remove disconnected pubs (based on the "remove_" flags)
    for (unsigned i=0; i<remove_.size(); i++)
    {
        if (remove_[i])
        {
            out_mutex_.lock();
            std::cout << "\n[" << boost::this_thread::get_id()
                      << "] << " << servers_[i]<< " is now disconnected \n" << std::endl;
            out_mutex_.unlock();

            // erase all server related elements
            sockets_[i]->close();
            sockets_.erase(sockets_.begin()+i);
            servers_.erase(servers_.begin()+i);
            buf_.erase(buf_.begin()+i);
            handle_returned_.erase(handle_returned_.begin()+i);
        }
    }

    // erase the flags too
    remove_.erase(std::remove(remove_.begin(), remove_.end(), true), remove_.end());

    // unlock any "sockets_.size()" usage/modification
    com_mutex_.unlock();
}

/*
 * Provides a clean exit
 *  - waits for <RETURN> keystroke to exit
 *  - @role: "server" or "client"
 */
void Communication::PrepareForExit(std::string role)
{
    if (role == "server")
    {
        boost::system::error_code ec;
        acceptor_->close(ec);
    }

    for (unsigned i=0; i<sockets_.size(); i++)
    {
        boost::system::error_code ec;
        sockets_[i]->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        sockets_[i]->close(ec);
    }

    io_service_->stop();
    communication_threads_.join_all();
}

/*
 * Reports that no server is now connected
 */
void Communication::NoServerReport()
{
        out_mutex_.lock();
        std::cout << "\n[" << boost::this_thread::get_id()
                  << "] All publishers disconnected - press <RETURN> to exit \n" << std::endl;
        out_mutex_.unlock();
}



// ----- Private functions -----

/*
 * Runs the io_service
 */
void Communication::Run()
{
    // loop until the application is stopped manually
    while (Running())
    {
        try
        {
            boost::system::error_code ec;
            io_service_->run(ec);

            // report any error
            if (ec)
            {
                out_mutex_.lock();
                std::cout << "[" << boost::this_thread::get_id()
                          << "] Error: " << ec << std::endl;
                out_mutex_.unlock();
            }
        }

        // catch any exception
        catch (std::exception & ex)
        {
            out_mutex_.lock();
            std::cout << "[" << boost::this_thread::get_id()
                      << "] Exception: " << ex.what() << std::endl;
            out_mutex_.unlock();
        }
    }
}

/*
 * Adds a client, listens for another
 *  - infinate loop
 *  - loops recursively
 */
void Communication::AddClient(const boost::system::error_code& error,
                              boost::shared_ptr<boost::asio::ip::tcp::socket> sock)
{
    if (!error)
    {
        // lock any "sockets_.size()" usage/modification
        com_mutex_.lock();

        out_mutex_.lock();
        std::cout << "\n[" << boost::this_thread::get_id()
                  << "]  >> Subscriber connected \n" << std::endl;
        out_mutex_.unlock();

        // keep subscriber record
        sockets_.push_back(sock);
        handle_returned_.push_back(true);
        remove_.push_back(false);

        // listen for the next subscriber, use another socket
        boost::shared_ptr<boost::asio::ip::tcp::socket> next_sock(new boost::asio::ip::tcp::socket(*io_service_));
        acceptor_->async_accept(*next_sock,
                               boost::bind(&Communication::AddClient,
                                           this,
                                           boost::asio::placeholders::error(),
                                           next_sock));

        // unlock any "sockets_.size()" usage/modification
        com_mutex_.unlock();

        // inform on first connection
        if (sockets_.size() == 1)
        {
            pending_first_->notify_one();
        }
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
 *  - reports message deliverry for each client's socket
 *  - signals any client disconnections
 */
void Communication::WriteHandler(const boost::system::error_code& error,
                             std::size_t bytes_transferred,
                             int i)
{
    boost::mutex::scoped_lock write_lock(com_mutex_);

    if (iter_pending_)
    {
        pending_iter_.wait(write_lock);
    }

    // report successful delivery
    if(bytes_transferred > 0)
    {
        out_mutex_.lock();
        std::cout << "[" << boost::this_thread::get_id()
                  <<  "] ---------- message sent ----------" << std::endl;
        out_mutex_.unlock();
    }

    // flag client disconnection
    else
    {
        remove_[i] = true;
    }

    // flag for this socket's read trial completion
    handle_returned_[i] = true;
}

/*
 * Connects to a server
 *  - on success: adds all server related elements
 *  - infinate loop
 *  - loops recursively on error condition: "connection refused"
 */
void Communication::AddServer(const boost::system::error_code& error,
                              boost::shared_ptr<boost::asio::ip::tcp::socket> soc,
                              boost::asio::ip::tcp::endpoint endpoint,
                              std::string server)
{
    // remote host is now connected
    if (!error)
    {
        // lock any "sockets_.size()" usage/modification
        com_mutex_.lock();

        // add a pub, along with all related pub elements
        sockets_.push_back(soc);
        servers_.push_back(server);
        buf_.push_back(*new boost::array<char, 30>);
        buf_[buf_.size()-1].assign(0);  // initialise the buffer
        handle_returned_.push_back(true);
        remove_.push_back(false);

        // report successful connection
        out_mutex_.lock();
        std::cout << "\n[" << boost::this_thread::get_id()
                  << "] >> Connection to " << endpoint << " succeded \n" << std::endl;
        out_mutex_.unlock();

        // unlock any "sockets_.size()" usage/modification
        com_mutex_.unlock();

        // inform on first connection
        if (sockets_.size() == 1)
        {
            pending_first_->notify_one();
        }
    }

    // remote host is not connected yet
    else if (error.message() == "Connection refused")
    {
        // try again (recursively)
        soc.reset(new boost::asio::ip::tcp::socket(*io_service_));
        soc->async_connect(endpoint,
                           boost::bind(&Communication::AddServer,
                                       this,
                                       boost::asio::placeholders::error(),
                                       soc,
                                       endpoint,
                                       server));
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
 * Handles asyncronous read operations
 *  - displays message from each server's socket
 *  - signals any server disconnections
 */
void Communication::ReadHandler(const boost::system::error_code& error,
                                std::size_t bytes_transferred,
                                std::string client_id,
                                int i)
{
    boost::mutex::scoped_lock read_lock(com_mutex_);

    if (iter_pending_)
    {
        pending_iter_.wait(read_lock);
    }

    // successful read
    if (bytes_transferred == 30)
    {
        // append this object's id and this thread's id to the message
        std::string message = buf_[i].data();
        message = message.substr(0, 30);
        message.append("to ");
        message.append(client_id);
        message.append(" [thr_ID: ");
        message.append(boost::lexical_cast<std::string>(boost::this_thread::get_id()));
        message.append("]");

        out_mutex_.lock();

        // display message
        std::cout << message << std::endl;

        // clear the read buffer
        buf_[i].assign(0);

        out_mutex_.unlock();
    }

    else  // server is disconnected and will be removed
    {
        remove_[i] = true;
    }

    // flag for this socket's read trial completion
    handle_returned_[i] = true;
}

