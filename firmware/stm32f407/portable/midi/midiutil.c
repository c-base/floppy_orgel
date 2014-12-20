/*
 * midiutil.c - Auxiliary MIDI functionality. Requires Steevs MIDI Library
 *		        (midiinfo.h) for enumerations used in name mapping.
 * Version 1.4
 *
 *  AUTHOR: Steven Goodwin (StevenGoodwin@gmail.com)
 *			Copyright 2010, Steven Goodwin.
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of
 *  the License,or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include "midifile.h"
#include "midiutil.h"

/*
** Data Tables
*/
static char *szPatchList[128] = {
	/*Pianos*/
	"Acoustic Grand Piano",
	"Bright Acoustic Piano",
	"Electric Grand Piano",
	"Honky-tonk Piano",
	"Electric Piano 1",
	"Electric Piano 2",
	"Harpsichord",
	"Clavinet",
	/*Chromatic Percussion*/
	"Celesta",
	"Glockenspiel",
	"Music Box",
	"Vibraphone",
	"Marimba",
	"Xylophone",
	"Tubular Bells",
	"Dulcimer",
	/*Organs*/
	"Draw Organ",
	"Percussive Organ",
	"Rock Organ",
	"Church Organ",
	"Reed Organ",
	"Accordian",
	"Harmonica",
	"Tango Accordian",
	/*Guitars*/
	"Acoustic Guitar (nylon)",
	"Acoustic Guitar (steel)",
	"Electric Guitar (jazz)",
	"Electric Guitar (clean)",
	"Electric Guitar (muted)",
	"Overdriven Guitar",
	"Distortion Guitar",
	"Guitar harmonics",
	/*Basses*/
	"Acoustic bass",
	"Electric Bass (finger)",
	"Electric Bass (picked)",
	"Fretless Bass",
	"Slap Bass 1",
	"Slap Bass 2",
	"Synth bass 1",
	"Synth bass 2",
	/*Strings*/
	"Violin",
	"Viola",
	"Cello",
	"Contrabass",
	"Tremolo strings",
	"Pizzicato strings",
	"Orchestral harp",
	"Timpani",
	/*Ensembles*/
	"String ensemble 1",
	"String ensemble 2",
	"Synth strings 1",
	"Synth strings 2",
	"Choir Ahhs",
	"Voice oohs",
	"Synth voice",
	"Orchestra hit",
	/*Brass*/
	"Trumpet",
	"Trombone",
	"Tuba",
	"Muted trumpet",
	"French horn",
	"Brass section",
	"Synth brass 1",
	"Synth brass 2",
	/*Reeds*/
	"Soprano sax",
	"Alto sax",
	"Tenor sax",
	"Baritone sax",
	"Oboe",
	"English horn",
	"Bassoon",
	"Clarinet",
	/*Pipes*/
	"Picclo",
	"Flute",
	"Recorder",
	"Pan Flute",
	"Bottle Blow",
	"Shakuhachi",
	"Whistle",
	"Ocarina",
	/*Synth Lead*/
	"Lead 5 (Square)",
	"Lead 5 (Sawtooth)",
	"Lead 5 (Calliope)",
	"Lead 5 (Chiff)",
	"Lead 5 (Charang)",
	"Lead 5 (Voice)",
	"Lead 5 (Fifths)",
	"Lead 5 (Bass+lead)",
	/*Synth Pads*/
	"Pad 1 (New age)",
	"Pad 2 (Warm)",
	"Pad 3 (Polysynth)",
	"Pad 4 (Choir)",
	"Pad 5 (Bowed)",
	"Pad 6 (Metallic)",
	"Pad 7 (Halo)",
	"Pad 8 (Sweep)",
	/*Synth FX*/
	"FX 1 (Rain)",
	"FX 2 (Soundtrack)",
	"FX 3 (Crystal)",
	"FX 4 (Atmosphere)",
	"FX 5 (Brightness)",
	"FX 6 (Goblins)",
	"FX 7 (Echoes)",
	"FX 8 (Sci-fi)",
	/*Ethnic*/
	"Sitar",
	"Banjo",
	"Shamisen",
	"Koto",
	"Kalimba",
	"Bagpipe",
	"Fiddle",
	"Shanai",
	/*Percussive*/
	"Tinkle bell",
	"Agogo",
	"Steel drums",
	"Woodblock",
	"Taiko drum",
	"Melodic tom",
	"Synth drum",
	"Reverse cymbal",
	/*FX*/
	"Guitar fret noise",
	"Breath noise",
	"Seashore",
	"Bird tweet",
	"Telephone ring",
	"Helicopter",
	"Applause",
	"Gunshot",
};

