#include <algorithm>
#include <vector>
#include "IP.h"
using namespace IP;
using std::vector;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_median:
//
// Apply median filter of size sz x sz to I1.
// Clamp sz to 9.
// Output is in I2.
//
void HW_median(ImagePtr I1, int sz, ImagePtr I2) {

    // ensure valid filter size
    if (sz < 1) sz = 1;
    if ((sz & 1) == 0) sz++;
    if (sz > 9) sz = 9;

    const int width = I1->width();
    const int height = I1->height();
    const int numChannels = I1->maxDepth();

    // prepare output image
    IP_copyImageHeader(I1, I2);

    const int halfSize = sz / 2;
    const int windowArea = sz * sz;

    // apply median filter to each channel
    for (int ch = 0; ch < numChannels; ++ch) {
        ChannelPtr<uchar> src, dst;
        int type;
        IP_getChannel(I1, ch, src, type);
        IP_getChannel(I2, ch, dst, type);

        std::vector<uchar> neighborhood(windowArea);

        for (int row = 0; row < height; ++row) {
            for (int col = 0; col < width; ++col) {
                // collect neighborhood pixels
                int index = 0;
                for (int offsetY = -halfSize; offsetY <= halfSize; ++offsetY) {
                    int sampleY = row + offsetY;
                    if (sampleY < 0) sampleY = 0;
                    if (sampleY >= height) sampleY = height - 1;

                    for (int offsetX = -halfSize; offsetX <= halfSize; ++offsetX) {
                        int sampleX = col + offsetX;
                        if (sampleX < 0) sampleX = 0;
                        if (sampleX >= width) sampleX = width - 1;

                        neighborhood[index++] = src[sampleY * width + sampleX];
                    }
                }

                // sort and pick median
                std::nth_element(neighborhood.begin(), neighborhood.begin() + windowArea / 2, neighborhood.end());
                dst[row * width + col] = neighborhood[windowArea / 2];
            }
        }
    }
}
