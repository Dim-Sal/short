#ifndef CONSUMER_H
#define CONSUMER_H
#include <iostream>

using namespace std;

/*
 * Class that represents consumers
 */
class Consumer
{
public:
    Consumer();
    void consume(int duration);
    void set_item(int item);

private:
    int consumed_item;
};

#endif // CONSUMER_H
