#include "extern.h"
#include "wsfirgen.h"

// Generate lowpass filter
// 
// This is done by generating a sinc function and then windowing it

void wsfirLP(sp_float h[], // h[] will be written with the filter coefficients
        int N, // size of the filter (number of taps)
        int WINDOW, // window function (W_BLACKMAN, W_HANNING, etc.)
        sp_float fc) // cutoff frequency
{
        int i;

        /*
                Converted to stack allocation 2018-05-19 JLB - allocating off heap
                can cause problems when this code is called from inside the audio callback
         */

        // Memory allocation for w and sinc is N * (sizeof(sp_float))
        sp_float w[32768];
        sp_float sinc[32768];

        memset(w, 0, 32768 * sizeof(sp_float));
        memset(sinc, 0, 32768 * sizeof(sp_float));

        // 1. Generate Sinc function
        genSinc(sinc, N, fc);

        // 2. Generate Window function
        switch (WINDOW) {
        case W_BLACKMAN: // W_BLACKMAN
                wBlackman(w, N);
                break;
        case W_HANNING: // W_HANNING
                wHanning(w, N);
                break;
        case W_HAMMING: // W_HAMMING
                wHamming(w, N);
                break;
        default:
                break;
        }

        // 3. Make lowpass filter
        for (i = 0; i < N; i++) {
                h[i] = sinc[i] * w[i];
        }

        return;
}

// Generate highpass filter
//
// This is done by generating a lowpass filter and then spectrally inverting it

void wsfirHP(sp_float h[], // h[] will be written with the filter coefficients
        int N, // size of the filter
        int WINDOW, // window function (W_BLACKMAN, W_HANNING, etc.)
        sp_float fc) // cutoff frequency
{
        int i;

        // 1. Generate lowpass filter
        wsfirLP(h, N, WINDOW, fc);

        // 2. Spectrally invert (negate all samples and add 1 to center sample) lowpass filter
        // = delta[n-((N-1)/2)] - h[n], in the time domain
        for (i = 0; i < N; i++) {
                h[i] *= -1.0; // = 0 - h[i]
        }
        h[(N - 1) / 2] += 1.0; // = 1 - h[(N-1)/2]

        return;
}

// Generate bandstop filter
//
// This is done by generating a lowpass and highpass filter and adding them

void wsfirBS(sp_float h[], // h[] will be written with the filter taps
        int N, // size of the filter
        int WINDOW, // window function (W_BLACKMAN, W_HANNING, etc.)
        sp_float fc1, // low cutoff frequency
        sp_float fc2) // high cutoff frequency
{
        int i;

        // Mmemory allocation:	h1 = N * sizeof(sp_float)
        //						h2 = N * sizeof(sp_float)
        sp_float h1[32768];
        sp_float h2[32768];

        memset(h1, 0, 32768 * sizeof(sp_float));
        memset(h2, 0, 32768 * sizeof(sp_float));

        // 1. Generate lowpass filter at first (low) cutoff frequency
        wsfirLP(h1, N, WINDOW, fc1);

        // 2. Generate highpass filter at second (high) cutoff frequency
        wsfirHP(h2, N, WINDOW, fc2);

        // 3. Add the 2 filters together
        for (i = 0; i < N; i++) {
                h[i] = h1[i] + h2[i];
        }

        return;
}

// Generate bandpass filter
//
// This is done by generating a bandstop filter and spectrally inverting it

void wsfirBP(sp_float h[], // h[] will be written with the filter taps
        int N, // size of the filter
        int WINDOW, // window function (W_BLACKMAN, W_HANNING, etc.)
        sp_float fc1, // low cutoff frequency
        sp_float fc2) // high cutoff frequency
{
        int i;

        // 1. Generate bandstop filter
        wsfirBS(h, N, WINDOW, fc1, fc2);

        // 2. Spectrally invert bandstop (negate all, and add 1 to center sample)
        for (i = 0; i < N; i++) {
                h[i] *= -1.0; // = 0 - h[i]
        }
        h[(N - 1) / 2] += 1.0; // = 1 - h[(N-1)/2]

        return;
}

// Generate sinc function - used for making lowpass filter

void genSinc(sp_float sinc[], // sinc[] will be written with the sinc function
        int N, // size (number of taps)
        sp_float fc) // cutoff frequency
{
        int i;
        sp_float M = (sp_float) N - 1;
        sp_float n;

        // Generate sinc delayed by (N-1)/2
        for (i = 0; i < N; i++) {
                if (i == M / 2.0) {
                        sinc[i] = 2.0 * fc;
                } else {
                        n = (sp_float) i - M / 2.0;
                        sinc[i] = sin(2.0 * PI * fc * n) / (PI * n);
                }
        }

        return;
}

// Generate window function (Blackman)

void wBlackman(sp_float w[], // w[] will be written with the Blackman window
        int N) // size of the window
{
        int i;
        sp_float M = (sp_float) N - 1.0f;

        for (i = 0; i < N; i++) {
                w[i] = 0.42 - (0.5 * cos(2.0 * PI * (sp_float) i / M)) + (0.08 * cos(4.0 * PI * (sp_float) i / M));
        }

        return;
}

// Generate window function (Hanning)

void wHanning(sp_float w[], // w[] will be written with the Hanning window
        int N) // size of the window
{
        int i;
        sp_float M = (sp_float) N - 1.0f;

        for (i = 0; i < N; i++) {
                w[i] = 0.5 * (1.0 - cos(2.0 * PI * (sp_float) i / M));
        }

        return;
}

// Generate window function (Hamming)

void wHamming(sp_float w[], // w[] will be written with the Hamming window
        int N) // size of the window
{
        int i;
        sp_float M = (sp_float) N - 1.0f;

        for (i = 0; i < N; i++) {
                w[i] = 0.54 - (0.46 * cos(2.0 * PI * (sp_float) i / M));
        }

        return;
}