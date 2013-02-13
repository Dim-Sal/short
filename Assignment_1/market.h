#ifndef MARKET_H
#define MARKET_H
#include <iostream>
#include <boost/thread.hpp>
#include "producer.h"
#include "consumer.h"
#include "run_options.h"

using namespace std;

class Market
{
public:
    Market(run_options* p_opt);
    void run();

private:
    // parameters
    int num_of_producers,
    num_of_consumers,
    production_duration,
    consumption_duration,
    market_buffer_size,

    // counters
    item_counter, // number of items currently present in the market_buffer
    prod_counter, // counts all producers presented in the market so far
    cons_counter; // counts all consumers presented in the market so far

    int* market_buffer;                       // shared resource
    boost::thread_group threads;              // structure for handling grouped threads
    boost::mutex buffer_mutex;                // resource mutex
    boost::condition_variable_any buff_FULL,  // condition var for producers
                                  buff_EMPTY; // condition variable for consumers
    // threaded functions
    void buffer_write(Producer current_producer); // writes in the shared buffer
    void buffer_read(Consumer current_consumer);  // reads from the shared buffer

};

#endif // MARKET_H
