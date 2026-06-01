#pragma once

#include <cuda_runtime.h>

void rotateImageCPU(const unsigned char* input, unsigned char* output, int width, int height);

__global__ void rotateImageCUDA(const unsigned char* input, unsigned char* output, int width, int height);

void runRotateImage();
