#pragma once

// Get some classic includes
#include <Windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <conio.h>
#include <vector>
#include "OpenAL/al.h"
#include "OpenAL/alc.h"
#include "OpenAL/efx.h"
#include "OpenAL/efx-creative.h"
#include "OpenAL/xram.h"

// Initialization and shutdown
void ALFWInit();
void ALFWShutdown();

// OpenAL initialization and shutdown
ALboolean ALFWInitOpenAL();
ALboolean ALFWShutdownOpenAL();

// File loading functions
ALboolean ALFWLoadWaveToBuffer(const char *szWaveFile, ALuint uiBufferID, ALenum eXRAMBufferMode = 0);

// ScriptExtension Queries 
ALboolean ALFWIsXRAMSupported();
ALboolean ALFWIsEFXSupported();

// asdfasdfsdfsdfsdf56
struct WaveInfo
{
	ALenum format;
	ALsizei freq;
	std::vector<char> data;
};
ALboolean ALFWSplitDataToBuffer(ALuint dstBuffer, const WaveInfo& srcInfo, int startSample, int numSamples);
ALboolean ALFWGetWaveInfo(const char* szWaveFile, WaveInfo& info);
bool ALFWGetWaveInfo(const char* szWaveFile, ALenum* format, ALsizei* freq, ALchar** data, ALsizei* size);

// Utilities
ALvoid ALFWprintf( const ALchar * x, ... );
ALchar *ALFWaddMediaPath(const ALchar *filename);
ALint ALFWKeyPress(void);

// EFX ScriptExtension function pointer variables

// Effect objects
extern LPALGENEFFECTS alGenEffects;
extern LPALDELETEEFFECTS alDeleteEffects;
extern LPALISEFFECT alIsEffect;
extern LPALEFFECTI alEffecti;
extern LPALEFFECTIV alEffectiv;
extern LPALEFFECTF alEffectf;
extern LPALEFFECTFV alEffectfv;
extern LPALGETEFFECTI alGetEffecti;
extern LPALGETEFFECTIV alGetEffectiv;
extern LPALGETEFFECTF alGetEffectf;
extern LPALGETEFFECTFV alGetEffectfv;

// Filter objects
extern LPALGENFILTERS alGenFilters;
extern LPALDELETEFILTERS alDeleteFilters;
extern LPALISFILTER alIsFilter;
extern LPALFILTERI alFilteri;
extern LPALFILTERIV alFilteriv;
extern LPALFILTERF alFilterf;
extern LPALFILTERFV alFilterfv;
extern LPALGETFILTERI alGetFilteri;
extern LPALGETFILTERIV alGetFilteriv;
extern LPALGETFILTERF alGetFilterf;
extern LPALGETFILTERFV alGetFilterfv;

// Auxiliary slot object
extern LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots;
extern LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots;
extern LPALISAUXILIARYEFFECTSLOT alIsAuxiliaryEffectSlot;
extern LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti;
extern LPALAUXILIARYEFFECTSLOTIV alAuxiliaryEffectSlotiv;
extern LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf;
extern LPALAUXILIARYEFFECTSLOTFV alAuxiliaryEffectSlotfv;
extern LPALGETAUXILIARYEFFECTSLOTI alGetAuxiliaryEffectSloti;
extern LPALGETAUXILIARYEFFECTSLOTIV alGetAuxiliaryEffectSlotiv;
extern LPALGETAUXILIARYEFFECTSLOTF alGetAuxiliaryEffectSlotf;
extern LPALGETAUXILIARYEFFECTSLOTFV alGetAuxiliaryEffectSlotfv;

// XRAM ScriptExtension function pointer variables and enum values

typedef ALboolean (__cdecl *LPEAXSETBUFFERMODE)(ALsizei n, ALuint *buffers, ALint value);
typedef ALenum    (__cdecl *LPEAXGETBUFFERMODE)(ALuint buffer, ALint *value);

extern LPEAXSETBUFFERMODE eaxSetBufferMode;
extern LPEAXGETBUFFERMODE eaxGetBufferMode;

// X-RAM Enum values
extern ALenum eXRAMSize, eXRAMFree;
extern ALenum eXRAMAuto, eXRAMHardware, eXRAMAccessible;
