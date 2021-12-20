#include "echo.h"

#include "Logging.h"
#include "EventLoop.h"

using namespace muduo;
using namespace muduo::net;

EchoServer::EchoServer(EventLoop* loop,
                       const InetAddress& listenAddr,
                       int maxConnections)
  : server_(loop, listenAddr, "EchoServer"),
    numConnected_(0),
    kMaxConnections_(maxConnections)
{
  server_.setConnectionCallback(
      std::bind(&EchoServer::onConnection, this, _1));
  server_.setMessageCallback(
      std::bind(&EchoServer::onMessage, this, _1, _2, _3));
}

void EchoServer::start()
{
  server_.start();
}

void EchoServer::onConnection(const TcpConnectionPtr& conn)
{
  LOG_INFO << "EchoServer - " << conn->peerAddress().toIpPort() << " -> "
           << conn->localAddress().toIpPort() << " is "
           << (conn->connected() ? "UP" : "DOWN");

  if (conn->connected())
  {
    ++numConnected_;
    if (numConnected_ > kMaxConnections_)
    {
      conn->shutdown();
      conn->forceCloseWithDelay(3.0);  // > round trip of the whole Internet.
    }
  }
  else
  {
    --numConnected_;
  }
  LOG_INFO << "numConnected = " << numConnected_;
}

void EchoServer::onMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           Timestamp time)
{
  string msg(buf->retrieveAllAsString());
  LOG_INFO << conn->name() << " echo " << msg.size() << " bytes at " << time.toString();
  conn->send(msg);
}

int main(int argc, char* argv[])
{
  LOG_INFO << "pid = " << getpid();
  EventLoop loop;
  InetAddress listenAddr(2007);
  int maxConnections = 5;
  if (argc > 1)
  {
    maxConnections = atoi(argv[1]);
  }
  LOG_INFO << "maxConnections = " << maxConnections;
  EchoServer server(&loop, listenAddr, maxConnections);
  server.start();
  loop.loop();
}