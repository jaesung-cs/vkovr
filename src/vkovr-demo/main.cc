#include <vkovr-demo/application.h>

#include <iostream>
#include <stdexcept>

int main()
{
  demo::Application application;

  try
  {
    application.run();
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << std::endl;
  }

  return 0;
}
