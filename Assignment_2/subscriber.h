#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H
#include <boost/signals2.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>

/*
 * Class representation for subscribers
 */
class Subscriber
{
    public:
        // Constructor: initialises condition variables
        Subscriber(){wait_for_signal_ = true; pending_display_ = false;}
        void set_id(int id) {id_ = id;}

        void set_message(std::string str);                     // message mutator
        void DisplayMessage();                                 // displays message received from publisher
        void AddConnection(boost::signals2::connection con){
                                connections_.push_back(con);}  // keeps record of subscriptions

    private:
        std::vector<boost::signals2::connection> connections_;  // connections to publishers (unused for now)
        boost::mutex message_mutex_;                            // resource mutex
        boost::condition_variable_any message_set_,             // condition variable for subscribers
                                      display_set_;             // condition variable for publishers
        bool wait_for_signal_,  // flag for subscriber to wait
             pending_display_;  // flag for publisher to wait
        std::string message_;   // shared resource
        int id_;
};

#endif // SUBSCRIBER_H
