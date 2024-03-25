#include "application.h"


extern void init_application(const char *project_name, int width, int height, bool full_screen);
extern void close_application();
extern void main_loop();

int main(int, char**)
{
  init_application("animations", 2048, 1024, true);

  main_loop();

  close_application();

  return 0;
}