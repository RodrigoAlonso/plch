//clock.cpp
//
// Monotonic system clock, in centiseconds. Kept apart from the rest of the
// engine so that windows.h does not leak into it.

#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

namespace plcsim_detail {

uint32_t system_clock(void *)
{
#ifdef _WIN32
  return (uint32_t)(GetTickCount64()/10);
#else
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC,&ts);
  return (uint32_t)((uint64_t)ts.tv_sec*100 + ts.tv_nsec/10000000);
#endif
}

} // namespace plcsim_detail
