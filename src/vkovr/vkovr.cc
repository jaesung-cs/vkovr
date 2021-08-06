#include <vkovr/vkovr.hpp>

namespace vkovr
{
void initialize()
{
  // Initializes LibOVR, and the Rift
  ovrInitParams initParams = { ovrInit_RequestVersion | ovrInit_FocusAware, OVR_MINOR_VERSION, NULL, 0, 0 };
  auto result = ovr_Initialize(&initParams);
  if (!OVR_SUCCESS(result))
    throw std::runtime_error("Failed to initialize libOVR");
}

void terminate()
{
  ovr_Shutdown();
}
}
