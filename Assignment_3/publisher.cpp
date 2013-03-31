#include "publisher.h"

/*
 * Launches the pub
 *  - @opt: the options parsed from the config file
 */
void Publisher::Launch(PubOptions opt)
{
    com_->InitIoService();
    com_->LaunchThreads(opt.threads);
    com_->Accept(opt.port);
}

/*
 * Sends a string message to all connected subscribers
 *  - loops infinately
 *  - exits on <RETURN> keystroke
 *  - thread safe
 */
void Publisher::PublishData()
{
    // initialise the sleep duration
    int sleep_duration = 0;

    // publish even if all subscribers are disconnected (they can re-connect)
    while (com_->Running())
    {
        // prepare the message in the form: "From pub_x [thr_ID: xxxxxxxx] "
        std::string message("");
        message.append("From ");
        message.append(id_);
        message.append(" [thr_ID: ");
        message.append(boost::lexical_cast<std::string>(boost::this_thread::get_id()));
        message.append("] ");

        com_->Write(message, 30);

        // sleep so as to simulate random message emission
        if (rand_intervals_)
        {
            sleep_duration = (rand() % upper_bound_ms_ + 1) * 1000;
            usleep(sleep_duration);
        }

        // sleep for a while (fixed intervals)
        else
        {
            usleep(upper_bound_ms_ * 1000);
        }
    }
}
