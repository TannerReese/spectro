CC=gcc
FLAGS=

spectro: spectro.o wav.o fourier.o
	$(CC) $(FLAGS) -o spectro spectro.o wav.o fourier.o -lm

spectro.o: spectro.c wav.h fourier.h
	$(CC) $(FLAGS) -c -o spectro.o spectro.c

wav.o: wav.c wav.h
	$(CC) $(FLAGS) -c -o wav.o wav.c

fourier.o: fourier.c fourier.h
	$(CC) $(FLAGS) -c -o fourier.o fourier.c



clean:
	rm *.o
	rm spectro

