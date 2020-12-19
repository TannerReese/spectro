#ifndef _FOURIER_H
#define _FOURIER_H

struct freqtbl_s;
typedef struct freqtbl_s *freqtbl_t;

// Allocate the memory for and pre-calculate the wave data
freqtbl_t make_freqtbl(double sample_freq, int samples_perblk, int cycles_perblk);
// Generate Frequency Table with a frequency close to freq
freqtbl_t gen_freqtbl(double freq, double sample_freq, double error);
void free_freqtbl(freqtbl_t tbl);

// Reset running sum and start taking new samples
// Allocate space for new window
// If samples_perwin <= 0 then start taking new samples with an unbounded window
void init_freqtbl(freqtbl_t tbl, int samples_perwin);
// Initializes frequency table and creates window whose duration does not exceed `maxdur`
void start_freqtbl(freqtbl_t tbl, double maxdur);
// Reset running sums, but keep window
void clear_freqtbl(freqtbl_t tbl);

// Returns current amplitude for frequency or a negative number if no amplitude is available yet
double freqtbl_get(freqtbl_t tbl);
// Moves window forward by one sample
void freqtbl_push(freqtbl_t tbl, double sample);

// Get the number of samples per block
unsigned int freqtbl_samps_perblk(freqtbl_t tbl);
// Get the frequency for this table
double freqtbl_freq(freqtbl_t tbl);



struct spectrum_s;
typedef struct spectrum_s *spectrum_t;

// Generate spectrum over frequency range [low, high] with `count` number of frequency tables
// Initializes frequency tables with windows of duration no longer than `maxdur`
spectrum_t gen_spectrum(double sample_freq, double low, double high, int count, double maxdur);
// Deallocate spectrum and associated frequency tables
void free_spectrum(spectrum_t spec);
// Clear the running sums of every table
void clear_spectrum(spectrum_t spec);

// Get number of frequency tables in spectrum
unsigned int spec_freqcount(spectrum_t spec);
// Get frequency for `i`th frequency table of spectrum
double spec_freq(spectrum_t spec, unsigned int i);

// Push sample to each frequency table of spectrum
void spec_push(spectrum_t spec, double sample);
// Returns amplitudes for `i`th frequency table in spectrum
double spec_get(spectrum_t spec, unsigned int i);

#endif
