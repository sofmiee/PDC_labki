#pragma once

#include <cuda_runtime.h>

void powerArrayCPU(const float* A, float* B, int N, float p);

__global__ void powerArrayCUDA(const float* A, float* B, int N, float p);

void runPowerArray();
