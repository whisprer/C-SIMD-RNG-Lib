#include <cstdint>
#include <vector>
#include <string>
#include <CL/cl.h>

static const char* KERNEL_SRC = R"CLC(
__kernel void ua_cl_philox(__global ulong* out, ulong seed, ulong n) {
  uint gid = get_global_id(0);
  if ((ulong)gid >= n) return;
  uint c0 = gid;
  uint c1 = 0, c2 = 0, c3 = 0;
  uint k0 = (uint)(seed ^ 0x9E3779B9u);
  uint k1 = (uint)((seed>>32) ^ 0xBB67AE85u);
  for (int i=0;i<10;++i) {
    uint lo0 = c0 * 0xD2511F53u;
    uint lo1 = c2 * 0xCD9E8D57u;
    uint hi0 = mul_hi(c0, 0xD2511F53u);
    uint hi1 = mul_hi(c2, 0xCD9E8D57u);
    uint r0 = hi1 ^ c1 ^ k0;
    uint r1 = lo1;
    uint r2 = hi0 ^ c3 ^ k1;
    uint r3 = lo0;
    c0=r0; c1=r1; c2=r2; c3=r3;
    k0 += 0x9E3779B9u; k1 += 0xBB67AE85u;
  }
  ulong v = ((ulong)c0 << 32) | (ulong)c1;
  out[gid] = v;
}
)CLC";

// Return 0 on success, negative on error.
extern "C" int ua_opencl_philox_generate_u64(uint64_t seed, uint64_t* host_out, size_t n) {
  cl_int err;
  cl_uint nump=0; err=clGetPlatformIDs(0,nullptr,&nump); if(err!=CL_SUCCESS || nump==0) return -1;
  std::vector<cl_platform_id> plats(nump); clGetPlatformIDs(nump, plats.data(), nullptr);
  cl_platform_id plat = plats[0];

  cl_uint numd=0; clGetDeviceIDs(plat, CL_DEVICE_TYPE_GPU, 0, nullptr, &numd); if(numd==0) return -2;
  std::vector<cl_device_id> devs(numd); clGetDeviceIDs(plat, CL_DEVICE_TYPE_GPU, numd, devs.data(), nullptr);
  cl_device_id dev = devs[0];

  cl_context ctx = clCreateContext(nullptr,1,&dev,nullptr,nullptr,&err); if(!ctx) return -3;
  cl_command_queue q = clCreateCommandQueue(ctx, dev, 0, &err); if(!q){ clReleaseContext(ctx); return -4; }

  const char* src = KERNEL_SRC; size_t len = std::strlen(KERNEL_SRC);
  cl_program prog = clCreateProgramWithSource(ctx, 1, &src, &len, &err);
  if (!prog) { clReleaseCommandQueue(q); clReleaseContext(ctx); return -5; }
  err = clBuildProgram(prog, 1, &dev, "", nullptr, nullptr);
  if (err != CL_SUCCESS) { clReleaseProgram(prog); clReleaseCommandQueue(q); clReleaseContext(ctx); return -6; }

  cl_kernel ker = clCreateKernel(prog, "ua_cl_philox", &err);
  if (!ker) { clReleaseProgram(prog); clReleaseCommandQueue(q); clReleaseContext(ctx); return -7; }

  cl_mem buf = clCreateBuffer(ctx, CL_MEM_WRITE_ONLY, n*sizeof(uint64_t), nullptr, &err);
  if (!buf) { clReleaseKernel(ker); clReleaseProgram(prog); clReleaseCommandQueue(q); clReleaseContext(ctx); return -8; }

  err  = clSetKernelArg(ker, 0, sizeof(cl_mem), &buf);
  err |= clSetKernelArg(ker, 1, sizeof(cl_ulong), &seed);
  cl_ulong narg = (cl_ulong)n;
  err |= clSetKernelArg(ker, 2, sizeof(cl_ulong), &narg);
  if (err != CL_SUCCESS) { clReleaseMemObject(buf); clReleaseKernel(ker); clReleaseProgram(prog); clReleaseCommandQueue(q); clReleaseContext(ctx); return -9; }

  size_t gsize = ((n+255)/256)*256;
  size_t lsize = 256;
  err = clEnqueueNDRangeKernel(q, ker, 1, nullptr, &gsize, &lsize, 0, nullptr, nullptr);
  if (err != CL_SUCCESS) { clReleaseMemObject(buf); clReleaseKernel(ker); clReleaseProgram(prog); clReleaseCommandQueue(q); clReleaseContext(ctx); return -10; }

  err = clEnqueueReadBuffer(q, buf, CL_TRUE, 0, n*sizeof(uint64_t), host_out, 0, nullptr, nullptr);

  clReleaseMemObject(buf); clReleaseKernel(ker); clReleaseProgram(prog); clReleaseCommandQueue(q); clReleaseContext(ctx);
  return (err==CL_SUCCESS) ? 0 : -11;
}
