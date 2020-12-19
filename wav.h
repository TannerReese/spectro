#ifndef _WAV_H
#define _WAV_H

#include <stdio.h>

struct wav_s;
typedef struct wav_s *wav_t;

typedef enum{
	WAV_OK = 0,
	WAV_NOT_RIFF,
	WAV_NOT_WAVE,
	WAV_NO_DATA,
	WAV_NO_FORMAT
} wav_err;

// Extract data from file stream
wav_t read_wav(FILE *fl, wav_err *err);
// Deallocate memory from wav
void free_wav(wav_t wv);

// Return sampling frequency of wav file
unsigned int wav_sample_freq(wav_t wv);
// Get number of samples in wav file
unsigned int wav_sample_count(wav_t wv);
// Get duration of wav file
double wav_duration(wav_t wv);

// Convert time to sample index
unsigned int wav_attime(wav_t wv, double time);
// Convert sample index to time
double wav_atindex(wav_t wv, unsigned int idx);
// Does not check for wav format returns contents unchanged
void *wav_sampat(wav_t wv, int sampidx, int chnl);
// Returns value of channel `chnl` at index `sampidx` normalized to [-1, 1)
double wav_fsampat(wav_t wv, int sampidx, int chnl);

#endif
