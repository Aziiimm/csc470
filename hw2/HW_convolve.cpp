#include "IP.h"
using namespace IP;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_convolve:
//
// Convolve I1 with filter kernel in Ikernel.
// Output is in I2.
//
void HW_convolve(ImagePtr I1, ImagePtr Ikernel, ImagePtr I2) {

    const int width = I1->width();
    const int height = I1->height();
    const int numChannels = I1->maxDepth();

    // get kernel info
    const int kernelW = Ikernel->width();
    const int kernelH = Ikernel->height();
    const int halfW = kernelW / 2;
    const int halfH = kernelH / 2;

    // get pointer to kernel data
    ChannelPtr<float> kernelData;
    int type;
    IP_getChannel(Ikernel, 0, kernelData, type);

    // prepare output
    IP_copyImageHeader(I1, I2);

    // convolve each channel
    for (int ch = 0; ch < numChannels; ++ch) {
        ChannelPtr<uchar> src, dst;
        IP_getChannel(I1, ch, src, type);
        IP_getChannel(I2, ch, dst, type);

        for (int row = 0; row < height; ++row) {
            for (int col = 0; col < width; ++col) {
                double sum = 0.0;

                // apply kernel
                for (int ky = -halfH; ky <= halfH; ++ky) {
                    int sampleY = row + ky;
                    if (sampleY < 0) sampleY = 0;
                    if (sampleY >= height) sampleY = height - 1;

                    for (int kx = -halfW; kx <= halfW; ++kx) {
                        int sampleX = col + kx;
                        if (sampleX < 0) sampleX = 0;
                        if (sampleX >= width) sampleX = width - 1;

                        float kernelValue = kernelData[(ky + halfH) * kernelW + (kx + halfW)];
                        sum += kernelValue * src[sampleY * width + sampleX];
                    }
                }
                // clamp and assign to output
                if (sum < 0.0) sum = 0.0;
                if (sum > 255.0) sum = 255.0;

                dst[row * width + col] = static_cast<uchar>(sum + 0.5); // round
            }
        }
    }
}
