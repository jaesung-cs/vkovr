#ifndef VKOVR_SESSION_H_
#define VKOVR_SESSION_H_

#include <vector>
#include <string>
#include <memory>

#include <vulkan/vulkan.hpp>

#include <OVR_CAPI_Vk.h>

namespace vkovr
{
class Swapchain;
class SwapchainCreateInfo;

class SessionCreateInfo;

class Session
{
  friend Session createSession(const SessionCreateInfo& createInfo);

public:
  Session();
  ~Session();

  Swapchain createSwapchain(const SwapchainCreateInfo& createInfo);

  bool opened();

  auto getFrameIndex() const { return frameIndex_; }

  operator ovrSession() const { return session_; }
  const auto& getOculusProperties() const { return hmdDesc_; }

  std::vector<std::string> getInstanceExtensions();
  vk::PhysicalDevice getPhysicalDevice(vk::Instance instance);
  std::vector<std::string> getDeviceExtensions();
  ovrSessionStatus getStatus() const;
  ovrInputState getInputState() const;

  void synchronizeWithQueue(vk::Queue queue);

  std::vector<ovrPosef> getEyePoses();
  void getTouchPoses();
  ovrMatrix4f getEyeProjection(ovrEyeType eye, float near, float far);

  void beginFrame();
  void endFrame(const std::vector<Swapchain>& swapchains);

  void recenter();

  void destroy();

private:
  ovrSession session_ = nullptr;
  ovrGraphicsLuid luid_{};
  ovrHmdDesc hmdDesc_{};

  int64_t frameIndex_ = 0;

  std::vector<ovrPosef> eyeRenderPoses_;
  ovrTimewarpProjectionDesc posTimewarpProjectionDesc_;
  double sensorSampleTime_ = 0.f;
};

class SessionCreateInfo
{
public:
  SessionCreateInfo() = default;

public:
};

Session createSession(const SessionCreateInfo& createInfo);
}

#endif // VKOVR_SESSION_H_
