#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include "power_array.hpp"

void powerArrayCPU(const float* A, float* B, int N, float p) {
    for (int i = 0; i < N; i++) {
        B[i] = powf(A[i], p);
    }
}

__global__ void powerArrayCUDA(const float* A, float* B, int N, float p) {
    int idx = threadIdx.x + blockIdx.x * blockDim.x;
    if (idx < N) {
        B[idx] = powf(A[idx], p);
    }
}

void runPowerArray() {
    const int N = 500000;
    const float p = 0.5f;
    size_t size = N * sizeof(float);

    float* h_A     = new float[N];
    float* h_B_cpu = new float[N];
    float* h_B_gpu = new float[N];

    for (int i = 0; i < N; i++) {
        h_A[i] = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 100.0f + 0.01f;
    }

    auto cpuStart = std::chrono::high_resolution_clock::now();
    powerArrayCPU(h_A, h_B_cpu, N, p);
    auto cpuEnd = std::chrono::high_resolution_clock::now();
    double cpuTime = std::chrono::duration<double, std::milli>(cpuEnd - cpuStart).count();

    float* d_A;
    float* d_B;
    cudaMalloc((void**)&d_A, size);
    cudaMalloc((void**)&d_B, size);
    cudaMemcpy(d_A, h_A, size, cudaMemcpyHostToDevice);

    const int threadsPerBlock = 256;
    const int blocksPerGrid = (N + threadsPerBlock - 1) / threadsPerBlock;

    cudaEvent_t gpuStart, gpuStop;
    cudaEventCreate(&gpuStart);
    cudaEventCreate(&gpuStop);
    cudaEventRecord(gpuStart);

    powerArrayCUDA<<<blocksPerGrid, threadsPerBlock>>>(d_A, d_B, N, p);

    cudaEventRecord(gpuStop);
    cudaEventSynchronize(gpuStop);
    float gpuTime = 0.0f;
    cudaEventElapsedTime(&gpuTime, gpuStart, gpuStop);

    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess)
        std::cerr << "CUDA kernel error (powerArray): " << cudaGetErrorString(err) << std::endl;

    cudaDeviceSynchronize();

    cudaMemcpy(h_B_gpu, d_B, size, cudaMemcpyDeviceToHost);

    bool correct = true;
    for (int i = 0; i < N; i++) {
        if (fabsf(h_B_cpu[i] - h_B_gpu[i]) > 1e-3f) {
            correct = false;
            break;
        }
    }

    std::cout << "=== Task 1: Power Array ===" << std::endl;
    std::cout << "N = " << N << ", p = " << p << std::endl;
    std::cout << "CPU time : " << cpuTime << " ms" << std::endl;
    std::cout << "GPU time : " << gpuTime << " ms" << std::endl;
    std::cout << "Speedup  : " << cpuTime / gpuTime << "x" << std::endl;
    std::cout << "Correct  : " << (correct ? "YES" : "NO") << std::endl;

    std::cout << "CPU[0..4]: ";
    for (int i = 0; i < 5; i++) std::cout << h_B_cpu[i] << " ";
    std::cout << std::endl;

    std::cout << "GPU[0..4]: ";
    for (int i = 0; i < 5; i++) std::cout << h_B_gpu[i] << " ";
    std::cout << std::endl;

    cudaFree(d_A);
    cudaFree(d_B);
    cudaEventDestroy(gpuStart);
    cudaEventDestroy(gpuStop);
    delete[] h_A;
    delete[] h_B_cpu;
    delete[] h_B_gpu;
}