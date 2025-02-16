#include <mirror/sync_scheduler/ManualSyncListener.hpp>

#include <iostream>
#include <string>

#include <spdlog/spdlog.h>
#include <zmq.hpp>

namespace mirror::sync_scheduler
{
zmq::context_t context{};
zmq::socket_t socket{context, zmq::socket_type::rep};
constexpr std::string MANUAL_SYNC_PORT{"9281"};

void manual_sync_init()
{
    socket.bind("tcp://*:" + MANUAL_SYNC_PORT);
}

void manual_sync_listen()
{
    while(true) {
        zmq::message_t request;
        socket.recv(request, zmq::recv_flags::none);
        spdlog::info("Manual sync requsted for {}. Unimplemented :(", request.str());
        std::cout << "Syncing " << request.str() << std::endl;
        socket.send(zmq::message_t{"unimplemented"});
    }
}
}