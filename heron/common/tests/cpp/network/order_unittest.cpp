#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include "gtest/gtest.h"

#include "basics/basics.h"
#include "errors/errors.h"
#include "threads/threads.h"
#include "network/network.h"

#include "basics/modinit.h"
#include "errors/modinit.h"
#include "threads/modinit.h"
#include "network/modinit.h"

#include "network/unittests.pb.h"
#include "network/host_unittest.h"
#include "network/oclient_unittest.h"
#include "network/oserver_unittest.h"

static const sp_uint32 SERVER_PORT = 59000;

class Terminate : public Client
{
 public:
  Terminate
  (
    EventLoopImpl*         eventLoop,
    const NetworkOptions& _options
  ) : Client(eventLoop, _options)
  {
    // Setup the call back function to be invoked when retrying
    retry_cb_ = [this] () { this->Retry(); };
  }

  ~Terminate()
  { }

 protected:

  void Retry() { Start(); }

  virtual void
  HandleConnect(NetworkErrorCode _status)
  {
    if (_status == OK)
    {
      TerminateMessage* message = new TerminateMessage();
      SendMessage(message);
      return;
    }

    AddTimer(retry_cb_, 1000);
  }

  virtual void
  HandleClose(NetworkErrorCode)
  {
    Stop();
    getEventLoop()->loopExit();
  }

 private:

  VCallback<>         retry_cb_;
};

static OrderServer*   server_;

void
start_server(sp_uint32 port)
{
  NetworkOptions options;
  options.set_host(LOCALHOST);
  options.set_port(port);
  options.set_max_packet_size(1024 * 1024);
  options.set_socket_family(PF_INET);

  EventLoopImpl ss;
  server_ = new OrderServer(&ss, options);
  if (server_->Start() != 0) GTEST_FAIL();
  ss.loop();
}

void
start_client(sp_uint32 port, sp_uint64 requests)
{
  NetworkOptions options;
  options.set_host(LOCALHOST);
  options.set_port(port);
  options.set_max_packet_size(1024 * 1024);
  options.set_socket_family(PF_INET);

  EventLoopImpl ss;
  OrderClient client(&ss, options, requests);
  client.Start();
  ss.loop();
}

void
terminate_server(sp_uint32 port)
{
  NetworkOptions options;
  options.set_host(LOCALHOST);
  options.set_port(port);
  options.set_max_packet_size(1024 * 1024);
  options.set_socket_family(PF_INET);

  EventLoopImpl ss;
  Terminate ts(&ss, options);
  ts.Start();
  ss.loop();
}

void
start_test(sp_int32 nclients, sp_uint64 requests)
{
  using namespace std::chrono;

  // start the server thread
  std::thread sthread(start_server, SERVER_PORT);

  auto start = high_resolution_clock::now();

  // start the client threads
  std::vector<std::thread> cthreads;
  for (sp_int32 i = 0 ; i < nclients ; i++)
  {
    cthreads.push_back(std::thread(start_client, SERVER_PORT, requests));
  }

  // wait for the client threads to terminate
  for (auto& thread : cthreads)
  {
    thread.join();
  }

  auto stop = high_resolution_clock::now();

  // now send a terminate message to server
  terminate_server(SERVER_PORT);
  sthread.join();

  ASSERT_TRUE(server_->sent_pkts() == server_->recv_pkts());
  ASSERT_TRUE(server_->sent_pkts() == nclients*requests);

  std::cout
    << nclients << " client(s) exchanged a total of "
    << requests << " in "
    << duration_cast<milliseconds>(stop-start).count() << " ms."
    << std::endl;

  delete server_;
}

// Test ordering with 1 client and 1 server
TEST(NetworkTest, test_switch_1)
{
  start_test(1, 100);
}

TEST(NetworkTest, test_switch_2)
{
  start_test(1, 1000);
}

TEST(NetworkTest, test_switch_3)
{
  start_test(1, 10000);
}

int
main(int argc, char **argv)
{
  heron::common::Initialize(argv[0]);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
