#ifndef CONSUMER_H
#define CONSUMER_H

/*
 * Class that represents consumers
 */
class Consumer
{
public:
    Consumer();
    void consume(int item);

private:
    int consumed_item;
};

#endif // CONSUMER_H
