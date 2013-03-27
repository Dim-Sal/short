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
        Subscriber() : com_(new Communication) {}    // initialises com
        void Launch(SubOptions opt);                 // launches the sub
        void DisplayMessage();                       // displays message(s) from publisher(s)
        void Exit(){com_->PrepareForExit("client");} // provides a clean exit

        // com accessor
        boost::shared_ptr<Communication> com(){return com_;}

        // id mutator
        void set_id(std::string id) {id_ = id;}

    private:
        std::string id_;
        boost::mutex m_;
        boost::shared_ptr<Communication> com_;  // network wrapper
};

#endif // SUBSCRIBER_H