static char *szGMDrums[128]={
	"???",	/* C0 */
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	/* C1  */
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	
	"???",	/* C2 */
	"???",	
	"???",	
	"High Q",	
	"Slap",	
	"???",	
	"???",	
	"Sticks",	
	"Square click",	
	"???",	
	"???",	
	"Acoustic Kick Drum",	
	"Electric Kick Drum",		/* C3=36 */
	"Side Stick",
	"Acoustic Snare Drum",
	"Hand Clap",
	"Electric Snare Drum ",	/*(crisp, electronic)", */
	"Low Floor Tom",
	"Closed Hi Hat",
	"High Floor Tom",
	"Opening Hi Hat",	/* pedal hh */
	"Low Tom",
	"Open Hi Hat",
	"Low Mid Tom",
	"High Mid Tom",		/* c4 */
	"Crash Cymbal 1",
	"High Tom",
	"Ride Cymbal 1",
	"Chinese Boom",
	"Ride Bell",
	"Tamborine",
	"Splash cymbal",
	"Cowbell",
	"Crash Cymbal (2)",
	"Vibra Slap",
	"Ride Cymbal (2)",
	"Hi bongo",		/* c5 */
	"Lo bongo",
	"Mute High Conga",
	"Open High Conga",
	"Low Conga",
	"High Timbale",
	"Low Timbale",
	"High Agogo",
	"Low Agogo",
	"Cabasa",
	"Maracas",
	"Short Hi Whistle",
	"Long Low Whistle",	/*c6 */
	"Short Guiro",
	"Long Guiro",
	"Claves",
	"High Woodblock",
	"Low Woodblock",
	"Mute Cuica",
	"Open Cuica",
	"Mute Triangle",
	"Open Triangle",
	"Shaker",
	"Jingle Bell",
	"Tring",	/* C7 */
	"Castinets",	
	"Mute Sudro",	
	"Open Sudro",	
};

static char *szCCList[] = {
	"Bank Select",
	"Modulation",
	"Breath Control",
	"Undefined 3",
	"Foot",
	"Portamento Time",
	"Date Entry",
	"Volume	",
	"Balance",
	"Undefined 9",
	"Pan",
	"Expression",
	"Effort Control 1",
	"Effort Control 2",
	"Undefined 14",
	"Undefined 15",
	"General Purpose 1",
	"General Purpose 2",
	"General Purpose 3",
	"General Purpose 4",
	/* 20-31 are undefined */
	"Undefined 20",
	"Undefined 21",
	"Undefined 22",
	"Undefined 23",
	"Undefined 24",
	"Undefined 25",
	"Undefined 26",
	"Undefined 27",
	"Undefined 28",
	"Undefined 29",
	"Undefined 30",
	"Undefined 31",
	/* LSB for control changes 0-31		32-63 */
	"lsb-32", "lsb-33", "lsb-34", "lsb-35", "lsb-36", "lsb-37", "lsb-38", 
	"lsb-39", "lsb-40", "lsb-41", "lsb-42", "lsb-43", "lsb-44", "lsb-45", 
	"lsb-46", "lsb-47", "lsb-48", "lsb-49", "lsb-50", "lsb-51", "lsb-52", 
	"lsb-53", "lsb-54", "lsb-55", "lsb-56", "lsb-57", "lsb-58", "lsb-59", 
	"lsb-60", "lsb-61", "lsb-62", "lsb-63",

	"Sustain Pedal",
	"Portamento",
	"Pedal Sustenuto",
	"Pedal Soft",
	"Legato Foot Switch",
	"Hold 2",
	"Sound Variation",
	"Harm Content",
	"Release Time",
	"Attack Time",
	"Brightness",
	"Reverb",
	"Delay",
	"Pitch Transpose",
	"Flange",
	"Special FX",
	"General Purpose 5",
	"General Purpose 6",
	"General Purpose 7",
	"General Purpose 8",
	"Portamento Control",
	/* 85-90 are undefined */
	"Undefined 85",
	"Undefined 86",
	"Undefined 87",
	"Undefined 88",
	"Undefined 89",
	"Undefined 90",
	"FX Depth",
	"Tremelo Depth",
	"Chorus Depth",
	"Celesta Depth",
	"Phaser Depth",
	"Data Inc",
	"Data Dec",
	"Non Reg Param LSB",
	"Non Ref Param MSB",
	"Reg Param LSB",
	"Reg Param MSB",
	/* 102-119 are undefined */
	"Undefined 102",
	"Undefined 103",
	"Undefined 104",
	"Undefined 105",
	"Undefined 106",
	"Undefined 107",
	"Undefined 108",
	"Undefined 109",
	"Undefined 110",
	"Undefined 111",
	"Undefined 112",
	"Undefined 113",
	"Undefined 114",
	"Undefined 115",
	"Undefined 116",
	"Undefined 117",
	"Undefined 118",
	"Undefined 119",
	"All Sound Off",
	"Reset All Controllers",
	"Local Control",
	"All Notes Off",
	"Omni Mode Off",
	"Omni Mode On",
	"Mono Mode On",
	"Poly Mode On",
};

