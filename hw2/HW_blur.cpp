#include "IP.h"
using namespace IP;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_blur:
//
// Blur image I1 with a box filter (unweighted averaging).
// The filter has width filterW and height filterH.
// We force the kernel dimensions to be odd.
// Output is in I2.
//

int clampValue(int value, int minValue, int maxValue) {
	if (value < minValue) return minValue;
	if (value > maxValue) return maxValue;
	return value;
}

void HW_blur(ImagePtr I1, int filterW, int filterH, ImagePtr I2) {

	// make sure filter dimensions are odd and positive
	if (filterW < 1) filterW = 1;
	if (filterH < 1) filterH = 1;
	if ((filterW & 1) == 0) filterW++;
	if ((filterH & 1) == 0) filterH++;

	const int imageWidth = I1->width();
	const int imageHeight = I1->height();
	const int numChannels = I1->maxDepth();

	// prepare output image and temp image
	IP_copyImageHeader(I1, I2);
	ImagePtr tempImage;
	IP_copyImageHeader(I1, tempImage);

	const int halfWidth = filterW / 2;
	const int halfHeight = filterH / 2;

	// horizontal pass
	for (int channel = 0; channel < numChannels; ++channel) {
		ChannelPtr<uchar> src, temp;
		int type;
		IP_getChannel(I1, channel, src, type);
		IP_getChannel(tempImage, channel, temp, type);

		for (int row = 0; row < imageHeight; ++row) {
			for (int col = 0; col < imageWidth; ++col) {
				int sum = 0;

				// accumulate values within the horizontal filter window
				for (int offsetX = -halfWidth; offsetX <= halfWidth; ++offsetX) {
					int sampleX = clampValue(col + offsetX, 0, imageWidth - 1);
					sum += src[row * imageWidth + sampleX];
				}

				temp[row * imageWidth + col] = static_cast<uchar>(sum / filterW);
			}
		}
	}

	// vertical pass
	for (int channel = 0; channel < numChannels; ++channel) {
		ChannelPtr<uchar> temp, dst;
		int type;
		IP_getChannel(tempImage, channel, temp, type);
		IP_getChannel(I2, channel, dst, type);

		for (int row = 0; row < imageHeight; ++row) {
			for (int col = 0; col < imageWidth; ++col) {
				int sum = 0;

				// accumulate values within the vertical filter window
				for (int offsetY = -halfHeight; offsetY <= halfHeight; ++offsetY) {
					int sampleY = clampValue(row + offsetY, 0, imageHeight - 1);
					sum += temp[sampleY * imageWidth + col];
				}

				dst[row * imageWidth + col] = static_cast<uchar>(sum / filterH);
			}
		}
	}
}