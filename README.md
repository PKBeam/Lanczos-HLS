# Lanczos-HLS
A Lanczos image upscaling implementation in C for Vivado HLS

Inspired by AMD's FidelityFX Super Resolution 1.0.

This module takes an input image and applies a Lanczos upscaling algorithm on it, outputting a higher resolution image.

Includes both software and HLS implementations.

(has some crazy memory optimisations since the target FPGA board is the Zynq 7000 with <4MB of memory)
