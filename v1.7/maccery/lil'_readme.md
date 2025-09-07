\# UA RNG v1.7



A fast, portable C++20 random number library with \*\*runtime SIMD dispatch\*\*:



\- Scalar baseline (xoshiro256\*\*)

\- AVX2 4× lanes

\- AVX-512F 8× lanes (optional)

\- Streams: `u64`, `\[0,1)` `double`, and `N(0,1)` normals (polar)

\- `jump()` for 2^128 subsequence



\## Usage



```cpp

\#include "ua/ua\_rng.h"

ua::Rng rng(1234);

std::vector<uint64\_t> v(1<<20);

rng.generate\_u64(v.data(), v.size());



