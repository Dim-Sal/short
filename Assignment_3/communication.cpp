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

    for (unsigned i=0; i<sockets_.size(); i++)
    {
        // send message through the sockets_
        boost::asio::async_write(*sockets_[i],
                                 boost::asio::buffer(str, length),
                                 boost::bind(&Communication::WriteHandler,
                                             this,
                                             boost::asio::placeholders::error,
                                             boost::asio::placeholders::bytes_transferred,
                                             sockets_[i]));
    }

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

        // get the pub from the options
        Connection con = servers[i];
        boost::shared_ptr<std::string> server_name(new std::string(con.pub_id));

        try
        {
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
                                            server_name));

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
 * Reads from server(s)
 */
void Communication::Read()
{
    while (!NoConnections() && Running())
    {
        boost::mutex::scoped_lock iter_lock(com_mutex_);

        // wait for a new server to be added
        while (is_pending_add_)
        {
            pending_add_condition_.wait(iter_lock);
        }

        for (unsigned i=0; i<sockets_.size(); i++)
        {
            // first async_read for sockets_[i]
            if (!is_reading_[i])
            {
                boost::asio::async_read(*sockets_[i],
                                        boost::asio::buffer(*buf_[i], 30),
                                        boost::bind(&Communication::ReadHandler,
                                                    this,
                                                    boost::asio::placeholders::error,
                                                    boost::asio::placeholders::bytes_transferred,
                                                    buf_[i],
                                                    servers_[i],
                                                    sockets_[i]));
            }

            is_reading_[i] = true;
        }

        is_pending_add_ = true;
    }
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
        sockets_[i]->cancel();
        sockets_[i]->shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
        sockets_[i]->close(ec);
    }

    // make sure all threads join
    work_.reset();
    io_service_->stop();
    pending_first_condition_->notify_all();

    if (role == "client")
    {
        is_pending_add_ = false;
        is_set_message_ = true;
        pending_add_condition_.notify_all();
        setting_message_condition_->notify_all();
        getting_message_condition_->notify_all();
    }

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

        // listen for the next subscriber, use another socket
        boost::shared_ptr<boost::asio::ip::tcp::socket> next_sock(new boost::asio::ip::tcp::socket(*io_service_));
        acceptor_->async_accept(*next_sock,
                               boost::bind(&Communication::AddClient,
                                           this,
                                           boost::asio::placeholders::error(),
                                           next_sock));

        // unlock any "sockets_.size()" usage/modification
        com_mutex_.unlock();
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
 *  - removes and reports any disconnected clients
 */
void Communication::WriteHandler(const boost::system::error_code& error,
                             std::size_t bytes_transferred,
                             boost::shared_ptr<boost::asio::ip::tcp::socket> soc)
{
    com_mutex_.lock();

    // report successful delivery
    if(bytes_transferred == 30)
    {
        out_mutex_.lock();
        std::cout << "[" << boost::this_thread::get_id()
                  <<  "] ---------- message sent ----------" << std::endl;
        out_mutex_.unlock();
    }

    // remove client
    else
    {
        soc->close();

        // remove disconnected clients' socket
        for (unsigned i=0; i<sockets_.size(); i++)
        {
            if (!sockets_[i]->is_open())
            {
                sockets_.erase(sockets_.begin()+i);

                // report disconnection
                out_mutex_.lock();
                std::cout << "\n[" << boost::this_thread::get_id()
                          <<  "] << Subscriber disconnected \n" << std::endl;
                out_mutex_.unlock();
            }
        }
    }

    com_mutex_.unlock();
}

/*
 * Connects to a server
 *  - on success: adds all server related elements
 *  - on failure: retries
 */
void Communication::AddServer(const boost::system::error_code& error,
                              boost::shared_ptr<boost::asio::ip::tcp::socket> soc,
                              boost::asio::ip::tcp::endpoint endpoint,
                              boost::shared_ptr<std::string> server)
{
    // remote host is now connected
    if (!error)
    {
        // lock any "sockets_.size()" usage/modification
        com_mutex_.lock();

        // add a pub, along with all related pub elements
        sockets_.push_back(soc);
        servers_.push_back(server);
        boost::shared_ptr< boost::array<char, 30> > new_buf_ptr(new boost::array<char, 30>);
        new_buf_ptr->assign(0);
        buf_.push_back(new_buf_ptr);
        is_reading_.push_back(false);

        // report successful connection
        out_mutex_.lock();
        std::cout << "\n[" << boost::this_thread::get_id()
                  << "] >> Connection to " << *server
                  << " at " << endpoint << " succeded \n" << std::endl;
        out_mutex_.unlock();

        // let the read operations begin/continue
        is_pending_add_ = false;
        pending_add_condition_.notify_one();

        // unlock any "sockets_.size()" usage/modification
        com_mutex_.unlock();

        // inform on first connection
        if (sockets_.size() == 1)
        {
            pending_first_condition_->notify_one();
        }
    }

    // remote host is not connected yet
    else if (error.message() == "Connection refused")
    {
        // try again
        usleep(100000);
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
 *  - sets message_ from server(s)
 *  - removes any disconnected servers
 */
void Communication::ReadHandler(const boost::system::error_code& error,
                                std::size_t bytes_transferred,
                                boost::shared_ptr <boost::array<char, 30> > buffer,
                                boost::shared_ptr <std::string> server_name,
                                boost::shared_ptr<boost::asio::ip::tcp::socket> soc)
{
    com_mutex_.lock();

    // successful read
    if (bytes_transferred == 30)
    {
        boost::mutex::scoped_lock message_lock(message_mutex_);

        // let an unread message be read
        while (!is_picked_message_)
        {
            getting_message_condition_->wait(message_lock);
        }

        // write message_ from buffer
        message_ = buffer->data();
        message_ = message_.substr(0, 30);

        // clear the read buffer
        buffer->assign(0);

        is_picked_message_ = false;
        is_set_message_ = true;
        setting_message_condition_->notify_one();

        // async_read next message on this server
        boost::asio::async_read(*soc,
                                boost::asio::buffer(*buffer, 30),
                                boost::bind(&Communication::ReadHandler,
                                            this,
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred,
                                            buffer,
                                            server_name,
                                            soc));
    }

    else  // server is disconnected and will be removed
    {
        out_mutex_.lock();
        std::cout << "\n[" << boost::this_thread::get_id()
                  << "] << " << *server_name << " is now disconnected \n" << std::endl;
        out_mutex_.unlock();

        // close and erase all server related elements
        soc->close();

        for (unsigned i=0; i<sockets_.size(); i++)
        {
            if (!sockets_[i]->is_open())
            {
                sockets_.erase(sockets_.begin()+i);
                servers_.erase(servers_.begin()+i);
                buf_.erase(buf_.begin()+i);
                is_reading_.erase(is_reading_.begin()+i);
            }
        }

        // check if all servers are disconnected
        if (sockets_.size() == 0)
        {
            // report for waiting on <RETURN> keystroke
            NoServerReport();
        }
    }

    com_mutex_.unlock();
}

