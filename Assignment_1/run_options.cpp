#include "run_options.h"

/*
 * Constructor
 *  - Sets default values for the market
 */
Run_Options::Run_Options()
{
    num_of_producers = 10;      // threads
    num_of_consumers = 10;      // threads
    production_duration = 500;  // milliseconds
    consumption_duration = 500; // milliseconds
    market_buffer_size = 1000;  // integers
}

// Setter functions:

void Run_Options::set_numOfProducers(int nOp)
{
    num_of_producers = nOp;
}

void Run_Options::set_numOfConsumers(int nOc)
{
    num_of_consumers = nOc;
}

void Run_Options::set_prodDuration(int pDur)
{
    production_duration = pDur;
}

void Run_Options::set_conDuration(int cDur)
{
    consumption_duration = cDur;
}

void Run_Options::set_marketBuffSize(int mbSize)
{
    market_buffer_size = mbSize;
}


// Getter functions:

int Run_Options::get_numOfProducers()
{
    return num_of_producers;
}

int Run_Options::get_numOfConsumers()
{
    return num_of_consumers;
}

int Run_Options::get_prodDuration()
{
    return production_duration;
}

int Run_Options::get_conDuration()
{
    return consumption_duration;
}

int Run_Options::get_marketBuffSize()
{
    return market_buffer_size;
}

