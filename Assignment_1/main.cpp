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
 * NOTE: Limitation on max threads used -> it works OK with up to 381 threads
 *        - with 382 threads (191 producers + 191 consumers) it can not create any more threads
 *          it throws:
 *
 *            " boost thread: failed in pthread_create: Resource temporarily unavailable "
 *
 * ---------------------------------------------------------------------------------------------
 * Author: Dimitris Saliaris
 * Date:   Feb 6th, 2013
 */

void setOptions(int num_of_args, char* arg_vector[], Run_Options* opt);
void showOptions(Run_Options* opt);

/*
 * Main function
 */
int main(int argc, char* argv[])
{
    cout << "          Producers-Consumers demo running" << endl
         << "          --------------------------------" << endl << endl;

    // create an object to store and handle options
    Run_Options* opt = new Run_Options();

    // parse and assign options
    setOptions(argc, argv, opt);

    // display options
    showOptions(opt);

    // create an object to simulate the producers-consumers problem
    Market* MyMarket = new Market(opt);

    // trigger the simulation
    MyMarket->run();

    return 0;
}

/*
 * Parses arguments into Run_Options' members
 */
void setOptions(int num_of_args, char* arg_vector[], Run_Options* opt)
{
    switch (num_of_args)
    {
    case 1:
        // default parameters are already set through the Run_Options constructor
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
            // default parameters are already set through the Run_Options constructor
            cout << endl << endl;
            break;
        }
        else cout << "\n *** Programme is being terminated... *** \n" << endl;
        sleep(3);
        exit(-1);
        }
    case 6:
    {
        opt->set_numOfProducers((int) strtod(arg_vector[1], NULL));
        opt->set_numOfConsumers((int) strtod(arg_vector[2], NULL));
        opt->set_prodDuration((int) strtod(arg_vector[3], NULL));
        opt->set_conDuration((int) strtod(arg_vector[4], NULL));
        opt->set_marketBuffSize((int) strtod(arg_vector[5], NULL));
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
void showOptions(Run_Options* opt)
{
    string dummy_input = "";

    cout << "\t  Number of producers: " << opt->get_numOfProducers() << " threads" << endl
         << "\t  Number of consumers: " << opt->get_numOfConsumers() << " threads" << endl
         << "\t  Production duration: " << opt->get_prodDuration() << " milliseconds" << endl
         << "\t Consumption duration: " << opt->get_conDuration() << " milliseconds" << endl
         << "\t Market-buffer length: " << opt->get_marketBuffSize() << " integers" << endl << endl
         << "Press <RETURN> key to continue..." << endl;
    getline(cin, dummy_input);
}
