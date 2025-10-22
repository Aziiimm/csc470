#include <cmath>
#include <vector>
#include <algorithm>
#include "IP.h"

using namespace IP;

// helper: clip to [0, 255]
static inline uchar clipByte(double v) {
    if (v < 0.0) return 0;
    if (v > 255.0) return 255;
    return static_cast<uchar>(v + 0.5);
}

// quantize to 0/255 at threshold 128
static inline uchar quantizeBW(double v) {
    return (v < 128.0) ? 0 : 255;
}

// apply gamma correction ( out = 255 * (in/255)^gamma )
static inline short gammaCorrectU8(uchar s, double gamma) {
    if (gamma <= 0.0) gamma = 1.0;
    double lin = static_cast<double>(s) / 255.0;
    double corr = std::pow(lin, gamma);
    return static_cast<short>(std::lround(corr * 255.0));
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_errDiffusion:
//
// Apply error diffusion algorithm to image I1.
//
// This procedure produces a black-and-white dithered version of I1.
// Each pixel is visited and if it + any error that has been diffused to it
// is greater than the threshold, the output pixel is white, otherwise it is black.
// The difference between this new value of the pixel from what it used to be
// (somewhere in between black and white) is diffused to the surrounding pixel
// intensities using different weighting systems.
//
// Use Floyd-Steinberg     weights if method=0.
// Use Jarvis-Judice-Ninke weights if method=1.
//
// Use raster scan (left-to-right) if serpentine=0.
// Use serpentine order (alternating left-to-right and right-to-left) if serpentine=1.
// Serpentine scan prevents errors from always being diffused in the same direction.
//
// A circular buffer is used to pad the edges of the image.
// Since a pixel + its error can exceed the 255 limit of uchar, shorts are used.
//
// Apply gamma correction to I1 prior to error diffusion.
// Output is saved in I2.
//
void HW_errDiffusion(ImagePtr I1, int method, bool serpentine, double gamma, ImagePtr I2) {

    const int width = I1->width();
    const int height = I1->height();
    const int numChannels = I1->maxDepth();

    // prepare output image
    IP_copyImageHeader(I1, I2);

    // floyd-steinberg (radius 1 horizontally and vertically)
    struct Tap { int dx, dy; double w; };
    std::vector<Tap> taps;
    int padRadiusX = 0;
    int padRadiusY = 0;

    if (method == 1) {
        // jarvis-judice-ninke (normalize weights to sum = 1, denom = 48)
        // row y: (+1, 0): 7, (+2, 0): 5
        // row y+1: (-2, +1): 3, (-1, +1): 5, (0, +1): 7, (+1, +1): 5, (+2, +1): 3
        // row y+2: (-2, +2): 1, (-1, +2): 3, (0, +2): 5, (+1, +2): 3, (+2, +2): 1

        const double d = 48.0;
        taps = { 
            { +1,  0, 7.0 / d }, { +2,  0, 5.0 / d },
            { -2, +1, 3.0 / d }, { -1, +1, 5.0 / d }, { 0, +1, 7.0 / d }, { +1, +1, 5.0 / d }, { +2, +1, 3.0 / d },
            { -2, +2, 1.0 / d }, { -1, +2, 3.0 / d }, { 0, +2, 5.0 / d }, { +1, +2, 3.0 / d }, { +2, +2, 1.0 / d }
        };
        padRadiusX = 2;
        padRadiusY = 2;
    } else {
        // default to floyd-steinberg
        const double d = 16.0;
        taps = { 
            { +1,  0, 7.0 / d },
            { -1, +1, 3.0 / d }, { 0, +1, 5.0 / d }, { +1, +1, 1.0 / d }
        };
        padRadiusX = 1;
        padRadiusY = 1; // lookahead
    }

    // 3 row circular buffer, each row buffer length = width + 2 * padRadiusX
    const int paddedWidth = width + 2 * padRadiusX;
    const int bufferRows = 3;
    std::vector<short> rowBuf(bufferRows * paddedWidth, 0);

    // helper to index circular rows
    auto rowIndex = [&](int r)->short* { return &rowBuf[(r % bufferRows) * paddedWidth]; };

    // for serpentine scan, mirror the tap dx when going right to left
    auto maybeMirrorDx = [&](int dx, bool rightToLeft) {
        return rightToLeft ? -dx : dx;
    };

    // process each channel
    for (int ch = 0; ch < numChannels; ++ch) {
        ChannelPtr<uchar> src, dst;
        int type;
        IP_getChannel(I1, ch, src, type);
        IP_getChannel(I2, ch, dst, type);

        // init the 3 row circular buffer with rows y = 0 to 2
        auto loadInputRowToBuf = [&](int imgY, short* bufRow) {
            // zero pad entire row first
            std::fill(bufRow, bufRow + paddedWidth, 0);
            if (imgY < 0 || imgY >= height) return; // out of bounds
            // fill center part with gamma corrected input
            for (int x = 0; x < width; ++x) {
                bufRow[padRadiusX + x] = gammaCorrectU8(src[imgY * width + x], gamma);
            }
        };

        loadInputRowToBuf(0, rowIndex(0));
        loadInputRowToBuf(1, rowIndex(1));
        loadInputRowToBuf(2, rowIndex(2));

        for (int y = 0; y < height; ++y) {
            const bool rightToLeft = (serpentine && (y & 1));
            short* currRow = rowIndex(y);
            short* rowP1 = rowIndex(y + 1);
            short* rowP2 = rowIndex(y + 2);

            // compute output pixels
            if (!rightToLeft) {
                for (int x = 0; x < width; ++x) {
                    const int px = padRadiusX + x;
                    double val = static_cast<double>(currRow[px]);
                    uchar out = quantizeBW(val);
                    dst[y * width + x] = out;

                    // error to diffuse
                    double err = val - static_cast<double>(out);

                    // distribute error to neighbors
                    for (const auto& t : taps) {
                        int dx = maybeMirrorDx(t.dx, false);
                        int dy = t.dy;

                        // target buffer row based on dy
                        short* targetRow = (dy == 0) ? currRow : (dy == 1 ? rowP1 : rowP2);

                        int tx = px + dx;
                        if (tx < 0 || tx >= paddedWidth) continue;
                        int add = static_cast<int>(std::lround(err * t.w));
                        int tmp = static_cast<int>(targetRow[tx]) + add;
                        // clamp to short range
                        if (tmp < -32768) tmp = -32768;
                        if (tmp > 32767) tmp = 32767;
                        targetRow[tx] = static_cast<short>(tmp);                                      
                    }
                }
            } else {
                for (int x = width - 1; x >= 0; --x) { 
                    const int px = padRadiusX + x;
                    double val = static_cast<double>(currRow[px]);
                    uchar out = quantizeBW(val);
                    dst[y * width + x] = out;

                    double err = val - static_cast<double>(out);

                    for (const auto& t : taps) {
                        int dx = maybeMirrorDx(t.dx, true);
                        int dy = t.dy;

                        short* targetRow = (dy == 0) ? currRow : (dy == 1 ? rowP1 : rowP2);

                        int tx = px + dx;
                        if (tx < 0 || tx >= paddedWidth) continue;
                        int add = static_cast<int>(std::lround(err * t.w));
                        int tmp = static_cast<int>(targetRow[tx]) + add;
                        // clamp to short range
                        if (tmp < -32768) tmp = -32768;
                        if (tmp > 32767) tmp = 32767;
                        targetRow[tx] = static_cast<short>(tmp);
                    }
                }
            }

            // rotate the buffer
            loadInputRowToBuf(y + 3, rowIndex(y + 3));
        }
    }
}
