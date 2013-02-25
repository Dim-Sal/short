#include "publisher.h"

/*
 * Connects a subscriber's slot to the publisher's signal
 */
boost::signals2::connection Publisher::AddSubscriber(Subscriber& sub)
{
    return signal_.connect(boost::bind(&Subscriber::set_message, &sub, _1));
}

/*
 * Sends a string message to all connected subscribers
 */
void Publisher::PublishData()
{
    // initialise the sleep duration
    std::string message("");
    int sleep_duration = 0;

    do // infinate loop
    {
        message.append("From Pub ");
        message.append(boost::lexical_cast<std::string>(id_));
        message.append(" [thr_ID: ");
        message.append(boost::lexical_cast<std::string>(boost::this_thread::get_id()));
        message.append("] ");
        signal_(message);

        // clear message
        message.assign("");

        // sleep so as to simulate random message emission (range: from 0.1 to 0.9 seconds)
        srand(time(NULL));  // random seed initialisation
        sleep_duration = (rand() % 9 + 1) * 100000;
        usleep(sleep_duration);
    }
    while (true);
}
