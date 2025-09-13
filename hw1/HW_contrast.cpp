#include "IP.h"
using namespace IP;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_contrast:
//
// Apply contrast enhancement to I1. Output is in I2.
// Stretch intensity difference from reference value (128) by multiplying
// difference by "contrast" and adding it back to 128. Shift result by
// adding "brightness" value.
//
void HW_contrast(ImagePtr I1, double brightness, double contrast, ImagePtr I2) {

    // copy image header (width, height) of the input image I1 to the output image I2
    IP_copyImageHeader(I1, I2);

    // initialize variables width, height, and total number of pixels
    int w = I1->width();
    int h = I1->height();
    int total = w * h;

    // initialize lookup table
    int lut[MXGRAY];
    for (int i = 0; i < MXGRAY; ++i) {
        // apply contrast and brightness adjustment
        double v = (i - 128.0) * contrast + 128.0 + brightness;
        // round to nearest integer and clamp
        int q = (int)floor(v + 0.5);
        if (q < 0) q = 0;
        if (q > MaxGray) q = MaxGray;
        lut[i] = q;
    }

    ChannelPtr<uchar> p1, p2; // image channel pointer (uchar as signed doesnt matter in this case)
    int type;

    // apply LUT to each channel
    for (int ch = 0; IP_getChannel(I1, ch, p1, type); ch++) {
        IP_getChannel(I2, ch, p2, type);
        for (int i = 0; i < total; i++) {
            *p2++ = (uchar) lut[*p1++];
        }
    }
}
