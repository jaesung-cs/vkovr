#ifndef VKOVR_DEMO_VR_WORKER_H_
#define VKOVR_DEMO_VR_WORKER_H_

#include <thread>

namespace demo
{
namespace engine
{
class VrWorker
{
public:
  VrWorker();
  ~VrWorker();

  void run();
  void terminate();

private:
  void loop();

  std::atomic_bool shouldTerminate_ = false;

  std::thread thread_;
};
}
}

#endif // VKOVR_DEMO_VR_WORKER_H_
