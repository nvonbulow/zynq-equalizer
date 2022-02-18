#include <stdio.h>
#include <math.h>
#include <complex.h>
#include <string.h>
#include <stdlib.h>
#include "fft.h"

#define FFT_DIRECTION_FORWARD -1
#define FFT_DIRECTION_BACKWARD 1

float PI = M_PI;
 
void _fft(float complex in[], float complex out[], int n, int step, int direction)
{
	int i;
    if (step < n) {
        _fft(out, in, n, step * 2, direction);
        _fft(out + step, in + step, n, step * 2, direction);
 
        for (i = 0; i < n; i += 2 * step) {
            float complex t = cexpf(direction*I * PI * i / n) * out[i + step];

            in[i / 2]     = out[i] + t;
            in[(i + n)/2] = out[i] - t;
        }
    }
}
 
void fft(float complex input[], float complex output[], int n)
{
    _fft(input, output, n, 1, FFT_DIRECTION_FORWARD);
}

void ifft(float complex input[], float complex output[], int n)
{
	int i;

    _fft(input, output, n, 1, FFT_DIRECTION_BACKWARD);

    for (i = 0; i < n; i++) input[i] /= n;
}
