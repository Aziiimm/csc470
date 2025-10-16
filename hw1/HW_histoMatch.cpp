#include "IP.h"
#include <vector>
#include <cmath>
#include <cstring>
using namespace IP;

void histoMatchApprox(ImagePtr, ImagePtr, ImagePtr);

// helper functions for histogram matching

// read the histogram shape
static void readTargetHisto(ImagePtr targetHisto, double tgt[MXGRAY]) {
	for (int i = 0; i < MXGRAY; ++i) tgt[i] = 0.0;

	ChannelPtr<uchar> p;
	int type;
	if (!IP_getChannel(targetHisto, 0, p, type)) return;

	int total = targetHisto->width() * targetHisto->height();
	int cnt = (total < MXGRAY) ? total : MXGRAY;
	for (int i = 0; i < cnt; ++i) tgt[i] = (double)p[i];
}

// build input histogram for channel ch and return Nch
static int buildInputHistogram(ImagePtr I, int ch, int H[MXGRAY]) {
	std::memset(H, 0, MXGRAY * sizeof(int));
	ChannelPtr<uchar> p;
	int type;
	IP_getChannel(I, ch, p, type);

	int N = I->width() * I->height();
	for (int i = 0; i < N; ++i) ++H[p[i]];
	return N;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// HW_histoMatch:
//
// Apply histogram matching to I1. Output is in I2.
//
void HW_histoMatch(ImagePtr I1, ImagePtr targetHisto, bool approxAlg, ImagePtr I2) {
	if (approxAlg) {
		histoMatchApprox(I1, targetHisto, I2);
		return;
	}

	// exact histogram matching
	IP_copyImageHeader(I1, I2);

	// read target shape
	double tgtRaw[MXGRAY];
	readTargetHisto(targetHisto, tgtRaw);

	ChannelPtr<uchar> pIn, pOut;
	int type;

	for (int ch = 0; IP_getChannel(I1, ch, pIn, type); ch++) {
		IP_getChannel(I2, ch, pOut, type);

		const int W = I1->width();
		const int Hh = I1->height();
		const int total = W * Hh;

		// histo1
		int histo1[MXGRAY];
		buildInputHistogram(I1, ch, histo1);

		// scale target to total pixels
		int histo2[MXGRAY];
		// sum raw
		double sumRaw = 0.0;
		for (int i = 0; i < MXGRAY; ++i) sumRaw += tgtRaw[i];

		if (sumRaw <= 0.0) {
			// flatten
			int base = total / MXGRAY, rem = total % MXGRAY;
			for (int i = 0; i < MXGRAY; ++i) histo2[i] = base + (i < rem ? 1 : 0);
		}
		else {
			// scale
			double scale = (double)total / sumRaw;
			int partial = 0;
			for (int i = 0; i < MXGRAY; ++i) {
				histo2[i] = (int)std::floor(tgtRaw[i] * scale + 0.5); // round
				partial += histo2[i];
				if (partial > total) {
					// clamp this bin and zero the rest
					int overshoot = partial - total;
					histo2[i] -= overshoot;
					for (int j = i + 1; j < MXGRAY; ++j) histo2[j] = 0;
					partial = total;
					break;
				}
			}

			// if under, give the remainder to the last bin
			if (partial < total) histo2[MXGRAY - 1] += (total - partial);
		}

		// build left right intervals
		int left[MXGRAY], right[MXGRAY];
		int r = 0;
		long Hsum = 0;
		for (int i = 0; i < MXGRAY; ++i) {
			left[i] = r;
			Hsum += histo1[i];
			while (Hsum > histo2[r] && r < MXGRAY - 1) {
				Hsum -= histo2[r];
				++r;
			}
			right[i] = r;
		}


		// reuse histo1 for output bins
		std::memset(histo1, 0, MXGRAY * sizeof(int));

		IP_getChannel(I1, ch, pIn, type);
		IP_getChannel(I2, ch, pOut, type);

		// remap pixels
		for (int k = 0; k < total; ++k) {
			int in = pIn[k];
			int p = left[in];

			if (histo1[p] < histo2[p]) {
				pOut[k] = (uchar)p;
			}
			else {
				int nextp = p + 1;
				if (nextp > right[in]) nextp = right[in];
				p = left[in] = nextp;
				pOut[k] = (uchar)p;
			}
			++histo1[p];
		}
	}
}

void histoMatchApprox(ImagePtr I1, ImagePtr targetHisto, ImagePtr I2) {

	IP_copyImageHeader(I1, I2);

	// target histogram
	double tgtRaw[MXGRAY];
	readTargetHisto(targetHisto, tgtRaw);

	ChannelPtr<uchar> pIn, pOut;
	int type;

	for (int ch = 0; IP_getChannel(I1, ch, pIn, type); ch++) {
		IP_getChannel(I2, ch, pOut, type);

		// input histogram
		int Hin[MXGRAY];
		int N = buildInputHistogram(I1, ch, Hin);

		// scaled target counts
		//int T[MXGRAY];

		// scaling
		//double sumRaw = 0.0;
		//for (int i = 0; i < MXGRAY; ++i) sumRaw += tgtRaw[i];
		//long long acc = 0;
		//if (sumRaw <= 0.0) {
		//	int base = N / MXGRAY, rem = N % MXGRAY;
		//	for (int i = 0; i < MXGRAY; ++i) {
		//		T[i] = base + (i < rem ? 1 : 0);
		//	}
		//} else {
		//	for (int i = 0; i < MXGRAY - 1; ++i) {
		//		T[i] = (int)std::floor(tgtRaw[i] * ((double)N / sumRaw) + 0.5);
		//		acc += T[i];
		//	}
		//	T[MXGRAY - 1] = (int)N - int(acc);
		//	if (T[MXGRAY - 1] < 0) T[MXGRAY - 1] = 0;
		//}

		// scaled target counts
		int T[MXGRAY];
		std::memset(T, 0, sizeof(T));

		// compute scaling
		double sumRaw = 0.0;
		for (int i = 0; i < MXGRAY; ++i) sumRaw += tgtRaw[i];

		if (sumRaw <= 0.0) {
			int base = N / MXGRAY, rem = N % MXGRAY;
			for (int i = 0; i < MXGRAY; ++i) T[i] = base + (i < rem ? 1 : 0);
		}
		else {
			double scale = (double)N / sumRaw;

			int running = 0;
			int lastNonZero = -1;

			for (int i = 0; i < MXGRAY; ++i) {
				// round each bin
				int ti = (int)std::floor(tgtRaw[i] * scale + 0.5);
				if (ti < 0) ti = 0;

				T[i] = ti;
				if (ti > 0) lastNonZero = i;

				running += ti;
				if (running > N) {
					// zero the remaining
					int extra = running - N;
					T[i] -= extra;
					for (int k = i + 1; k < MXGRAY; ++k) T[k] = 0;
					running = N;
					break;
				}
			}

			if (running < N) {
				int idx = (lastNonZero >= 0) ? lastNonZero : (MXGRAY - 1);
				T[idx] += (N - running);
			}
		}


		// build cdfs
		double Cin[MXGRAY], Ctgt[MXGRAY];
		double run = 0.0, inv = (N > 0) ? 1.0 / N : 0.0;
		for (int i = 0; i < MXGRAY; ++i) { run += (double)Hin[i]; Cin[i] = run * inv; }
		run = 0.0;
		for (int i = 0; i < MXGRAY; ++i) { run += (double)T[i]; Ctgt[i] = (N > 0) ? run / (double)N : 0.0; }

		// LUT through CDF matching
		int LUT[MXGRAY];
		int j = 0;
		for (int i = 0; i < MXGRAY; ++i) {
			double s = Cin[i];
			while (j < MXGRAY - 1 && Ctgt[j] < s) ++j;
			if (j == 0) LUT[i] = 0;
			else {
				double d1 = std::fabs(Ctgt[j] - s);
				double d0 = std::fabs(Ctgt[j - 1] - s);
				LUT[i] = (d0 <= d1) ? (j - 1) : j;
			}
		}
		// apply LUT
		IP_getChannel(I1, ch, pIn, type);
		IP_getChannel(I2, ch, pOut, type);
		for (int k = 0; k < N; ++k) pOut[k] = (uchar)LUT[pIn[k]];
	}
}
