/* ------------------------------------------------------------------------------
 * Publishers-Subscribers NETWORK demo
 *   - main function resides here
 *   - launches a single publisher or a single subscriber
 *   - arguements are passed through the corresponding configuration file
 *   - programme runs INFINATE LOOPS (press ENTER in the active terminal to exit)
 *
 * ------------------------------------------------------------------------------
 * USAGE: takes 1 arguement (the configuration file)
 *
 *             e.g.  1st arguement: pub_1.config
 *
 * ------------------------------------------------------------------------------
 * NOTE: in the configuration file, the threads count should be at least two,
 *       as there should always be one acceptor/connector thread
 *
 *             e.g. <local ... communication_threads_count = "2"/>
 *
 * ------------------------------------------------------------------------------
 * Author: Dimitris Saliaris
 * Date:   March 18th, 2013
 */

#include <boost/thread.hpp>
#include <boost/asio.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>
#include <fstream>
#include "publisher.h"
#include "subscriber.h"

// Struct for subscriber connections (used for the xml parser)
struct Connection
{
    std::string pub_id;
    std::string ip;
    std::string port;
};

// Struct for publisher options (used for the xml parser)
struct PubOptions
{
    std::string pub_id;
    int threads;
    std::string port;
    bool rand_intervals;
    int upper_bound_ms;
};

// Struct for subscriber options (used for the xml parser)
struct SubOptions
{
    std::string sub_id;
    int threads;
    int connections_count;
    std::vector<Connection> pubs;
};

void LaunchPub(Publisher& pub, PubOptions opt);  // Launches a publisher (server)
void LaunchSub(Subscriber& sub, SubOptions opt); // Launches a subscriber (client)
bool FileIsValid(std::string file_name);         // Validates config-file arguement
void SetOptions(PubOptions& options, std::string file_name);  // xml parser for pub
void SetOptions(SubOptions& options, std::string file_name);  // xml parser for sub

/*
 * main: @argc = 2, @argv[1] : the config file name
 */
int main(int argc, char *argv[])
{
    std::string file_name = argv[1];

    // accept only one arguement
    if ((argc != 2) || (!FileIsValid(file_name)))
    {
        std::cout << "usage: pub<x>.config" << std::endl
                  << "            OR" << std::endl
                  << "       sub<x>.config \n" << std::endl;
        return -1;
    }

    // launch a pub
    if (file_name.substr(0,3)=="pub")
    {
        // parse xml and set user defined options
        PubOptions opt;
        SetOptions(opt, file_name);

        // create and launch the pub
        Publisher pub;
        pub.set_id(opt.pub_id);
        pub.set_rand_intervals(opt.rand_intervals);
        pub.set_upper_bound_ms(opt.upper_bound_ms);
        LaunchPub(pub, opt);
    }

    // launch a sub
    else if (file_name.substr(0,3)=="sub")
    {
        // parse xml and set user defined options
        SubOptions opt;
        SetOptions(opt, file_name);

        // create and launch the sub
        Subscriber sub;
        sub.set_id(opt.sub_id);
        LaunchSub(sub, opt);
    }

    return 0;
}

/*
 * Launches a publisher (server)
 */
void LaunchPub(Publisher& pub, PubOptions opt)
{
    boost::shared_ptr<boost::asio::io_service> io_service(new boost::asio::io_service);
    boost::shared_ptr<boost::asio::io_service::work> work(new boost::asio::io_service::work(*io_service));

    std::cout << "[" << boost::this_thread::get_id()
              << "] Press [return] to exit." << std::endl;

    // create and launch the communication threads
    boost::thread_group worker_threads;
    for (int i=0; i<opt.threads; i++)
    {
        worker_threads.create_thread(boost::bind(&Publisher::ServerRun, &pub, io_service));
    }

    // setup listening for and accepting subs
    boost::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor(new boost::asio::ip::tcp::acceptor( *io_service ));
    boost::shared_ptr<boost::asio::ip::tcp::socket> sock(new boost::asio::ip::tcp::socket( *io_service ));
    try
    {
        boost::asio::ip::tcp::resolver resolver( *io_service );
        boost::asio::ip::tcp::resolver::query query( "127.0.0.1", opt.port);
        boost::asio::ip::tcp::endpoint endpoint = *resolver.resolve( query );
        acceptor->open( endpoint.protocol() );
        acceptor->set_option( boost::asio::ip::tcp::acceptor::reuse_address( true ) );
        acceptor->bind( endpoint );
        acceptor->listen( boost::asio::socket_base::max_connections );

        std::cout << "Listening on: " << endpoint << std::endl;

        acceptor->async_accept(*sock, boost::bind(&Publisher::AddSubscriber, &pub, boost::asio::placeholders::error(), sock, acceptor ));
    }

    // report any exceptions
    catch (std::exception & ex)
    {
        std::cout << "[" << boost::this_thread::get_id() << "] Exception: " << ex.what() << std::endl;
    }

    // start publish
    io_service->post(boost::bind(&Publisher::PublishData, &pub));

    // listen for exit signal from user
    std::cin.get();

    // prepare for exit
    boost::system::error_code ec;
    acceptor->close(ec);
    for (unsigned i=0; i<pub.sockets().size(); i++)
    {
        boost::system::error_code ec;
        pub.sockets()[i]->shutdown( boost::asio::ip::tcp::socket::shutdown_both, ec );
        pub.sockets()[i]->close( ec );
    }

    io_service->stop();
    worker_threads.join_all();
}

/*
 * Launches a subscriber (client)
 */
