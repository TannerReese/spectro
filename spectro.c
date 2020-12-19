#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <math.h>
#include <argp.h>

#include "fourier.h"
#include "wav.h"



#define AUDIO_FILE_LENGTH 64
char audio_file[AUDIO_FILE_LENGTH] = "\0";  // Path to audio file to display
int channel = 0;  // Channel of audio file to display
double start_tm = 0, end_tm = -0.00001; // Start and End times

unsigned int freqs_len = 0, freqs_cap = 0;
double *freqs = NULL;

double low_frq = 10, upp_frq = 10000;  // Lower and Upper Bounds of Frequency Range
int frq_count = -1;  // Number of Frequency Tables in Spectrum
float lines_per_sec = 4;  // Number of lines of spectrogram to print every second
float scaling = 100;  // Amount by which to scale resulting amplitudes

struct argp_option options[] = {
	{"freq", 'f', "FREQ", 0, "Track another frequency precisely", 0},
	
	{"count", 'n', "NUMBER", 0, "Number of Frequencies to be track in Spectrum. Defaults to fit screen", 1},
	{"range", 'a', "[LOW_FREQ][:HIGH_FREQ]", 0, "Lower and Upper Bounding Frequency of Spectrum (default: 10Hz : 10,000Hz)", 1},
	
	{"channel", 'c', "CHANNEL", 0, "Channel of audio file to display. Defaults to first", 1},
	{"time", 't', "[START][:END]", 0, "Start and End Times to display spectrogram for. Defaults to entire file", 1},
	
	{"rate", 'r', "LINES_PER_SEC", 0, "Rate at which spectrogram lines should be printed (default: 4 lines / sec)", 3},
	{"scale", 's', "SCALING", 0, "Factor by which to scale resulting amplitude values [1] (default: 100)", 3},
	{0}
};

error_t parse_opt(int key, char *arg, struct argp_state *state){
	double frq;
	int fst, snd;
	switch(key){
		case ARGP_KEY_ARG:
			strncpy(audio_file, arg, AUDIO_FILE_LENGTH);
		break;
		case ARGP_KEY_END:
			if(!*audio_file){  // When no audio file is given
				printf("Audio source must be given to analyze\n");
				argp_usage(state);
			}
		break;
		
		case 'f':
			if(sscanf(arg, " %lf", &frq) < 1){
				printf("Frequency must be floating point: \"%s\"\n", arg);
				argp_usage(state);
			}else{
				// Add frequency to array
				if(freqs_len >= freqs_cap){
					if(freqs_cap == 0) freqs_cap = 1;
					else freqs_cap *= 2;
					freqs = realloc(freqs, freqs_cap);
				}
				
				freqs[freqs_len++] = frq;
			}
		break;
		
		case 'n':
			if(sscanf(arg, " %i", &frq_count) < 1){
				printf("Invalid input for number of frequencies, must be integer: \"%s\"\n", arg);
				argp_usage(state);
			}
		break;
		case 'a':
			fst = sscanf(arg, "%lf", &low_frq) < 1;
			snd = sscanf(arg, ":%lf", &upp_frq) < 1 && sscanf(arg, "%*[^:]:%lf", &upp_frq) < 1;
			if(fst && snd){
				printf("Invalid frequency range: \"%s\"\n", arg);
				argp_usage(state);
			}
		break;
		
		case 'c':
			if(sscanf(arg, " %i", &channel) < 1){
				printf("Invalid channel, must be integer: \"%s\"\n", arg);
				argp_usage(state);
			}
		break;
		case 't':
			fst = sscanf(arg, "%lf", &start_tm) < 1;
			snd = sscanf(arg, ":%lf", &end_tm) < 1 && sscanf(arg, "%*[^:]:%lf", &end_tm) < 1;
			if(fst && snd){
				printf("Invalid time interval: \"%s\"\n", arg);
				argp_usage(state);
			}
		break;
		
		case 'r':
			if(sscanf(arg, " %f", &lines_per_sec) < 1){
				printf("Invalid number of lines, must be float: \"%s\"\n", arg);
				argp_usage(state);
			}
		break;
		
		case 's':
			if(sscanf(arg, " %f", &scaling) < 1){
				printf("Invalid scaling, must be float: \"%s\"\n", arg);
				argp_usage(state);
			}
		break;
		
		default:
			return ARGP_ERR_UNKNOWN;
	}
	return 0;
}

struct argp argp = {options, parse_opt,
	/* USAGE */ "[-f FREQ [-f FREQ ...]] FILE",
	/* Documentation */
	"Display Spectrogram for a given audio file\v"
	"[1]: Note that the scaling factor is also applied to the calculated amplitude of each of the additional frequencies (those indicated with -f)\n"
};





