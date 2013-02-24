#ifndef PUBLISHER_H
#define PUBLISHER_H
#include <boost/signals2.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include "subscriber.h"

/*
 * Class representation for publishers
 */
class Publisher
{
    public:
        Publisher(){}
        void set_id(int id) {id_ = id;}
        boost::signals2::connection AddSubscriber(Subscriber& sub);  // connects subscriber slot to signal_
        void PublishData();                                          // sends a string to subscribers

    private:
        boost::signals2::signal<void (std::string)> signal_;  // signal used for publishing to subscribers
        int id_;                                              // publisher's identifier
};

#endif // PUBLISHER_H
