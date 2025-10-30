# PowerShell build script for MSVC
$root = Resolve-Path ".."
$build = Join-Path $root "build-msvc"
$dist  = Join-Path $root "dist-msvc"

Remove-Item -Recurse -Force $build, $dist -ErrorAction Ignore
New-Item -ItemType Directory -Force -Path $build, $dist | Out-Null

cmake -S $root -B $build -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_BUILD_TYPE=Release `
  -DUA_BUILD_BENCH=ON `
  -DUA_ENABLE_AVX2=ON `
  -DUA_ENABLE_AVX512=ON `
  -DUA_BUILD_STATIC=ON `
  -DUA_BUILD_SHARED=ON `
  -DCMAKE_INSTALL_PREFIX=$dist

cmake --build $build --config Release --target INSTALL