void LaunchSub(Subscriber &sub, SubOptions opt)
{
    boost::shared_ptr<boost::asio::io_service> io_service(new boost::asio::io_service);
    boost::shared_ptr<boost::asio::io_service::work> work(new boost::asio::io_service::work( *io_service ));

    std::cout << "[" << boost::this_thread::get_id() << "] Press [return] to exit." << std::endl;

    // create and launch the communication threads
    boost::thread_group worker_threads;
    for (int i=0; i<opt.threads; i++)
    {
        worker_threads.create_thread(boost::bind(&Subscriber::ClientRun, &sub, io_service));
    }

    // connect with the pubs
    for (int i=0; i<opt.connections_count; i++)
    {
        boost::shared_ptr<boost::asio::ip::tcp::socket> sock(new boost::asio::ip::tcp::socket( *io_service ));

        try
        {
            // get the pub from the options
            Connection con = opt.pubs[i];

            // try to connect
            boost::asio::ip::tcp::resolver resolver( *io_service );
            boost::asio::ip::tcp::resolver::query query(con.ip, con.port);
            boost::asio::ip::tcp::resolver::iterator iterator = resolver.resolve( query );
            boost::asio::ip::tcp::endpoint endpoint = *iterator;
            sock->async_connect(endpoint,
                                boost::bind(&Subscriber::AddPublisher,
                                            &sub,
                                            boost::asio::placeholders::error(),
                                            sock,
                                            endpoint,
                                            con.pub_id));

            std::cout << "Connecting to: " << endpoint << std::endl;
        }

        // catch any exception
        catch( std::exception & ex )
        {
            std::cout << "[" << boost::this_thread::get_id() << "] Exception: " << ex.what() << std::endl;
        }
    }

    // start reading from all pubs
    io_service->post(boost::bind(&Subscriber::DisplayMessage, &sub));

    // listen for exit signal from user
    std::cin.get();

    // prepare for exit
    for (unsigned i=0; i<sub.sockets().size(); i++)
    {
        boost::system::error_code ec;
        sub.sockets()[i]->shutdown( boost::asio::ip::tcp::socket::shutdown_both, ec );
        sub.sockets()[i]->close( ec );
    }

    io_service->stop();
    worker_threads.join_all();
}

/*
 * Validates the config-file arguement passed by the user
 */
bool FileIsValid(std::string file_name)
{
    std::string role = file_name.substr(0,3);  // pub OR sub
    std::string id = file_name.substr(3);
    id.erase(id.rfind('.'), std::string::npos);  // one or more digits
    std::string extention = file_name.substr(file_name.rfind('.'),
                                             std::string::npos);  // ".config"

    // check that the id is only digits
    std::locale loc;
    bool valid_id = true;
    for (unsigned i=0; i<id.length(); i++) valid_id &= std::isdigit(id[i], loc);

    // validate
    return (((role == "pub") || (role == "sub"))
            && (valid_id)
            && (extention == ".config"));
}

/*
 * Sets user-defined options for a publisher
 */
void SetOptions(PubOptions& options, std::string file_name)
{
    // read the xml (config) file into a boost property tree
    std::ifstream config(file_name.c_str());
    boost::property_tree::ptree opt;
    read_xml(config, opt);

    // set some pub options
    options.pub_id = opt.get_child("local.<xmlattr>.id").data();
    options.port = opt.get_child("local.<xmlattr>.listening_port").data();
    options.threads = boost::lexical_cast<int>(opt.get_child("local.<xmlattr>.communication_threads_count").data());

    // check if there are at least two threads available
    if (!(options.threads > 1))
    {
        std::cout << "ERROR -> in the configuration file: \n" <<
                     "communication_threads_count should be at least 2 \n\n" <<
                     "      e.g. <local ... communication_threads_count = \"2\"/>\n\n";
        exit(-1);
    }

    // set production options
    std::string temp = opt.get_child("data_production.<xmlattr>.rand_intervals").data();
    options.rand_intervals = (temp == "true" ? true : false);
    options.upper_bound_ms = boost::lexical_cast<int>(opt.get_child("data_production.<xmlattr>.upper_bound_ms").data());
}

/*
 * Sets user-defined options for a subscriber
 */
void SetOptions(SubOptions& options, std::string file_name)
{
    // read the xml (config) file into a boost property tree
    std::ifstream config(file_name.c_str());
    boost::property_tree::ptree opt;
    read_xml(config, opt);

    // set some sub options
    options.sub_id = opt.get_child("local.<xmlattr>.id").data();
    options.threads = boost::lexical_cast<int>(opt.get_child("local.<xmlattr>.communication_threads_count").data());

    // check if there are at least two threads available
    if (!(options.threads > 1))
    {
        std::cout << "ERROR -> in the configuration file: \n" <<
                     "communication_threads_count should be at least 2 \n\n" <<
                     "      e.g. <local ... communication_threads_count = \"2\"/>\n\n";
        exit(-1);
    }

    // set connection options
    options.connections_count = boost::lexical_cast<int>(opt.get_child("remote_connections.<xmlattr>.count").data());
    BOOST_FOREACH(boost::property_tree::ptree::value_type& val, opt.get_child("remote_connections"))
    {
        if (val.first == "publisher")
        {
            Connection con;
            con.pub_id = val.second.get_child("<xmlattr>.id").data();
            con.ip = val.second.get_child("<xmlattr>.ip").data();
            con.port = val.second.get_child("<xmlattr>.port").data();
            options.pubs.push_back(con);
        }
    }
}
