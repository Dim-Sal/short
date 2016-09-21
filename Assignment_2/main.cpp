#include <boost/thread.hpp>
#include <boost/signals2.hpp>
#include "publisher.h"
#include "subscriber.h"

/* ---------------------------------------------------------------------------------------------
 * Publishers-Subscribers demo
 * ---------------------------------------------------------------------------------------------
 * Usage: takes 2 arguements (or none -> default) as follows
 *
 *               1st arguement: number of publisher threads
 *               2nd arguement: number of subscriber threads
 * ---------------------------------------------------------------------------------------------
 * Default options:
 *                    publishers: [ 5] threads
 *                   subscribers: [15] threads
 * ---------------------------------------------------------------------------------------------
 * NOTE : programme runs INFINATE LOOPS, manually kill to exit)
 * ---------------------------------------------------------------------------------------------
 * Author: Dimitris Saliaris
 * Date:   Feb 24th, 2013
 */

/*
 * Structure that holds user-defined options
 */
struct Options { int num_of_pubs, num_of_subs;};

void SetOptions(int num_of_args, char* arg_vector[], Options& opt);
void ShowOptions(Options* p_opt);

/*
 * Main function
 */
int main(int argc, char* argv[])
{
    std::cout << "          Publishers-Subscribers demo running" << std::endl
              << "          -----------------------------------" << std::endl << std::endl;

    // parse, assign, and show user-defined options (or default)
    Options options;
    SetOptions(argc, argv, options);
    int num_of_pub = options.num_of_pubs;
    int num_of_sub = options.num_of_subs;
    ShowOptions(&options);

    // create publishers and set their id
    Publisher pubs[num_of_pub];
    for (int i=0; i<num_of_pub; i++)
    {
        pubs[i].set_id(i+1);
    }

    // create subscribers and set their id
    Subscriber subs[num_of_sub];
    for (int i=0; i<num_of_sub; i++)
    {
        subs[i].set_id(i+1);
    }

    // initialise some utility variables for random shuffling and selection
    srand(time(NULL));  // random seed initialisation
    int num_of_con = 0;
    int indices[num_of_pub];
    for(int i=0; i<num_of_pub; i++)
    {
        indices[i] = i;
    }

    // register connections (for EACH subscriber)
    for (int i=0; i<num_of_sub; i++)
    {
        // randomly determine number of connections to publishers for the (i-th) subscriber
        num_of_con = rand() % num_of_pub + 1;

        // randomly select publishers for the (i-th) subscriber
        std::random_shuffle(indices, indices+num_of_pub);
        for (int j=0; j<num_of_con; j++)
        {
            // connect the (i-th) subscriber with the (j-th) publisher and register the connection
            subs[i].AddConnection(pubs[indices[j]].AddSubscriber(subs[i]));
        }
    }

    // create a thread group to hold all pub-sub threads
    boost::thread_group threads;

    // create and launch pub threads
    for (int i=0; i<num_of_pub; i++)
    {
        threads.create_thread(boost::bind(&Publisher::PublishData, &pubs[i]));
    }

    // create and launch sub threads
    for (int i=0; i<num_of_sub; i++)
    {
        threads.create_thread(boost::bind(&Subscriber::DisplayMessage, &subs[i]));
    }

    threads.join_all();
    return 0;
}

/*
 * Parses user-defined arguments into an Options struct
 */
void SetOptions(int num_of_args, char* arg_vector[], Options& opt)
{
    using namespace std;

    // set default parameters
    opt.num_of_pubs = 5;
    opt.num_of_subs = 15;

    switch (num_of_args)
    {
    // no arguements passed
    case 1:
        cout << "\t  DEFAULT parameters are being used: " << endl << endl;
        break;

    // 1 arguement passed (one missing)
    case 2:  // TODO: check and validate...
        {
        string use_default = "N";
        cout << "      *** Warning: " << endl
             << "          One parameter is missing here... " << endl
             << "          Do you wish to use default parameters? " << endl
             << "                       [Y/N]" << endl;
        getline(cin, use_default);

        if ((use_default.substr(0,1) == "Y") || (use_default == "y"))
        {
            // using default parameters
            cout << endl << endl;
            break;
        }
        else cout << "\n *** Programme is being terminated... *** \n" << endl;

        // wait 2 seconds for the exit message to be read
        sleep(2);
        exit(-1);
        }

    // 2 arguements passed (proper usage)
    case 3:  // TODO: check and validate...
    {
        opt.num_of_pubs = (int) strtod(arg_vector[1], NULL);
        opt.num_of_subs = (int) strtod(arg_vector[2], NULL);
        break;
    }

    // more arguements passed ()
    default:  // TODO: check and validate...

        cout << "\n *** Too many agruements: Programme is being terminated... *** \n" << endl;

        // wait 2 seconds for the exit message to be read
        sleep(2);
        exit(-1);
        break;
    }
}

/*
 * Displays options to be used
 */
void ShowOptions(Options* p_opt)
{
    using namespace std;

    cout << "\t   Number of publishers: " << p_opt->num_of_pubs << " threads" << endl
         << "\t  Number of subscribers: " << p_opt->num_of_subs << " threads \n" << endl;

    // wait 2 seconds for the display to be read
    sleep(2);
}
