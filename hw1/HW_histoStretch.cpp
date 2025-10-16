#include "IP.h"
using namespace IP;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_histoStretch:
//
// Apply histogram stretching to I1. Output is in I2.
// Stretch intensity values between t1 and t2 to fill the range [0,255].
//
void HW_histoStretch(ImagePtr I1, int t1, int t2, ImagePtr I2) {

    // copy image header (width, height) of the input image I1 to the output image I2
    IP_copyImageHeader(I1, I2);

    // initialize variables for width, height, and total number of pixels
    int w = I1->width();
    int h = I1->height();
    int total = w * h;

    // clamp t1 and t2 to [0, 255] range
    if (t1 < 0) t1 = 0;
    if (t1 > MaxGray) t1 = MaxGray;
    if (t2 < 0) t2 = 0;
    if (t2 > MaxGray) t2 = MaxGray;

    // swap t1 and t2 if t1 is larger
    if (t1 > t2) {
        int temp = t1;
        t1 = t2;
        t2 = temp;
    }

    // initialize lookup table
    int lut[MXGRAY];
    int delta = t2 - t1;
    if (delta <= 0) delta = 1; // prevent division by zero

    for (int i = 0; i < MXGRAY; ++i) {
        int q;
        if (i < t1) q = 0;
        else if (i > t2) q = MaxGray;
        else {
            // stretch to [0, 255]
            double v = (double)(i - t1) * MaxGray / (double)delta;
            // round to nearest integer and clamp
            q = (int)(v + 0.5);
            if (q < 0) q = 0;
            if (q > MaxGray) q = MaxGray;
        }
        lut[i] = q;
    }

    ChannelPtr<uchar> p1, p2; // image channel pointer (uchar as signed doesnt matter in this case)
    int type;
    // apply LUT to each channel
    for (int ch = 0; IP_getChannel(I1, ch, p1, type); ch++) {
        IP_getChannel(I2, ch, p2, type);
        for (int i = 0; i < total; i++) {
            *p2++ = (uchar)lut[*p1++];
        }
    }
}

