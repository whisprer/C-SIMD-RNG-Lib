#include <ua/ua_rng.h>
#include <cstdio>
int main() {
  ua::Rng rng(123);
  double x[4]; rng.uniform(x, 4);
  std::printf("%.9f %.9f %.9f %.9f\n", x[0], x[1], x[2], x[3]);
  return 0;
}
