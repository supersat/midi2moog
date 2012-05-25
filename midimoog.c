/* A MIDI file to Moog Doodle converter.

Feed the output file into the bin-to-url.pl script to get a playable URL!

Copyright (c) 2012, Karl Koscher (supersat at uw.edu)
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

 - Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 - Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#include <stdio.h>
#include <malloc.h>
#include <math.h>
#include <stdlib.h>

typedef struct event_s {
	char type;
	float value;
	struct event_s* next;
} Event;

typedef struct chunk_s {
	int delay;
	Event* events;
	struct chunk_s* next;
} Chunk;

char* bitBuffer;
int nBitsAllocated, bitPos;
Event* curEvent;
Chunk* curChunk;
Chunk* track[4];
int curTrack;
int tempo;
int ticksPerQN;

void addEvent(char type, float value)
{
	if (!curEvent) {
		curEvent = curChunk->events = malloc(sizeof(Event));
	} else {
		curEvent = curEvent->next = malloc(sizeof(Event));
	}

	curEvent->type = type;
	curEvent->value = value;
	curEvent->next = NULL;
	
	printf("EVENT: %d\tValue: %f\n", type, value);
}

void addTrackInitEvents()
{
	// These are the doodle defaults
	addEvent(3, 0.05f);
	addEvent(4, 2100);
	addEvent(5, 7);
	addEvent(6, 0.82f);
	addEvent(7, 0);
	addEvent(8, 0.8f);
	addEvent(9, 0);
	addEvent(10, 0);
	addEvent(11, 0.4f);
	addEvent(12, 1);
	addEvent(14, 0);
	addEvent(15, 4);
	addEvent(16, 4);
	addEvent(17, 0.46f);
	addEvent(18, 16);
	addEvent(19, 0);
	addEvent(20, 4);
	addEvent(21, 0.82f);
	addEvent(22, 2);
	addEvent(23, 0);
	addEvent(24, 0);
	addEvent(25, 0.46f);
	addEvent(26, 1);
}

void addChunk(int delay)
{
	if (!curChunk) {
		curChunk = track[curTrack] = malloc(sizeof(Chunk));

		curChunk->delay = delay;
		curChunk->events = NULL;
		curChunk->next = NULL;
		curEvent = NULL;

		addTrackInitEvents();
	} else {
		curChunk = curChunk->next = malloc(sizeof(Chunk));

		curChunk->delay = delay;
		curChunk->events = NULL;
		curChunk->next = NULL;
		curEvent = NULL;
	}
}

int getDeltaT(FILE* midFile)
{
	int deltaT = 0;
	unsigned char b;

	do {
		deltaT <<= 7;
		fread(&b, 1, 1, midFile);
		deltaT |= (b & 0x7f);
	} while (b & 0x80);

	return deltaT;
}

int convToMoogDelay(int deltaT)
{
	// Moog delays are in 50ms increments
	int uSperTick = tempo / ticksPerQN;
	return (uSperTick * deltaT / 50000) + 0.5; // Round up
}

int convToMoogKey(unsigned char midiKey)
{
	// A4 = MIDI 69 = Moog 4
	int note = midiKey - 65;
	
	// TODO: Adjust the octave of the synth if the note is out of range.
	// For now, change the octave of the note.
	while (note < 0) {
		note += 12;
	}

	while (note > 23) {
		note -= 12;
	}

	return note;
}

int putValue(char* bits, int bitPos, int value, int nBits) {
	while (nBits--) {
		bits[bitPos++] = (value & 1);
		value >>= 1;
	}

	return bitPos;
}

int getNumChunks(Chunk* head)
{
	Chunk* cur = head;
	int count = 0;

	while (cur) {
		cur = cur->next;
		count++;
	}

	return count;
}

int getNumEvents(Event* head)
{
	Event* cur = head;
	int count = 0;

	while (cur) {
		cur = cur->next;
		count++;
	}

	return count;
}

int lerp20(float value, float min, float max)
{
	return (int)((value - min) * pow(2, 20) / (max - min));
}

int putEvent(char* bits, int bitPos, Event* event)
{
	switch (event->type) {
	case 0:
		bits[bitPos++] = 0;
		bitPos = putValue(bits, bitPos, (int)event->value, 5);
		break;
	case 1:
		bits[bitPos++] = 1;
		bits[bitPos++] = 0;
		break;
	case 2:
		bits[bitPos++] = 0;
		bitPos = putValue(bits, bitPos, event->value == 0 ? 25 : 24, 5);
		break;
	case 14:
		bits[bitPos++] = 0;
		bitPos = putValue(bits, bitPos, event->value == 0 ? 27 : 26, 5);
		break;

	case 26:
		bits[bitPos++] = 0;
		bitPos = putValue(bits, bitPos, event->value == 0 ? 29 : 28, 5);
		break;
	case 3:
	case 7:
	case 8:
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
		bits[bitPos++] = 1;
		bits[bitPos++] = 1;
		bitPos = putValue(bits, bitPos, event->type, 5);
		bitPos = putValue(bits, bitPos, lerp20(event->value, 0, 1), 20);
		break;
	case 4:
		bits[bitPos++] = 1;
		bits[bitPos++] = 1;
		bitPos = putValue(bits, bitPos, event->type, 5);
		bitPos = putValue(bits, bitPos, lerp20(event->value, 100, 5100), 20);
		break;
	case 5:
		bits[bitPos++] = 1;
		bits[bitPos++] = 1;
		bitPos = putValue(bits, bitPos, event->type, 5);
		bitPos = putValue(bits, bitPos, lerp20(event->value, 1, 16), 20);
		break;
	case 6:
	case 17:
	case 21:
	case 25:
		bits[bitPos++] = 1;
		bits[bitPos++] = 1;
		bitPos = putValue(bits, bitPos, event->type, 5);
		bitPos = putValue(bits, bitPos, lerp20(event->value, 0.1f, 1), 20);
		break;
	case 19:
	case 23:
		bits[bitPos++] = 1;
		bits[bitPos++] = 1;
		bitPos = putValue(bits, bitPos, event->type, 5);
		bitPos = putValue(bits, bitPos, lerp20(event->value, -1, 1), 20);
		break;
	case 15:
	case 18:
	case 22:
		bits[bitPos++] = 1;
                bits[bitPos++] = 1;
                bitPos = putValue(bits, bitPos, event->type, 5);
		if (event->value == 32) {
			bitPos = putValue(bits, bitPos, 0, 3);
		} else if (event->value == 16) {
			bitPos = putValue(bits, bitPos, 1, 3);
		} else if (event->value == 8) {
			bitPos = putValue(bits, bitPos, 2, 3);
		} else if (event->value == 4) {
			bitPos = putValue(bits, bitPos, 3, 3);
		} else if (event->value == 2) {
			bitPos = putValue(bits, bitPos, 4, 3);
		} else if (event->value == 0.0625f) {
			bitPos = putValue(bits, bitPos, 5, 3);
		} else {
			fprintf(stderr, "OOPS! Can't serialize event %d, value %f\n", event->type, event->value);
			exit(0);
		}
		break;
	case 16:
	case 20:
	case 24:
		bits[bitPos++] = 1;
                bits[bitPos++] = 1;
                bitPos = putValue(bits, bitPos, event->type, 5);
		bitPos = putValue(bits, bitPos, (int)event->value, 3);
		break;
	default:
		fprintf(stderr, "OOPS! Don't know how to serialize event %d!\n", event->type);
		exit(0);
	}
	
	return bitPos;
}

void serialize(FILE* outFile)
{
	char bits[1048576];
	int bitPos = 0;
	int nEvents;
	int i;
	unsigned char b;

	// HACK: Only output the first track for now. Multiple tracks sound bad due to
	// timing issues and the fact that they all use the same synthesis parameters.
	for (curTrack = 0; curTrack < 1; curTrack++) {
		if (!track[curTrack])
			return;

		bitPos = putValue(bits, bitPos, getNumChunks(track[curTrack]), 10);
		
		curChunk = track[curTrack];
		while (curChunk) {
			if (curChunk->delay > 63) {
				bits[bitPos++] = 1;
				bitPos = putValue(bits, bitPos, curChunk->delay, 10);
			} else {
				bits[bitPos++] = 0;
				bitPos = putValue(bits, bitPos, curChunk->delay, 6);
			}

			nEvents = getNumEvents(curChunk->events);
			if (nEvents == 1) {
				bits[bitPos++] = 1;
			} else {
				bits[bitPos++] = 0;
				bitPos = putValue(bits, bitPos, nEvents, 5);
			}

			curEvent = curChunk->events;
			while (curEvent) {
				bitPos = putEvent(bits, bitPos, curEvent);
				curEvent = curEvent->next;
			}

			curChunk = curChunk->next;
		}
	}

	for (i = 0; i < bitPos; i += 8) {
		b = (bits[i] << 7) |
		    (bits[i + 1] << 6) |
		    (bits[i + 2] << 5) |
		    (bits[i + 3] << 4) |
		    (bits[i + 4] << 3) |
		    (bits[i + 5] << 2) |
		    (bits[i + 6] << 1) |
		    bits[i + 7];
		fwrite(&b, 1, 1, outFile);
	}		
}

int main(int argc, char* argv[])
{
	FILE* midFile;
	FILE* outFile;
	unsigned char buf[16], lastCmd = 0;
	int i, nTracks;
	long trackEnd;
	int totalTime;
	int timeLimit;

	if (argc != 3 && argc != 4) {
		fprintf(stderr, "Usage: %s <midi file> <moog binary file> [max quarter notes]\n", argv[0]);
		return -1;
	}

	midFile = fopen(argv[1], "rb");
	if (!midFile) {
		perror(argv[1]);
		return -1;
	}

	outFile = fopen(argv[2], "wb");
	if (!outFile) {
		perror(argv[2]);
		return -1;
	}

	// Read the MIDI header
	fread(buf, 14, 1, midFile);
	if (memcmp(buf, "MThd\x00\x00\x00\x06", 8)) {
		fprintf(stderr, "Unexpected MIDI file format\n");
		return -1;
	}

	nTracks = (buf[10] << 8) | buf[11];
	ticksPerQN = (buf[12] << 8) | buf[13];

	if (argc == 4) {
		timeLimit = ticksPerQN * atoi(argv[3]);
	} else {
		timeLimit = 0;
	}
		
	curTrack = 0;
	curChunk = NULL;
	curEvent = NULL;
	for (i = 0; i < 4; i++) {
		track[i] = NULL;
	}

	printf("Found %d tracks (%d ticks per quarter-note)\n", nTracks, ticksPerQN);
	
	while (nTracks--) {
		printf("--- NEXT TRACK ---\n");
		// Advance Moog track if the previous MIDI track had any events in it
		if (curChunk) {
			curTrack++;
			curChunk = NULL;
			curEvent = NULL;
		}

		fread(buf, 8, 1, midFile);
		if (memcmp(buf, "MTrk", 4)) {
			fprintf(stderr, "Expected to find MIDI track header\n");
			return -1;
		}

		trackEnd = ftell(midFile) + ((buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7]);
		totalTime = 0;

		while (ftell(midFile) < trackEnd) {
			int deltaT = getDeltaT(midFile);
			totalTime += deltaT;

			fread(buf, 1, 1, midFile);
			if (buf[0] & 0x80) {
				lastCmd = buf[0];
			} else {
				// This is a parameter for the last command. Unread the first byte.
				fseek(midFile, -1, SEEK_CUR);
			}

			switch (lastCmd >> 4) {
			case 8:
				fread(buf, 2, 1, midFile);
				printf("%08d KEY OFF: Chan %2d, Key %3d, Velocity %3d\n", deltaT, lastCmd & 0xf, buf[0], buf[1]);

				if (timeLimit && totalTime >= timeLimit)
					break;

				if (deltaT || !track[curTrack]) {
					addChunk(convToMoogDelay(deltaT));
				}

				// This assumes only one key is on at any given time.
				addEvent(1, 0);
				break;
			case 9:
				fread(buf, 2, 1, midFile);
				printf("%08d KEY ON:  Chan %2d, Key %3d, Velocity %3d\n", deltaT, lastCmd & 0xf, buf[0], buf[1]);

				if (timeLimit && totalTime >= timeLimit)
                                        break;

				if (deltaT || !track[curTrack]) {
					addChunk(convToMoogDelay(deltaT));
				}

				addEvent(0, convToMoogKey(buf[0]));
				break;
			case 0xa:
				fread(buf, 2, 1, midFile);
				printf("%08d KEY AT:  Chan %2d, Key %3d, Velocity %3d\n", deltaT, lastCmd & 0xf, buf[0], buf[1]);
				break;
			case 0xb:
				fread(buf, 2, 1, midFile);
				printf("%08d CTRL C:  Chan %2d, Control %3d, Value %3d\n", deltaT, lastCmd & 0xf, buf[0], buf[1]);
				break;
			case 0xc:
				fread(buf, 1, 1, midFile);
				printf("%08d PATCH C: Chan %2d, Patch %3d\n", deltaT, lastCmd & 0xf, buf[0]);
				break;
			case 0xd:
				fread(buf, 1, 1, midFile);
				printf("%08d CH AT:   Chan %2d, Channel %3d\n", deltaT, lastCmd & 0xf, buf[0]);
				break;
			case 0xe:
				fread(buf, 2, 1, midFile);
				printf("%08d PITCH W: Chan %d, Value %5d\n", deltaT, lastCmd & 0xf, (buf[1] << 7) | buf[0]);
				break;
			case 0xf:
				if (lastCmd == 0xff) {
					// meta-events. Read type and length.
					fread(buf, 2, 1, midFile);

					// We only care about tempo.
					if (buf[0] == 0x51) {
						if (buf[1] != 3) {
							fprintf(stderr, "Unexpected tempo length\n");
							return -1;
						}
	
						fread(buf, 3, 1, midFile);
						tempo = (buf[0] << 16) | (buf[1] << 8) | buf[2];
						printf("Tempo %d\n", tempo);
					} else {
						fseek(midFile, buf[1], SEEK_CUR);
					}
				}
			}
		}
	}

	serialize(outFile);
}
