#include "subscriber.h"

/*
 * Message mutator
 *  - is called by a connected publisher when there is a new message
 *  - thread-safe
 */
void Subscriber::set_message(std::string str)
{
    // assure only one thread (a publisher) can access this code at a time
    boost::mutex::scoped_lock mutator_lock(message_mutex_);

    // let this thread (a publisher) wait while another (a subscriber) is using the message for display
    while(pending_display_)
    {
        display_set_.wait(mutator_lock);
    }

    message_ = str;             // set the publisher message
    pending_display_ = true;    // set flag to let publisher threads wait
    wait_for_signal_ = false;   // set flag to let a susbscriber thread display
    message_set_.notify_one();  // notify a subscriber thread that it's now ready to display
}

/*
 * Displays a new message received from a publisher
 *  - runs an infinate loop
 *  - thread-safe
 */
void Subscriber::DisplayMessage()
{
    while(true) // infinate loop
    {
        // assure only one thread (a subscriber) can access this code at a time
        boost::mutex::scoped_lock display_lock(message_mutex_);

        // let this thread (a subscriber) wait while another (a publisher) is setting the message
        while(wait_for_signal_)
        {
            message_set_.wait(display_lock);
        }

        // append this object's id and this thread's id to the message
        message_.append(" to Sub ");
        message_.append(boost::lexical_cast<std::string>(id_));
        message_.append(" [thr_ID: ");
        message_.append(boost::lexical_cast<std::string>(boost::this_thread::get_id()));
        message_.append("]");

        std::cout << message_ << std::endl;  // display the message

        message_.assign("");        // clear the message
        pending_display_ = false;   // set flag to let a publisher thread set the next message
        wait_for_signal_ = true;    // set flag to let this subscriber thread wait for the next message
        display_set_.notify_one();  // notify a publisher thread that it's now ready to set the next message
    }
}
