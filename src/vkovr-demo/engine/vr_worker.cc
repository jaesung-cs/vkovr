#include <vkovr-demo/engine/vr_worker.h>

#include <iostream>
#include <chrono>

namespace demo
{
namespace engine
{
VrWorker::VrWorker()
{
}

VrWorker::~VrWorker()
{
  terminate();
  thread_.join();
}

void VrWorker::run()
{
  thread_ = std::thread([=] {loop(); });
}

void VrWorker::loop()
{
  while (!shouldTerminate_)
  {
    std::cout << "VR loop is running" << std::endl;

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(1s);
  }
}

void VrWorker::terminate()
{
  shouldTerminate_ = true;
}
}
}
