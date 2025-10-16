#include "IP.h"
#include <cstdlib>
#include <cmath>
using namespace IP;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_quantize:
//
// Quantize I1 to specified number of levels. Apply dither if flag is set.
// Output is in I2.
//
void HW_quantize(ImagePtr I1, int levels, bool dither, ImagePtr I2) {

    // copy image header (width, height) of the input image I1 to the output image I2
    IP_copyImageHeader(I1, I2);

    // initialize variables width, height, and total number of pixels
    int w = I1->width();
    int h = I1->height();
    int total = w * h;

    // clamp levels to be at least 2
    if (levels < 2) levels = 2;
    if (levels > MXGRAY) levels = MXGRAY;

    // compute uniform bin size and midpoint bias
    // step size = 256 / levels, levels = 128 / levels
    double step = 256.0 / levels;
    double bias = 128.0 / levels;

    ChannelPtr<uchar> p1, p2; // image channel pointer (uchar as signed doesnt matter in this case)
    int type;

    if (!dither) {
        // if no dithering, use lookup table
        int lut[MXGRAY];
        for (int i = 0; i < MXGRAY; ++i) {
            int k = (int)(i / step);
            if (k >= levels) k = levels - 1; // clamp to max level

            // midpoint of bin k = bias + k * step
            double mid = bias + k * step;

            // round to nearest integer and clamp
            int q = (int)(mid + 0.5);
            if (q < 0) q = 0;
            if (q > MaxGray) q = MaxGray;

            lut[i] = q;
        }

        // apply LUT to each channel
        for (int ch = 0; IP_getChannel(I1, ch, p1, type); ch++) {
            IP_getChannel(I2, ch, p2, type);
            for (int i = 0; i < total; i++) {
                *p2++ = (uchar)lut[*p1++];
            }
        }
    }
    else {
        // if dithering
        for (int ch = 0; IP_getChannel(I1, ch, p1, type); ch++) {
            IP_getChannel(I2, ch, p2, type);

            // alternate sign across pixels
            bool addSign = true;

            for (int i = 0; i < total; i++) {
                // base value
                double v = (double)(*p1++);

                // jitter j in [0, bias]
                double j = (rand() / (double)RAND_MAX) * bias;

                // alternate the sign
                double vj;
                if (addSign) {
                    vj = v + j;
                }
                else {
                    vj = v - j;
                }
                addSign = !addSign;

                // clamp vj to [0, 255]
                if (vj < 0.0) vj = 0.0;
                if (vj > 255.0) vj = 255.0;
                 
                // find bin index for jittered value
                int k = (int)(vj / step);
                if (k >= levels) k = levels - 1; // clamp to max level

                // midpoint of bin k, rounded
                double mid = bias + k * step;
                int q = (int)(mid + 0.5);
                if (q < 0) q = 0;
                if (q > MaxGray) q = MaxGray;

                *p2++ = (uchar)q;
            }

        }
    }
}
