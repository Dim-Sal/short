#ifndef RUN_OPTIONS_H
#define RUN_OPTIONS_H

/*
 * Stuct that stores run parameters for the market
 *  - the parameters represent market size and rates
 */
struct run_options
{
    int num_of_producers,
        num_of_consumers,
        production_duration,
        consumption_duration,
        market_buffer_size;
};

#endif // RUN_OPTIONS_H
