#ifndef OPTIONS_H
#define OPTIONS_H

// Struct for subscriber connections (used for the xml parser)
struct Connection
{
    std::string pub_id;
    std::string ip;
    std::string port;
};

// Struct for publisher options (used for the xml parser)
struct PubOptions
{
    std::string pub_id;
    int threads;
    std::string port;
    bool rand_intervals;
    int upper_bound_ms;
};

// Struct for subscriber options (used for the xml parser)
struct SubOptions
{
    std::string sub_id;
    int threads;
    int connections_count;
    std::vector<Connection> pubs;
};

#endif // OPTIONS_H
