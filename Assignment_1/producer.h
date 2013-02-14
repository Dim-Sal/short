#ifndef PRODUCER_H
#define PRODUCER_H
#include <iostream>

using namespace std;

/*
 * Class that represents producers
 */
class Producer
{
public:
    Producer(int id);
    void produce(int duration);
    int get_item();

private:
    int produced_item;

};

#endif // PRODUCER_H
