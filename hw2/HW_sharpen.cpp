#include "IP.h"
using namespace IP;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_sharpen:
//
// Sharpen image I1. Output is in I2.
//

static inline uchar clipToByte(double v) {
    if (v < 0.0) return 0;
    if (v > 255.0) return 255;
    return static_cast<uchar>(v + 0.5); // round
}

void HW_sharpen(ImagePtr I1, int size, double factor, ImagePtr I2) {

    // make sure filter size is odd 
    if (size < 1) size = 1;
    if ((size & 1) == 0) size++;

    const int width = I1->width();
    const int height = I1->height();
    const int numChannels = I1->maxDepth();

    // create blurred image using size x size box filter
    ImagePtr blurred;
    HW_blur(I1, size, size, blurred);

    // prepare output image
    IP_copyImageHeader(I1, I2);

    // sharpen for each channel, then clip
    for (int ch = 0; ch < numChannels; ++ch) {
        ChannelPtr<uchar> src, blur, dst;
        int type;
        IP_getChannel(I1, ch, src, type);
        IP_getChannel(blurred, ch, blur, type);
        IP_getChannel(I2, ch, dst, type);

        for (int y = 0; y < height; ++y) {
            const int rowOffset = y * width;
            for (int x = 0; x < width; ++x) {
                const int idx = rowOffset + x;
                const double s = static_cast<double>(src[idx]);
                const double b = static_cast<double>(blur[idx]);
                const double out = s + factor * (s - b); // unsharp mask
                dst[idx] = clipToByte(out); // clip to [0, 255]
            }
        }
    }
}
