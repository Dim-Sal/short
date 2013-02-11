#ifndef RUN_OPTIONS_H
#define RUN_OPTIONS_H

/*
 * Class that stores and handles run parameters for the market
 *  - the parameters represent market size and rates
 * ---------------------------------------------------------------------------------------------
 * DEFAULT options:
 *                    producer: [  10] threads
 *                    consumer: [  10] threads
 *         producer sleep-time: [ 500] milliseconds
 *         consumer sleep-time: [ 500] milliseconds
 *               buffer-length: [1000] integers
 *
 * ---------------------------------------------------------------------------------------------
 * NOTE: Limitation on max threads used -> it works OK with up to 381 threads
 *        - with 382 threads (191 producers + 191 consumers) it can not create any more threads
 *          it throws:
 *
 *            " boost thread: failed in pthread_create: Resource temporarily unavailable "
 * ---------------------------------------------------------------------------------------------
 *
 */
class Run_Options
{
public:
    // constructor
    Run_Options();

    // Setter functions
    void set_numOfProducers(int nOp),
         set_numOfConsumers(int nOc),
         set_prodDuration(int pDur),
         set_conDuration(int cDur),
         set_marketBuffSize(int mbSize);

    // Getter functions
    int get_numOfProducers(),
        get_numOfConsumers(),
        get_prodDuration(),
        get_conDuration(),
        get_marketBuffSize();

private:
    // Options
    int num_of_producers,
        num_of_consumers,
        production_duration,
        consumption_duration,
        market_buffer_size;
};

#endif // RUN_OPTIONS_H