static char *szNoteName[] = {
	"C ",
	"Db",
	"D ",
	"Eb",
	"E ",
	"F ",
	"Gb",
	"G ",
	"Ab",
	"A ",
	"Bb",
	"B ",
};

static float fFreqlist[] = {
	261.63f,
	277.18f,
	293.66f,
	311.13f,
	329.63f,
	349.23f,
	369.99f,
	392.00f,
	415.30f,
	440.00f,
	466.16f,
	493.88f,
};

/*
** Name resolving functions
*/
BOOL muGetInstrumentName(char *pName, int32_t iInstr)
{
	if (iInstr < 0 || iInstr > 127)
		return FALSE;
  strcpy_s(pName, 32, szPatchList[iInstr]);
	return TRUE;
}

BOOL muGetDrumName(char *pName, int32_t iInstr)
{
	if (iInstr < 0 || iInstr > 127)
		return FALSE;
	strcpy_s(pName, 32, szGMDrums[iInstr]);
	return TRUE;
}

void muGetMIDIMsgName(char *pName, tMIDI_MSG iMsg) {
	switch(iMsg){
		case	msgNoteOff:         strcpy_s(pName, 32, "Note off");          break;
		case	msgNoteOn:          strcpy_s(pName, 32, "Note on");           break;
		case	msgNoteKeyPressure: strcpy_s(pName, 32, "Note key pressure"); break;
		case	msgSetParameter:    strcpy_s(pName, 32, "Set parameter");     break;
		case	msgSetProgram:      strcpy_s(pName, 32, "Set program");       break;
		case	msgChangePressure:  strcpy_s(pName, 32, "Change pressure");   break;
		case	msgSetPitchWheel:   strcpy_s(pName, 32, "Set pitch wheel");   break;
		case	msgMetaEvent:       strcpy_s(pName, 32, "Meta event");        break;
		case	msgSysEx1:          strcpy_s(pName, 32, "SysEx1");            break;
		case	msgSysEx2:          strcpy_s(pName, 32, "SysEx2");            break;
    default:                  strcpy_s(pName, 32, "Unknown");           break;
		}
}

BOOL muGetControlName(char *pName, tMIDI_CC iCC)
{
	if (iCC < 0 || iCC > 127)
		return FALSE;
	strcpy_s(pName, 32, szCCList[iCC]);
	return TRUE;
}

BOOL muGetKeySigName(char *pName, tMIDI_KEYSIG iKey)
{
static char *iKeysList[2][8] = {
/*#*/{"C ", "G ", "D ", "A ", "E ", "B ", "F#", "C#", },
/*b*/{"C ", "F ", "Bb", "Eb", "Ab", "Db", "Gb", "Cb", },
};

int32_t iRootNum = (iKey&7);
int32_t iFlats = (iKey&keyMaskNeg);
int32_t iMin = (iKey&keyMaskMin);

	strcpy_s(pName, 8, iKeysList[iFlats?1:0][iRootNum]);
	strcat_s(pName, 8, iMin ? " Min" : " Maj");
	return TRUE;
}

