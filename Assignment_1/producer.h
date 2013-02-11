#ifndef PRODUCER_H
#define PRODUCER_H

/*
 * Class that represents producers
 */
class Producer
{
public:
    Producer(int id);
    int produce();

private:
    int produced_item;

};

#endif // PRODUCER_H
