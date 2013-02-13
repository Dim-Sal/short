#include <iostream>
#include <string>
#include <boost/chrono.hpp>
#include "market.h"
#include "run_options.h"

/* ---------------------------------------------------------------------------------------------
 * Producers-Consumers demo
 *   - main function resides here
 *   - arguements are passed through the terminal
 *   - programme runs an INFINATE LOOP (manually kill to exit)
 *
 * ---------------------------------------------------------------------------------------------
 * USAGE: takes 5 arguements (or none -> default) as follows
 *
 *               1st arguement: number of producer threads
 *               2nd arguement: number of consumer threads
 *               3rd arguement: sleep-duration for producer threads (in milliseconds)
 *               4th arguement: sleep-duration for consumer threads (in milliseconds)
 *               5th arguement: buffer-length (integers)
 *
 * ---------------------------------------------------------------------------------------------
 * DEFAULT options:
 *                    producer: [  10] threads
 *                    consumer: [  10] threads
 *     producer sleep-duration: [ 500] milliseconds
 *     consumer sleep-duration: [ 500] milliseconds
 *               buffer-length: [1000] integers
 *
 * ---------------------------------------------------------------------------------------------
 * Author: Dimitris Saliaris
 * Date:   Feb 6th, 2013
 */

void setOptions(int num_of_args, char* arg_vector[], run_options* p_opt);
void showOptions(run_options* p_opt);

/*
 * Main function
 */
int main(int argc, char* argv[])
{
    cout << "          Producers-Consumers demo running" << endl
         << "          --------------------------------" << endl << endl;

    // create an object to store and handle options
    run_options opt;

    // default parameters are set here
    opt.num_of_producers = 10;      // threads
    opt.num_of_consumers = 10;      // threads
    opt.production_duration = 500;  // milliseconds
    opt.consumption_duration = 500; // milliseconds
    opt.market_buffer_size = 1000;  // integers

    // parse and assign user-defined options (if any)
    setOptions(argc, argv, &opt);

    // display options
    showOptions(&opt);

    // create an object to simulate the producers-consumers problem
    Market MyMarket(&opt);

    // trigger the simulation
    MyMarket.run();

    return 0;
}

/*
 * Parses arguments into Run_Options' members
 */
void setOptions(int num_of_args, char* arg_vector[], run_options* p_opt)
{
    switch (num_of_args)
    {
    case 1:
        // using default parameters
        cout << "\t  DEFAULT parameters are being used: " << endl << endl;
        break;

    case 2: // *** TODO: check and validate...
    case 3: // *** TODO: check and validate...
    case 4: // *** TODO: check and validate...
    case 5: // *** TODO: check and validate...
        {
        string use_default = "N";
        cout << "      *** Warning: " << endl
             << "          Some parameters are missing here... " << endl
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
    case 6:
    {
        // *** TODO: check and validate...
        p_opt->num_of_producers = (int) strtod(arg_vector[1], NULL);
        p_opt->num_of_consumers = (int) strtod(arg_vector[2], NULL);
        p_opt->production_duration = (int) strtod(arg_vector[3], NULL);
        p_opt->consumption_duration = (int) strtod(arg_vector[4], NULL);
        p_opt->market_buffer_size = (int) strtod(arg_vector[5], NULL);
        break;
    }
    default:
        // *** TODO: check and validate...
        break;
    }
}

/*
 * Displays options to be used
 */
void showOptions(run_options* p_opt)
{
    cout << "\t  Number of producers: " << p_opt->num_of_producers << " threads" << endl
         << "\t  Number of consumers: " << p_opt->num_of_consumers << " threads" << endl
         << "\t  Production duration: " << p_opt->production_duration << " milliseconds" << endl
         << "\t Consumption duration: " << p_opt->consumption_duration << " milliseconds" << endl
         << "\t Market-buffer length: " << p_opt->market_buffer_size << " integers" << endl << endl;

    // wait 2 seconds for the display to be read
    sleep(3);
}