BOOL muGetTextName(char *pName, tMIDI_TEXT iEvent)
{
	if (iEvent<1 || iEvent>7)	return FALSE;
	return muGetMetaName(pName, (tMIDI_META)iEvent);
}

BOOL muGetMetaName(char *pName, tMIDI_META iEvent)
{
	switch(iEvent)
		{
		case	metaSequenceNumber:	    strcpy_s(pName, 32, "Sequence Number");	    break;
		case	metaTextEvent:		      strcpy_s(pName, 32, "Text Event");		      break;
		case	metaCopyright:		      strcpy_s(pName, 32, "Copyright");			      break;
		case	metaTrackName:		      strcpy_s(pName, 32, "Track Name");		      break;
		case	metaInstrument:		      strcpy_s(pName, 32, "Instrument");		      break;
		case	metaLyric:			        strcpy_s(pName, 32, "Lyric");				        break;
		case	metaMarker:			        strcpy_s(pName, 32, "Marker");			        break;
		case	metaCuePoint:		        strcpy_s(pName, 32, "Cue Point");			      break;
		case	metaMIDIPort:		        strcpy_s(pName, 32, "MIDI Port");		        break;
		case	metaEndSequence:	      strcpy_s(pName, 32, "End Sequence");		    break;
		case	metaSetTempo:		        strcpy_s(pName, 32, "Set Tempo");			      break;
		case	metaSMPTEOffset:	      strcpy_s(pName, 32, "SMPTE Offset");		    break;
		case	metaTimeSig:		        strcpy_s(pName, 32, "Time Sig");			      break;
		case	metaKeySig:			        strcpy_s(pName, 32, "Key Sig");			        break;
		case	metaSequencerSpecific:	strcpy_s(pName, 32, "Sequencer Specific");	break;
		default:	return FALSE;
		}
	return TRUE;

}


/*
** Conversion Functions
*/
int32_t muGetNoteFromName(const char *pName) {
  int32_t note_map[] = {9, 11, 0, 2, 4, 5, 7};
  char *p, cpy[16];
  int32_t note = 0;

	strncpy_s(cpy, 256, pName, 15);
	cpy[15] = '\0';
	p = cpy;

	while(!isalpha(*p) && *p)
		p++;
	
	if (*p) {
		note = toupper(*p)-'A';
		if (note >= 0 && note <= 7) {
			note = note_map[note];
			p++;
			if (*p == 'b')
				note--, p++;
			else if (*p == '#')
				note++, p++;
			
			note += atoi(p)*12+MIDI_NOTE_C0;
    }
  }
	
	return note;
}

char *muGetNameFromNote(char *pStr, int32_t iNote) {
	if (!pStr)		return NULL;

	if (iNote<0 || iNote>127)
		strcpy_s(pStr, 8, "ERR");
	else
		sprintf_s(pStr, 8, "%s%d", szNoteName[iNote%12], ((iNote-MIDI_NOTE_C0) / 12));
	
	return pStr;
}

float muGetFreqFromNote(int32_t iNote)
{
int32_t oct = iNote/12-5;
float freq;

	if (iNote<0 || iNote>127)	return 0;

	freq = fFreqlist[iNote%12];
	
	while(oct > 0)
		freq *= 2.0f, oct--;
	
	while(oct < 0)
		freq /= 2.0f, oct++;
	
	return freq;
}

int32_t muGetNoteFromFreq(float fFreq)
{
/* This is for completeness, I'm not sure of how often it
** will get used. Therefore, the code is un-optimised :)
*/
int32_t iNote, iBestNote=0;
float fDiff=20000, f;

	for(iNote=0;iNote<127;++iNote)
		{
		f = muGetFreqFromNote(iNote);
		f -= fFreq; if (f<0) f=-f;
		if (f < fDiff)
			{
			fDiff = f; 
			iBestNote = iNote;
			}
		}
		
	return iBestNote;
}


