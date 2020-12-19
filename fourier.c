#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "fourier.h"

#define MATH_PI 3.141592653589793


struct freqtbl_s {
	double frequency;
	
	// Read-Only Variables for storing wave data
	int cycles, samples;  // Number of cycles and samples in block (wave data)
	// Arrays of doubles representing samples of sin and cos within block
	double *sine, *cosine;
	
	// Variables used during calculation of running sums
	int blkidx;  // Location within block (wave data)
	int winwidth, winidx;  // Size of window and current location within window
	// NOTE: winwidth < 0 used to indicate infinite window
	int samps_in_win;  // Number of samples in window currently
	double *window;  // Store prior samples
	
	// Keep track of running sums and norms
	double sine_sum, cosine_sum;
	double sine_norm, cosine_norm;
};


// Initialize memory of frequency table pointed to by `tbl`
static freqtbl_t fill_freqtbl(freqtbl_t tbl, double sample_freq, int samples_perblk, int cycles_perblk){
	// Frequencies greater than sample_freq are unresolvable
	if(samples_perblk < cycles_perblk) return NULL;
	
	tbl->frequency = sample_freq * cycles_perblk / samples_perblk;
	
	tbl->cycles = cycles_perblk;
	tbl->samples = samples_perblk;
	
	// Generate waves
	tbl->sine = malloc(sizeof(double) * samples_perblk);
	tbl->cosine = malloc(sizeof(double) * samples_perblk);
	double radians_persamp = 2 * MATH_PI * cycles_perblk / samples_perblk;
	for(int i = 0; i < samples_perblk; i++){
		tbl->sine[i] = sin(i * radians_persamp);
		tbl->cosine[i] = cos(i * radians_persamp);
	}
	
	// Initialize variables that will be used for accumulation
	tbl->blkidx = 0;
	tbl->winwidth = 0;
	tbl->winidx = 0;
	tbl->samps_in_win = 0;
	tbl->window = NULL;
	
	tbl->sine_sum = 0;
	tbl->sine_norm = 0;
	tbl->cosine_sum = 0;
	tbl->cosine_norm = 0;
	
	return tbl;
}

freqtbl_t make_freqtbl(double sample_freq, int samples_perblk, int cycles_perblk){
	// Frequencies greater than sample_freq are unresolvable
	if(samples_perblk < cycles_perblk) return NULL;
	
	freqtbl_t tbl = malloc(sizeof(struct freqtbl_s));
	return fill_freqtbl(tbl, sample_freq, samples_perblk, cycles_perblk);
}

static void rational_approx(double target, double error, int *num, int *den){
	double work = target;
	int intpart;
	
	// Store last and current denominator and numerator
	/* Can be thought of as matrix
	 * [n  ln] = M
	 * [d  ld]
	 * Starting at the identity matrix
	 */
	int n = 1, ln = 0;
	int d = 0, ld = 1;
	
	int nextN, nextD;
	do{
		intpart = (int)work;
		
		/* Postmultiply M by corresponding matrix
		 * M * [intpart  1] -> M
		 *     [   1     0]
		 */
		nextN = intpart * n + ln;
		nextD = intpart * d + ld;
		ln = n;
		ld = d;
		n = nextN;
		d = nextD;
		
		if(work == intpart) break;
		work = 1 / (work - intpart);
	}while(fabs(target - (double)n / d) >= error);
	
	*num = n;
	*den = d;
}

freqtbl_t gen_freqtbl(double freq, double sample_freq, double error){
	int num, den;
	rational_approx(sample_freq / freq, error, &num, &den);
	return make_freqtbl(sample_freq, num, den);
}

void free_freqtbl(freqtbl_t tbl){
	if(tbl->window) free(tbl->window);
	
	free(tbl->cosine);
	free(tbl->sine);
	free(tbl);
}



void init_freqtbl(freqtbl_t tbl, int samples_perwin){
	clear_freqtbl(tbl);  // Reset all values
	
	tbl->winwidth = samples_perwin <= 0 ? -1 : samples_perwin;
	
	if(tbl->window) free(tbl->window);
	if(samples_perwin <= 0){
		tbl->window = NULL;
	}else{
		tbl->window = malloc(sizeof(double) * tbl->winwidth);
	}
}

void start_freqtbl(freqtbl_t tbl, double maxdur){
	unsigned int maxsamps = (unsigned int)(maxdur * tbl->frequency * tbl->samples / tbl->cycles);
	unsigned int cycs = maxsamps / tbl->samples;
	if(cycs < 5) cycs = 5;
	init_freqtbl(tbl, cycs * tbl->samples);
}

