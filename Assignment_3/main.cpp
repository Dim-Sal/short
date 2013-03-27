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
 *             e.g.  1st arguement: file_name.config
 *
 * ------------------------------------------------------------------------------
 * NOTES: (1) in the configuration file, the threads count should be at least two,
 *            as there should always be one acceptor/connector thread
 *
 *                  e.g. <local ... communication_threads_count = "2"/>
 *
 *        (2) all sub's pubs (servers) should be launched BEFORE the sub
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
#include "options.h"

bool FileIsValid(std::string file_name);  // Validates config-file arguement
void SetOptions(PubOptions& options, boost::property_tree::ptree opt);  // xml parser for pub
void SetOptions(SubOptions& options, boost::property_tree::ptree opt);  // xml parser for sub

/*
 * main:
 *  - argc = 2,
 *  - argv[1] : the config file name
 */
int main(int argc, char *argv[])
{
    std::string file_name = argv[1];

    // accept only one arguement
    if ((argc != 2) || (!FileIsValid(file_name)))
    {
        std::cout << "usage: <file_name>.config \n" << std::endl;
        return -1;
    }

    // read the xml (config) file into a boost property tree
    std::ifstream config(file_name.c_str());
    boost::property_tree::ptree opt;
    read_xml(config, opt);

    // launch a pub
    if (opt.get_child("local.<xmlattr>.role").data() == "pub")
    {
        // parse xml and set user defined options
        PubOptions options;
        SetOptions(options, opt);

        // create the pub
        Publisher pub;
        pub.set_id(options.pub_id);
        pub.set_rand_intervals(options.rand_intervals);
        pub.set_upper_bound_ms(options.upper_bound_ms);

        // setup srand for random production time simulation
        srand(time(NULL));

        // launch the pub and start publishing
        pub.Launch(options);
        pub.com()->io_service()->post(boost::bind(&Publisher::PublishData,
                                                  &pub));

        // listen for exit signal from user
        std::cin.get();

        // clean exit
        pub.Exit();
    }

    // launch a sub
    else if (opt.get_child("local.<xmlattr>.role").data() == "sub")
    {
        // parse xml and set user defined options
        SubOptions options;
        SetOptions(options, opt);

        // create the sub
        Subscriber sub;
        sub.set_id(options.sub_id);

        // launch the sub and start the message display
        sub.Launch(options);
        sub.com()->io_service()->post(boost::bind(&Subscriber::DisplayMessage,
                                                  &sub));

        // listen for exit signal from user
        std::cin.get();

        // clean exit
        sub.Exit();
    }

    return 0;
}

/*
 * Validates the config-file arguement passed by the user
 */
bool FileIsValid(std::string file_name)
{
    std::string extention = file_name.substr(file_name.rfind('.'),
                                             std::string::npos);
    return (extention == ".config");
}

/*
 * Sets user-defined options for a publisher
 */
void SetOptions(PubOptions& options, boost::property_tree::ptree opt)
{
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
void SetOptions(SubOptions& options, boost::property_tree::ptree opt)
{
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
