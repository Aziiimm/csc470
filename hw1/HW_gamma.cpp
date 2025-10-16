#include "IP.h"
#include <cmath>
using namespace IP;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_gammaCorrect:
//
// Gamma correct image I1. Output is in I2.
//
void HW_gammaCorrect(ImagePtr I1, double gamma, ImagePtr I2) {

    // copy image header (width, height) of the input image I1 to the output image I2
    IP_copyImageHeader(I1, I2);

    // initialize variables width, height, and total number of pixels
    int w = I1->width();
    int h = I1->height();
    int total = w * h;

    // prevent invalid gamma values
    if (gamma <= 0.0) gamma = 1.0;

    double exponent = 1.0 / gamma;

    // initialize lookup table
    int lut[MXGRAY];
    for (int i = 0; i <= MaxGray; ++i) {
        // normalized value in [0, 1]
        // apply gamma correction
        lut[i] = MaxGray * pow(((double) i/ MaxGray), exponent);
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
