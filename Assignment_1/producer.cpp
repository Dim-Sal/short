#include "producer.h"

Producer::Producer(int id)
{
    produced_item = id;
}

/*
 * Representation of some production
 */
void Producer::produce(int duration)
{
    usleep(duration);
}

int Producer::get_item()
{
    return produced_item;
}
