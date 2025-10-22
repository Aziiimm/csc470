#include "IP.h"
#include <vector>
#include <algorithm>
using namespace IP;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_convolve:
//
// Convolve I1 with filter kernel in Ikernel.
// Output is in I2.
//

// replicate-pad an input channel to a new buffer
static void makeReplicatePadded(
    const uchar* src, int w, int h, int padX, int padY,
    std::vector<uchar>& padded, int& pw, int& ph)
{
    pw = w + 2 * padX;
    ph = h + 2 * padY;
    padded.assign(pw * ph, 0);

    // copy interior
    for (int y = 0; y < h; ++y) {
        uchar* dstRow       = &padded[(y + padY) * pw + padX];
        const uchar* srcRow = &src[y * w];
        std::copy(srcRow, srcRow + w, dstRow);
    }

    // replicate left/right borders
    for (int y = 0; y < h; ++y) {
        uchar* row = &padded[(y + padY) * pw];
        // left pad
        std::fill(row, row + padX, row[padX]);
        // right pad
        std::fill(row + padX + w, row + padX + w + padX, row[padX + w - 1]);
    }

    // replicate top/bottom rows
    // top
    for (int y = 0; y < padY; ++y) {
        std::copy(&padded[(padY) * pw], &padded[(padY + 1) * pw],
                  &padded[y * pw]);
    }
    // bottom
    for (int y = 0; y < padY; ++y) {
        std::copy(&padded[(padY + h - 1) * pw], &padded[(padY + h) * pw],
                  &padded[(padY + h + y) * pw]);
    }
}

void HW_convolve(ImagePtr I1, ImagePtr Ikernel, ImagePtr I2) {

    const int width       = I1->width();
    const int height      = I1->height();
    const int numChannels = I1->maxDepth();

    // get kernel info
    const int kernelW = Ikernel->width();
    const int kernelH = Ikernel->height();
    const int halfW   = kernelW / 2;
    const int halfH   = kernelH / 2;

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

        // build a replicate-padded buffer once (follows professor's padding handout)
        std::vector<uchar> padded;
        int paddedW = 0, paddedH = 0;
        makeReplicatePadded(src, width, height, halfW, halfH, padded, paddedW, paddedH);

        for (int row = 0; row < height; ++row) {
            for (int col = 0; col < width; ++col) {
                double sum = 0.0;

                // apply kernel
                // kernel center in padded image is (col+halfW, row+halfH)
                const int pcy = row + halfH;
                const int pcx = col + halfW;

                for (int ky = -halfH; ky <= halfH; ++ky) {
                    const int py   = pcy + ky;
                    const int krow = (ky + halfH) * kernelW;
                    const uchar* prow = &padded[py * paddedW];

                    for (int kx = -halfW; kx <= halfW; ++kx) {
                        const int px = pcx + kx;
                        float kernelValue = kernelData[krow + (kx + halfW)];
                        sum += kernelValue * prow[px];
                    }
                }

                // clamp and assign to output
                if (sum < 0.0)   sum = 0.0;
                if (sum > 255.0) sum = 255.0;
                dst[row * width + col] = static_cast<uchar>(sum + 0.5); // round
            }
        }
    }
}
