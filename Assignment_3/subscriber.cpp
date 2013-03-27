#include "subscriber.h"

void Subscriber::Launch(SubOptions opt)
{
    com_->InitIoService();
    com_->LaunchThreads(opt.threads);
    com_->Connect(opt.connections_count, opt.pubs);
}

/*
 * Displays message(s) from publisher(s)
 *  - loops till all pubs are disconnected or sub exits (manually)
 *  - thread-safe
 */
void Subscriber::DisplayMessage()
{
    // wait for the first remote host to connect
    boost::mutex::scoped_lock first_conn(m_);
    while(com_->NoConnections() && com_->Running())
    {
        com_->pending_first()->wait(first_conn);
    }

    // read asyncronously from all pubs
    while (!com_->NoConnections() && com_->Running())
    {
        com_->Read(id_);
    }

    // all pubs are disconnected: wait for <RETURN> keystroke
    com_->NoServerReport();
}