void clear_freqtbl(freqtbl_t tbl){
	tbl->blkidx = 0;
	tbl->winidx = 0;
	tbl->samps_in_win = 0;
	
	tbl->sine_sum = 0;
	tbl->sine_norm = 0;
	tbl->cosine_sum = 0;
	tbl->cosine_norm = 0;
}



double freqtbl_get(freqtbl_t tbl){
	if(tbl->samps_in_win >= tbl->winwidth && tbl->sine_norm > 0 && tbl->cosine_norm > 0){
		return hypot(tbl->sine_sum / tbl->sine_norm, tbl->cosine_sum / tbl->cosine_norm);
	}else{
		return -1;
	}
}

void freqtbl_push(freqtbl_t tbl, double sample){
	double s, c;
	
	// Include new sample in the running sums
	s = tbl->sine[tbl->blkidx];
	tbl->sine_sum += s * sample;
	tbl->sine_norm += s * s;
	c = tbl->cosine[tbl->blkidx];
	tbl->cosine_sum += c * sample;
	tbl->cosine_norm += c * c;
	
	// Remove samples that are no longer in the scope of the window from the sine and cosine sums
	// Only occurs in finite mode
	if(tbl->samps_in_win >= tbl->winwidth && tbl->winwidth > 0){
		// Get position of the oldest sample in the window, within the block
		int oldidx = (tbl->blkidx - tbl->winwidth) % tbl->samples;
		if(oldidx < 0) oldidx += tbl->samples;
		
		// Remove oldest sample from the running sums
		s = tbl->sine[oldidx];
		tbl->sine_sum -= s * tbl->window[tbl->winidx];
		tbl->sine_norm -= s * s;
		c = tbl->cosine[oldidx];
		tbl->cosine_sum -= c * tbl->window[tbl->winidx];
		tbl->cosine_norm -= c * c;
	}else{
		tbl->samps_in_win++;
	}
	
	if(tbl->winwidth > 0){
		// Store sample into window if window being used
		tbl->window[tbl->winidx] = sample;
		tbl->winidx++;
		tbl->winidx %= tbl->winwidth;
	}
	
	tbl->blkidx++;
	tbl->blkidx %= tbl->samples;
}



double freqtbl_freq(freqtbl_t tbl){
	return tbl->frequency;
}





struct spectrum_s {
	double lowest, highest;
	double ratio;
	
	struct freqtbl_s *begin, *end;  // Beginning and End of Array of frequency tables
};


spectrum_t gen_spectrum(double sample_freq, double low, double high, int count, double maxdur){
	// Frequencies must be greater than zero
	if(low <= 0 || high <= 0) return NULL;
	// Lower bound of frequency must be higher than upper bound
	if(low > high) return NULL;
	
	// There must be at least two tables to cover range
	if(count < 2) return NULL;
	
	spectrum_t spec = malloc(sizeof(struct spectrum_s));
	spec->lowest = low;
	spec->highest = high;
	
	count = abs(count);
	spec->ratio = pow(high / low, 1 / (double)(count - 1));
	
	// Allocate memory for array of frequency tables
	spec->begin = malloc(sizeof(struct freqtbl_s) * count);
	spec->end = spec->begin + count - 1;
	
	double f = low;
	int perblk;  // Samples per Block
	for(int i = 0; i < count; i++){
		perblk = (int)(sample_freq / f);
		fill_freqtbl(spec->begin + i, sample_freq, perblk, 1);
		start_freqtbl(spec->begin + i, maxdur);
		
		f *= spec->ratio;
	}
	
	return spec;
}

void free_spectrum(spectrum_t spec){
	for(freqtbl_t tbl = spec->begin; tbl <= spec->end; tbl++){
		if(tbl->window) free(tbl->window);
		free(tbl->sine);
		free(tbl->cosine);
	}
	free(spec);
}

void clear_spectrum(spectrum_t spec){
	for(freqtbl_t tbl = spec->begin; tbl <= spec->end; tbl++){
		clear_freqtbl(tbl);
	}
}



// Get number of frequency tables in spectrum
unsigned int spec_freqcount(spectrum_t spec){
	return (unsigned int)(spec->end - spec->begin) + 1;
}

// Get frequency for `i`th frequency table of spectrum
double spec_freq(spectrum_t spec, unsigned int i){
	return spec->begin[i].frequency;
}



// Push sample to each frequency table of spectrum
void spec_push(spectrum_t spec, double sample){
	for(freqtbl_t tbl = spec->begin; tbl <= spec->end; tbl++){
		freqtbl_push(tbl, sample);
	}
}

// Returns amplitudes for `i`th frequency table in spectrum
double spec_get(spectrum_t spec, unsigned int i){
	return freqtbl_get(spec->begin + i);
}

