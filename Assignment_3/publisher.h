#ifndef PUBLISHER_H
#define PUBLISHER_H
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/thread.hpp>
#include "communication.h"
#include "options.h"

/*
 * Class representation for publishers
 */
class Publisher
{
    public:
        Publisher() : com_(new Communication) {}     // initialises com
        void Launch(PubOptions opt);                 // launches the pub
        void PublishData();                          // sends a string to subscribers
        void Exit(){com_->PrepareForExit("server");} // provides a clean exit

        // com accessor
        boost::shared_ptr<Communication> com(){return com_;}

        // mutators
        void set_id(std::string id) {id_ = id;}
        void set_rand_intervals(bool is_random){rand_intervals_ = is_random;}
        void set_upper_bound_ms(int up_b){upper_bound_ms_ = up_b;}

    private:
        std::string id_;
        int upper_bound_ms_;
        bool rand_intervals_;
        boost::mutex m_;
        boost::shared_ptr<Communication> com_; // network wrapper

};

#endif // PUBLISHER_H
