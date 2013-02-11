#include "producer.h"

Producer::Producer(int id)
{
    produced_item = id;
}

int Producer::produce()
{
    return produced_item;
}
