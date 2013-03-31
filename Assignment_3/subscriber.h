#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include "communication.h"
#include "options.h"

/*
 * Class representation for subscribers
 */
class Subscriber
{
    public:
        Subscriber() : com_(new Communication),
                       message_("") {}

        void Launch(SubOptions opt);                 // launches the sub
        void GetMessages();                          // gets message(s) from pub(s)
        void DisplayMessages();                      // displays message(s)
        void Exit(){com_->PrepareForExit("client");} // provides a clean exit

        // com accessor
        boost::shared_ptr<Communication> com(){return com_;}

        // id mutator
        void set_id(std::string id) {id_ = id;}


    private:
        boost::shared_ptr<Communication> com_;  // network wrapper
        std::string id_;

        boost::mutex connection_mutex_,
                     message_mutex_;
        std::string message_;
        bool is_set_message_;
};

#endif // SUBSCRIBER_H
