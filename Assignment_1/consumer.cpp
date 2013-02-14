#include "consumer.h"

Consumer::Consumer(){}

/*
 * Representation of some consumption
 */
void Consumer::consume(int duration)
{
    usleep(duration);
}

void Consumer::set_item(int item)
{
    consumed_item = item;
}
