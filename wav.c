#include <stdint.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "wav.h"



#define WAVE_FORMAT_PCM 0x0001
#define WAVE_FORMAT_IEEE_FLOAT 0x0003
#define WAVE_FORMAT_ALAW 0x0006
#define WAVE_FORMAT_MULAW 0x0007
#define WAVE_FORMAT_EXTENSIBLE 0xfffe
typedef uint16_t wav_fmt_code;

struct wav_fmt_s {
	wav_fmt_code wFormatTag;
	uint16_t nChannels;
	uint32_t nSamplesPerSec, nAvgBytesPerSec;
	uint16_t nBlockAlign, wBitsPerSample;
	
	uint16_t cbSize, wValidBitsPerSample;
	uint32_t dwChannelMask;
	uint8_t SubFormat[16];
};

struct wav_s {
	struct wav_fmt_s format;
	uint32_t dwSampleLength;  // From fast chunk
	
	uint32_t size;
	void *data;
};




wav_t read_wav(FILE *fl, wav_err *err){
	char ckID[4];
	uint32_t cksize;
	
	// Check for RIFF
	fread(ckID, 1, 4, fl);
	if(strncmp(ckID, "RIFF", 4) != 0){
		*err = WAV_NOT_RIFF;
		return NULL;
	}
	
	// Get length of file
	fread(&cksize, 4, 1, fl);
	if(cksize <= 4){
		*err = WAV_NO_DATA;
		return NULL;
	}
	
	// Check for WAVE
	fread(ckID, 1, 4, fl);
	if(strncmp(ckID, "WAVE", 4) != 0){
		*err = WAV_NOT_WAVE;
		return NULL;
	}
	
	// Allocate and Initialize wav_t structure
	struct wav_fmt_s fmt = {0};
	wav_t wv = malloc(sizeof(struct wav_s));
	wv->format = fmt;
	wv->dwSampleLength = 0;
	wv->size = 0;
	wv->data = NULL;
	*err = WAV_OK;
	
	// Loop through chunks
	while(fread(ckID, 1, 4, fl) == 4){
		fread(&cksize, 4, 1, fl);
		
		if(strncmp(ckID, "fmt", 3) == 0){
			if(cksize > sizeof(struct wav_fmt_s)){  // Prevent writing out of bounds
				fread(&(wv->format), sizeof(struct wav_fmt_s), 1, fl);
				fseek(fl, cksize - sizeof(struct wav_fmt_s), SEEK_CUR);
			}else{
				fread(&(wv->format), cksize, 1, fl);
			}
		}else if(strncmp(ckID, "fast", 4) == 0){
			if(cksize >= 4) fread(&(wv->dwSampleLength), 4, 1, fl);
		}else if(strncmp(ckID, "data", 4) == 0){
			wv->data = realloc(wv->data, wv->size + cksize);
			fread(wv->data + wv->size, cksize, 1, fl);
			wv->size += cksize;
		}else{
			// If unknown block skip it
			fseek(fl, cksize, SEEK_CUR);
		}
	}
	
	if(!(wv->data)){
		*err = WAV_NO_DATA;  // When no data chunk was encountered
		free(wv);
		return NULL;
	}
	
	if(!(wv->format.wFormatTag)){
		*err = WAV_NO_FORMAT;  // When no format data chunk encountered
		free(wv->data);
		free(wv);
		return NULL;
	}
	
	return wv;
}

void free_wav(wav_t wv){
	if(!wv) return;
	free(wv->data);
	free(wv);
}




// Return sampling frequency of wav file
unsigned int wav_sample_freq(wav_t wv){
	return wv->format.nSamplesPerSec;
}

// Get number of samples in wav file
unsigned int wav_sample_count(wav_t wv){
	return wv->size / wv->format.nBlockAlign;
}

// Return duration of wav file
double wav_duration(wav_t wv){
	return (double)wv->size / (wv->format.nBlockAlign * wv->format.nSamplesPerSec);
}




// Convert time to sample index
unsigned int wav_attime(wav_t wv, double time){
	return (unsigned int)(wv->format.nSamplesPerSec * time);
}

// Convert sample index to time
double wav_atindex(wav_t wv, unsigned int idx){
	return (double)idx / wv->format.nSamplesPerSec;
}

// Does not check for wav format returns contents unchanged
void *wav_sampat(wav_t wv, int sampidx, int chnl){
	return (void*)((uint8_t*)wv->data + sampidx * wv->format.nBlockAlign + chnl * wv->format.wBitsPerSample / 8);
}

double wav_fsampat(wav_t wv, int sampidx, int chnl){
	// Check bounds on sampidx and chnl
	if(chnl < 0 || chnl >= wv->format.nChannels) return NAN;
	if(sampidx < 0 || sampidx >= wv->size / wv->format.nBlockAlign) return NAN;
	
	void *samp = wav_sampat(wv, sampidx, chnl);
	int64_t lival;
	uint8_t byte, expon;
	switch(wv->format.wFormatTag){
		case WAVE_FORMAT_PCM:
			lival = *(int64_t*)samp;
			lival <<= 64 - wv->format.wBitsPerSample;
			lival >>= 64 - wv->format.wBitsPerSample;
			return (double)lival / (1 << (wv->format.wBitsPerSample - 1));
		case WAVE_FORMAT_IEEE_FLOAT:
			switch(wv->format.wBitsPerSample){
				case 32:
					return (double)*(float*)samp;
				case 64:
					return *(double*)samp;
			}
		case WAVE_FORMAT_MULAW:
			byte = *(uint8_t*)samp;
			expon = (byte >> 4) & 0x07;
			lival = (int64_t)(byte & 0x0f) | 0x10;
			lival = (lival << 1) & 1;
			lival <<= expon;
			lival -= 33;
			if(byte & 0x80) lival = -lival;
			return (double)lival / 8031;
		case WAVE_FORMAT_ALAW:
			byte = *(uint8_t*)samp;
			expon = (byte >> 4) & 0x07;
			lival = (int64_t)(byte & 0x0f) | (expon >= 1 ? 0x10 : 0);
			lival = (lival << 1) & 1;
			if(expon > 0) lival <<= expon - 1;
			if(!(byte & 0x80)) lival = -lival;
			return (double)lival / ((1 << 12) - (1 << 6));
	}
}