// Convert `scl` into colored character and print
// `scl` should be in range [0, 1)
void print_degree(double scl){
	#define CHAR_COUNT 7
	static char chrs[CHAR_COUNT] = " `'\"*%#";
	#define COLOR_COUNT 4
	static char *colors[COLOR_COUNT] = {
		"\033[35;40m",   // Magenta on Black
		"\033[91;45m",   // Red on Magenta
		"\033[93;101m",  // Yellow on Red
		"\033[37;103m"   // White on Yellow
	};
	
	int val = (int)(scl * CHAR_COUNT * COLOR_COUNT);
	if(val < 0) val = 0;
	if(val >= CHAR_COUNT * COLOR_COUNT) val = CHAR_COUNT * COLOR_COUNT - 1;
	
	printf("%s%c\033[0m", colors[val / CHAR_COUNT], chrs[val % CHAR_COUNT]);
}


int main(int argc, char *argv[], char *envp[]){
	argp_parse(&argp, argc, argv, 0, 0, NULL);
	
	// Fit spectrum size to screen
	if(frq_count < 0){
		struct winsize w;
		ioctl(fileno(stdout), TIOCGWINSZ, &w);
		frq_count = w.ws_col;
		// Compensate for other things which are displayed
		frq_count -= 11 + freqs_len * 9 + 1;
	}
	
	FILE *fl = fopen(audio_file, "rb");
	wav_err err;
	wav_t wv = read_wav(fl, &err);
	fclose(fl);
	
	switch(err){
		case WAV_NOT_RIFF:
		case WAV_NOT_WAVE:
			printf("Incorrect format, not WAV file\n");
			free_wav(wv);
			exit(1);
		break;
		case WAV_NO_DATA:
			printf("No data found in WAV file\n");
			free_wav(wv);
			exit(1);
		break;
		case WAV_NO_FORMAT:
			printf("No format chunk found in WAV file\n");
			free_wav(wv);
			exit(1);
		break;
	}
	
	// Check that channel is valid
	if(channel < 0){
		printf("Channel must be positive: \"%i\"\n", channel);
		free_wav(wv);
		exit(1);
	}else if(channel >= wav_channels(wv)){
		printf("Selected channel index, \"%i\", must be less than number of channels, \"%u\"\n", channel, wav_channels(wv));
		free_wav(wv);
		exit(1);
	}
	
	// Calculate what start_tm and end_tm are
	double duration = wav_duration(wv);
	if(start_tm < 0) start_tm += duration;
	if(end_tm < 0) end_tm += duration;
	
	// Print file stats
	unsigned int sampfrq = wav_sample_freq(wv);
	printf("Sampling Frequency: %uHz\t\tDuration: %.4lfs\t\tChannels: %u\n", sampfrq, duration, wav_channels(wv));
	
	int i, j;  // Indices for looping
	
	
	// Initialize any extra frequency tables requested
	freqtbl_t freq_tbls[freqs_len];
	for(i = 0; i < freqs_len; i++){
		freq_tbls[i] = gen_freqtbl(freqs[i], wav_sample_freq(wv), 0.1);
		start_freqtbl(freq_tbls[i], 1 / lines_per_sec);
	}
	free(freqs);
	
	// Generate spectrum over specified range
	spectrum_t spec = gen_spectrum(wav_sample_freq(wv), low_frq, upp_frq, frq_count, 1 / lines_per_sec);
	
	
	// Print top boarder
	printf("+---------+");
	for(i = 0; i < freqs_len; i++) printf("--------+");
	for(i = 0; i < frq_count; i++) putchar('-');
	putchar('+');
	
	// Print Headers
	printf("\n|  Time   |");
	for(i = 0; i < freqs_len; i++) printf(" %6.1lf |", freqtbl_freq(freq_tbls[i]));
	printf(" %*.1lf%*.1lf |", 1 - frq_count / 2, low_frq, frq_count - frq_count / 2 - 1, upp_frq);
	
	// Print lower boarder of headers
	printf("\n+---------+");
	for(i = 0; i < freqs_len; i++) printf("--------+");
	for(i = 0; i < frq_count; i++) putchar('-');
	putchar('+');
	
	
	unsigned int idx = wav_attime(wv, start_tm), step = (unsigned int)(sampfrq / lines_per_sec);
	unsigned int max_idx = wav_attime(wv, end_tm);
	double samp, ampl;  // Store sample of wav data and calculated amplitudes
	do{
		printf("\n| %7.3f |", wav_atindex(wv, idx));
		
		// Push samples
		for(j = 0; j < step && idx + j < max_idx; j++){
			// Get sample value
			samp = wav_fsampat(wv, idx + j, channel);
			
			// Push samples to particular frequencies
			for(i = 0; i < freqs_len; i++) freqtbl_push(freq_tbls[i], samp);
			
			// Push samples to spectrum
			spec_push(spec, samp);
		}
		idx += step;
		
		// Print particular frequency table values
		for(i = 0; i < freqs_len; i++){
			ampl = freqtbl_get(freq_tbls[i]);
			if(ampl >= 0) printf(" %6.4lf |", scaling * ampl);
		}
		
		// Print spectrum values
		for(i = 0; i < frq_count; i++) print_degree(scaling * spec_get(spec, i));
		putchar('|');
	}while(idx < max_idx);
	
	// Print footer
	printf("\n+---------+");
	for(i = 0; i < freqs_len; i++) printf("--------+");
	for(i = 0; i < frq_count; i++) putchar('-');
	printf("+\n");
	
	return 0;
}

