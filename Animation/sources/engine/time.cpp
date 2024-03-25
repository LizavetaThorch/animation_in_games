#include <chrono>


using time_point = std::chrono::high_resolution_clock::time_point;

static time_point startTime, curTime;
static float savedTime, deltaTime; // in seconds

void start_time()
{
  curTime = startTime = std::chrono::high_resolution_clock::now();
  savedTime = deltaTime = 0.f;
}

void update_time()
{
  time_point newTime = std::chrono::high_resolution_clock::now();
  std::chrono::duration<float> d = newTime - curTime;
  deltaTime = d.count();
  curTime = newTime;
  d = curTime - startTime;
  savedTime = d.count();
}

float get_time()
{
  return savedTime;
}

float get_delta_time()
{
  return deltaTime;
}
