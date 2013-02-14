#include "market.h"

/*
 * Default constructor
 *  - sets all (user-defined/default) market parameters
 *  - initialises resources and counters
 */
Market::Market(run_options* p_opt)
{
    // set all parameters
    num_of_producers = p_opt->num_of_producers;
    num_of_consumers = p_opt->num_of_consumers;
    production_duration = p_opt->production_duration;
    consumption_duration = p_opt->consumption_duration;
    market_buffer_size = p_opt->market_buffer_size;

    // initialise resources and counters
    market_buffer = new int[market_buffer_size];
    for (int i = 0; i < market_buffer_size; i++)
    {
        market_buffer[i] = 0;
    }

    item_counter = 0;
    prod_counter = 0;
    cons_counter = 0;
}

/*
 * Writes a datum (here: an int) on the shared buffer
 *  - loops forever
 *  - if the buffer is being used, all other threads (both producers and consumers) wait on the mutex
 *  - if buffer is full, producer-threads wait on the condition_variable_any object
 */
void Market::buffer_write(Producer current_producer)
{
    while (true) // infinate loop
    {
        // produce (sleep) BEFORE entering the critical section
        current_producer.produce(production_duration*1000);

        // if mutex is unlocked, the producer-thread locks it and accesses the code, else it waits for its turn
        boost::mutex::scoped_lock write_lock(buffer_mutex);

        // while buffer is full, producer-thread waits here for a notification from a consumer-thread
        while (item_counter == market_buffer_size)
        {
            buff_FULL.wait(write_lock);
        }

        // write in the next free buffer slot
        market_buffer[item_counter] = current_producer.get_item();

        // show: buffer-index -- number of units produced so far -- product (actually the thread number)
        cout << "   [" << item_counter << "]   PRODUCTION: " << ++prod_counter << " --> produced: " << market_buffer[item_counter] << endl;

        // increase item counter (also represents the buffer INDEX for the next available slot for writting)
        item_counter++;

        // if this is the first production after an empty buffer, notify a consumer (if any is waiting)
        if (item_counter == 1)
        {
            buff_EMPTY.notify_one();
        }

    } // end(while)
}

/*
 * Reads a datum (here: an int) from the shared buffer
 *  - loops forever
 *  - if the buffer is being used, all other threads (both producers and consumers) wait on the mutex
 *  - if buffer is empty, consumer-threads wait on the condition_variable_any object
 */
void Market::buffer_read(Consumer current_consumer)
{
    // consume (sleep) BEFORE entering the critical section
    current_consumer.consume(consumption_duration*1000);

    while (true) // infinate loop
    {
        // if mutex is unlocked, the consumer-thread locks it and accesses the code, else it waits for its turn
        boost::mutex::scoped_lock read_lock(buffer_mutex);

        // while buffer is empty, consumer-thread waits here for a notification from a producer-thread
        while (item_counter == 0)
        {
            buff_EMPTY.wait(read_lock);
        }

        // read from the last occupied buffer slot
        current_consumer.consume(market_buffer[item_counter-1]);

        // show: buffer-index -- number of units consumed so far -- product (actually the producer-thread number)
        cout << "   [" << item_counter-1 << "]   CONSUMPTION: " << ++cons_counter << " <-- consumed: " << market_buffer[item_counter-1] << endl;

        // decrease item counter
        item_counter--;

        // if this is the first consumption after an full buffer, notify a producer (if any is waiting)
        if (item_counter == (market_buffer_size-1))
        {
            buff_FULL.notify_one();
        }
    }
}

/*
 * Runs a multi-threaded implementation of the Producers-Consumers problem
 */
void Market::run()
{
    // Print Headers
    cout << "  index    action  counter       value " << endl
         << "  -----    ------  -------       ----- " << endl << endl;

    // create and launch all producer threads
    for (int i=0; i<num_of_producers; i++)
    {
        Producer producer(i+1);
        threads.create_thread(boost::bind(&Market::buffer_write, this, producer));
    }

    // create and launch all consumer threads
    for (int i=0; i<num_of_consumers; i++)
    {
        Consumer consumer;
        threads.create_thread(boost::bind(&Market::buffer_read, this, consumer));
    }

    threads.join_all();
}

/*
 * Destructor
 *  - Explicitly frees all object memory
 */
Market::~Market()
{
    // Free memory allocated for the market buffer
    delete[] this->market_buffer;
}