int32_t muGuessChord(const int32_t *pNoteStatus, const int32_t channel, const int32_t lowRange, const int32_t highRange) {
	int32_t octave[24];
	int32_t i;
	int32_t lowestNote=999;
	int32_t startNote = 999;
	int32_t chordRoot = 0;
	int32_t chordType = 0;
	int32_t chordAdditions = 0;

	for(i=0;i<24;++i) {
		octave[i] = 0;
	}

	for(i=lowRange;i<=highRange;++i) {
		if (pNoteStatus[channel*128 + i]) {
			if (i<lowestNote) {
				lowestNote = i;
			}
			++octave[i%12];
			++octave[i%12+12];
			if ((i%12) < startNote) {
				startNote = i%12;
			}
		}
	}

	if (lowestNote == 999) {
		return -1;
	}

	/* Bring it into line with the 0-11 range */
	lowestNote %= 12;
	
	/* Majors */
	if (octave[startNote+3] && octave[startNote+8]) {
		chordRoot = startNote+8;
		chordType = CHORD_TYPE_MAJOR;
	} else if (octave[startNote+5] && octave[startNote+9]) {
		chordRoot = startNote+5;
		chordType = CHORD_TYPE_MAJOR;
	} else if (octave[startNote+4] && octave[startNote+7]) {
		chordRoot = startNote;
		chordType = CHORD_TYPE_MAJOR;

	/* Minor */
	} else if (octave[startNote+4] && octave[startNote+9]) {
		chordRoot = startNote+9;
		chordType = CHORD_TYPE_MINOR;
	} else if (octave[startNote+5] && octave[startNote+8]) {
		chordRoot = startNote+5;
		chordType = CHORD_TYPE_MINOR;
	} else if (octave[startNote+3] && octave[startNote+7]) {
		chordRoot = startNote;
		chordType = CHORD_TYPE_MINOR;

	/* Diminished */
	} else if (octave[startNote+3] && octave[startNote+6]) {
		chordRoot = lowestNote;
		chordType = CHORD_TYPE_DIM;
	} else if (octave[startNote+6] && octave[startNote+9]) {
		chordRoot = lowestNote;
		chordType = CHORD_TYPE_DIM;

	/* Augmented */
	} else if (octave[startNote+4] && octave[startNote+8]) {
		chordRoot = lowestNote;
		chordType = CHORD_TYPE_AUG;
	}

	if (octave[chordRoot + 10]) {
		chordAdditions |= CHORD_ADD_7TH;
	}
	if (octave[chordRoot + 11]) {
		chordAdditions |= CHORD_ADD_MAJ7TH;
	}
	if (octave[chordRoot + 2]) {
		chordAdditions |= CHORD_ADD_9TH;
	}

	chordRoot %= 12;

	if (chordType == 0) {
		return -1;
	}

	return chordRoot | chordType | chordAdditions | (lowestNote<<16);
}

char *muGetChordName(char *str, int32_t chord) {
	int32_t root = chord & CHORD_ROOT_MASK;
	int32_t bass = (chord & CHORD_BASS_MASK) >> 16;

	if (root < 0 || root > 11) {
		root = 0;
	}

	if (bass < 0 || bass > 11) {
		bass = 0;
	}

	strcpy_s(str, 8, szNoteName[root]);

	switch(chord & CHORD_TYPE_MASK) {
		case CHORD_TYPE_MAJOR:
			break;
		case CHORD_TYPE_MINOR:
			strcat_s(str, 8, "m");
			break;
		case CHORD_TYPE_AUG:
			strcat_s(str, 8, " aug");
			break;
		case CHORD_TYPE_DIM:
			strcat_s(str, 8, " dim");
			break;
	}

	if (chord & CHORD_ADD_7TH) {
		strcat_s(str, 8, "+7");
	}
	if (chord & CHORD_ADD_9TH) {
		strcat_s(str, 8, "+9");
	}
	if (chord & CHORD_ADD_MAJ7TH) {
		strcat_s(str, 8, "+7M");
	}
	
	if (bass != root) {
		strcat_s(str, 8, "/");
		strcat_s(str, 8, szNoteName[bass]);
	}

	return str;
}

