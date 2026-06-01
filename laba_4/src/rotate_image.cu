#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cstdlib>
#include <iostream>
#include <chrono>
#include "rotate_image.hpp"

void rotateImageCPU(const unsigned char* input, unsigned char* output, int width, int height) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            output[y * width + x] = input[(height - 1 - x) * width + y];
        }
    }
}

__global__ void rotateImageCUDA(const unsigned char* input, unsigned char* output, int width, int height) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    if (x < width && y < height) {
        output[y * width + x] = input[(height - 1 - x) * width + y];
    }
}

void runRotateImage() {
    const int WIDTH  = 512;
    const int HEIGHT = 512;
    size_t size = WIDTH * HEIGHT * sizeof(unsigned char);

    unsigned char* h_input   = new unsigned char[WIDTH * HEIGHT];
    unsigned char* h_out_cpu = new unsigned char[WIDTH * HEIGHT];
    unsigned char* h_out_gpu = new unsigned char[WIDTH * HEIGHT];

    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        h_input[i] = static_cast<unsigned char>(rand() % 256);
    }

    auto cpuStart = std::chrono::high_resolution_clock::now();
    rotateImageCPU(h_input, h_out_cpu, WIDTH, HEIGHT);
    auto cpuEnd = std::chrono::high_resolution_clock::now();
    double cpuTime = std::chrono::duration<double, std::milli>(cpuEnd - cpuStart).count();

    unsigned char* d_input;
    unsigned char* d_output;
    cudaMalloc((void**)&d_input,  size);
    cudaMalloc((void**)&d_output, size);
    cudaMemcpy(d_input, h_input, size, cudaMemcpyHostToDevice);

    dim3 threadsPerBlock(16, 16);
    dim3 blocksPerGrid(
        (WIDTH  + threadsPerBlock.x - 1) / threadsPerBlock.x,
        (HEIGHT + threadsPerBlock.y - 1) / threadsPerBlock.y
    );

    cudaEvent_t gpuStart, gpuStop;
    cudaEventCreate(&gpuStart);
    cudaEventCreate(&gpuStop);
    cudaEventRecord(gpuStart);

    rotateImageCUDA<<<blocksPerGrid, threadsPerBlock>>>(d_input, d_output, WIDTH, HEIGHT);

    cudaEventRecord(gpuStop);
    cudaEventSynchronize(gpuStop);
    float gpuTime = 0.0f;
    cudaEventElapsedTime(&gpuTime, gpuStart, gpuStop);

    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess)
        std::cerr << "CUDA kernel error (rotateImage): " << cudaGetErrorString(err) << std::endl;

    cudaDeviceSynchronize();

    cudaMemcpy(h_out_gpu, d_output, size, cudaMemcpyDeviceToHost);

    bool correct = true;
    for (int i = 0; i < WIDTH * HEIGHT; i++) {
        if (h_out_cpu[i] != h_out_gpu[i]) {
            correct = false;
            break;
        }
    }

    std::cout << "\n=== Task 2: Image Rotation 90 CW ===" << std::endl;
    std::cout << "Image    : " << WIDTH << "x" << HEIGHT << std::endl;
    std::cout << "CPU time : " << cpuTime << " ms" << std::endl;
    std::cout << "GPU time : " << gpuTime << " ms" << std::endl;
    std::cout << "Speedup  : " << cpuTime / gpuTime << "x" << std::endl;
    std::cout << "Correct  : " << (correct ? "YES" : "NO") << std::endl;

    std::cout << "CPU[0..4]: ";
    for (int i = 0; i < 5; i++) std::cout << static_cast<int>(h_out_cpu[i]) << " ";
    std::cout << std::endl;

    std::cout << "GPU[0..4]: ";
    for (int i = 0; i < 5; i++) std::cout << static_cast<int>(h_out_gpu[i]) << " ";
    std::cout << std::endl;

    cudaFree(d_input);
    cudaFree(d_output);
    cudaEventDestroy(gpuStart);
    cudaEventDestroy(gpuStop);
    delete[] h_input;
    delete[] h_out_cpu;
    delete[] h_out_gpu;
}