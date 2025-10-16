#include "IP.h"
using namespace IP;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_clip:
//
// Clip intensities of image I1 to [t1,t2] range. Output is in I2.
// If    input<t1: output = t1;
// If t1<input<t2: output = input;
// If      val>t2: output = t2;
//
void HW_clip(ImagePtr I1, int t1, int t2, ImagePtr I2) {

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
        int temp = t1; t1 = t2; t2 = temp;
    }

    // initialize lookup table
    int lut[MXGRAY];
    for (int i = 0; i < MXGRAY; ++i) {
        if (i < t1) lut[i] = t1;
        else if (i > t2) lut[i] = t2;
        else lut[i] = i; // within [t1, t2] keep original value
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
