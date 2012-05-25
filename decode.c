// Karl's really crappy doodle decoder
#include <stdio.h>
#include <math.h>
#include <malloc.h>

int cc(char* bits, long offset, int n) {
	int result = 0;

	// WHO THE $#%#@$@#T% STORES VALUES LSB FIRST?!!?!
	while (n--) {
		result *= 2;
		result |= bits[offset + n];
	}

	return result;
}

float S(char* bits, long offset, float min, float max) {
	long intVal = cc(bits, offset, 20);
	if (!intVal)
		return 0;
	// Missing max?
	return intVal / pow(2, 20) * (max - min) + min;
}

int main(int argc, char* argv[]) {
	FILE* f;
	long fileSize, bitPos, i, j;
	char* bits;
	char byteRead;
	int type;
	int delay, nEvents, nChunks, trackNum;
	float value;

	f = fopen(argv[1], "rb");
	if (!f) {
		perror(argv[1]);
		return -1;
	}

	fseek(f, 0, SEEK_END);
	fileSize = ftell(f);
	fseek(f, 0, SEEK_SET);

	// Ugly but what the hell
	bits = malloc(fileSize * 8);
	bitPos = 0;
	for (i = 0; i < fileSize; i++) {
		fread(&byteRead, 1, 1, f);
		for (j = 0; j < 8; j++) {
			bits[bitPos++] = (byteRead & 0x80) ? 1 : 0;
			byteRead *= 2;
		}
	}

	// LET'S DO SOME DECODING!!!111
	bitPos = 0;
	trackNum = 0;
	while (bitPos < fileSize * 8) {
		nChunks = cc(bits, bitPos, 10);
		bitPos += 10;

		printf("--- TRACK %d, %3d CHUNKS ---\n", trackNum++, nChunks);

		while (nChunks--) {
			if (!bits[bitPos++]) {
				delay = cc(bits, bitPos, 6);
				bitPos += 6;
			} else {
				delay = cc(bits, bitPos, 10);
				bitPos += 10;
			}
			
			if (!bits[bitPos++]) {
				nEvents = cc(bits, bitPos, 5);
				bitPos += 5;
			} else {
				nEvents = 1;
			}
	
			printf("DELAY: %d, %2d EVENTS\n", delay, nEvents);
			while (nEvents--) {
				if (!bits[bitPos++]) {
					type = cc(bits, bitPos, 5);
					bitPos += 5;
		
					if (type < 24) {
						printf("\tEVENT:  0 = %d\n", type);
					} else if (type == 24) {
						printf("\tEVENT:  2 = 1\n");
					} else if (type == 25) {
						printf("\tEVENT:  2 = 0\n");
					} else if (type == 26) {
						printf("\tEVENT: 14 = 1\n");
					} else if (type == 27) {
						printf("\tEVENT: 14 = 0\n");
					} else if (type == 28) {
						printf("\tEVENT: 26 = 1\n");
					} else if (type = 29) {
						printf("\tEVENT: 26 = 0\n");
					} else {
						fprintf(stderr, "OOPS!\n");
					}
				} else if (!bits[bitPos++]) {
					printf("\tEVENT:  1 = 0\n");
				} else {
					type = cc(bits, bitPos, 5);
					bitPos += 5;
					switch(type) {
					case 15:
					case 18:
					case 22:
						switch(cc(bits, bitPos, 3)) {
						case 0: value = 32; break;
						case 1: value = 16; break;
						case 2: value = 8; break;
						case 3: value = 4; break;
						case 4: value = 2; break;
						case 5: value = 0.0625f; break;
						default: value = -1;
						}
						bitPos += 3;
						break;
					case 16:
					case 20:
					case 24:
						value = cc(bits, bitPos, 3);
						bitPos += 3;
						break;
					case 1:
					case 3:
					case 7:
					case 8:
					case 9:
					case 10:
					case 11:
					case 12:
					case 13:
						value = S(bits, bitPos, 0, 1);
						bitPos += 20;
						break;
					case 4:
						value = S(bits, bitPos, 100, 5100);
						bitPos += 20;
						break;
					case 5:
						value = S(bits, bitPos, 1, 16);
						bitPos += 20;
						break;
					case 6:
					case 17:
					case 21:
					case 25:
						value = S(bits, bitPos, 0.1, 1);
						bitPos += 20;
						break;
					case 19:
					case 23:
						value = S(bits, bitPos, -1, 1);
						bitPos += 20;
					break;
					default:
						fprintf(stderr, "OOPS: What about event %d?\n", type);
						value = -999;
					}
					printf("\tEVENT: %2d = %f\n", type, value);
				}
			}
		}
	}

	// Free memory?
	// Close files?
	// nah.
}
