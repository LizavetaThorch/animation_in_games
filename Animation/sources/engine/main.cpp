#include "application.h"

int main(int, char**)
{
  init_application("animations", 2048, 1024, true);

  main_loop();

  close_application();

  return 0;
}
