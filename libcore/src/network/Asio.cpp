#include "util/Standard.hh"
#include "Asio.hpp"
#include "IOService.hpp"

namespace Sirikata {
namespace Network {

InternalIOWork::InternalIOWork(IOService& serv, const String& name)
 : InternalIOService::work(serv.asioService()),
   mName(name)
{
    logEvent("created");
}

InternalIOWork::InternalIOWork(IOService* serv, const String& name)
 : InternalIOService::work(serv->asioService()),
   mName(name)
{
    logEvent("created");
}

InternalIOWork::~InternalIOWork() {
    logEvent("destroyed");
}

void InternalIOWork::logEvent(const String& evt) {
    if (mName != "")
        SILOG(io,insane,"IOWork event: " << mName << " -> " << evt);
}



InternalIOStrand::InternalIOStrand(IOService &io)
 : boost::asio::io_service::strand(io.asioService())
{
}


TCPSocket::TCPSocket(IOService&io):
    boost::asio::ip::tcp::socket(io.asioService())
{
}

TCPSocket::~TCPSocket()
{
}


TCPListener::TCPListener(IOService&io, const boost::asio::ip::tcp::endpoint&ep):
    boost::asio::ip::tcp::acceptor(io.asioService(),ep)
{
}

TCPListener::~TCPListener()
{
}

void TCPListener::async_accept(TCPSocket&socket,
                               const std::tr1::function<void(const boost::system::error_code&)>&cb) {
    this->InternalTCPAcceptor::async_accept(socket,cb);
}


TCPResolver::TCPResolver(IOService&io)
    : boost::asio::ip::tcp::resolver(io.asioService())
{
}

TCPResolver::~TCPResolver()
{
}


UDPSocket::UDPSocket(IOService&io):
    boost::asio::ip::udp::socket(io.asioService())
{
}

UDPSocket::~UDPSocket()
{
}

UDPResolver::UDPResolver(IOService&io)
    : boost::asio::ip::udp::resolver(io.asioService())
{
}

UDPResolver::~UDPResolver()
{
}

DeadlineTimer::DeadlineTimer(IOService& io)
    : boost::asio::deadline_timer(io.asioService()) {
}


} // namespace Network
} // namespace Sirikata
