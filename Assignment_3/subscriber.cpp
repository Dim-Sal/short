#include "subscriber.h"

void Subscriber::Launch(SubOptions opt)
{
    com_->InitIoService();
    com_->LaunchThreads(opt.threads);
    com_->Connect(opt.connections_count, opt.pubs);
}

/*
 * Gets message(s) from publisher(s)
 *  - loops till all pubs are disconnected or sub exits (manually)
 *  - thread-safe
 */
void Subscriber::GetMessages()
{
    // wait for the first remote host to connect
    boost::mutex::scoped_lock connection_lock(connection_mutex_);
    while(com_->NoConnections() && com_->Running())
    {
        com_->pending_first_condition()->wait(connection_lock);
    }

    // read from publishers
    if (com_->Running())
    {
        com_->Read();
    }
}

/*
 * Displays message(s) form publisher(s)
 */
void Subscriber::DisplayMessages()
{
    while(com_->Running())
    {
        boost::mutex::scoped_lock display_lock(message_mutex_);

        // wait until a message is read
        while (!com_->is_set_message())
        {
            com_->setting_message_condition()->wait(display_lock);
        }

        if (com_->Running())
        {
            // get message from com
            message_ = com_->message();

            // append this sub's id and this thread's id
            message_.append("to ");
            message_.append(id_);
            message_.append(" [thr_ID: ");
            message_.append(boost::lexical_cast<std::string>(boost::this_thread::get_id()));
            message_.append("]");

            // display message
            std::cout << message_ << std::endl;

            com_->set_is_picked_message(true);
            com_->set_is_set_message(false);
            com_->getting_message_condition()->notify_one();

            // re-initialise message
            message_ = "";
        }
    }
}
