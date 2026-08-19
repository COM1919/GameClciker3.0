/*
 * Copyright (c) 2006, Creative Labs Inc.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided
 * that the following conditions are met:
 * 
 *     * Redistributions of source code must retain the above copyright notice, this list of conditions and
 * 	     the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright notice, this list of conditions
 * 	     and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *     * Neither the name of Creative Labs Inc. nor the names of its contributors may be used to endorse or
 * 	     promote products derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

// Win32 version of the Creative Labs OpenAL 1.1 Framework for samples
#include <windows.h>
#include <stdio.h>

#include "Famewrok.h"
#include "CWaves.h"
#include "aldlist.h"

#define STB_VORBIS_HEADER_ONLY // 告诉编译器，我们只需要头文件部分，不需要实现
#include <OpenAL/Decoder/stb_vorbis.h>

#define MINIMP3_IMPLEMENTATION // 告诉编译器，我们需要库的实现代码
#include <OpenAL/Decoder/minimp3_ex.h>

static CWaves *g_pWaveLoader = NULL;

// Imported EFX functions

// Effect objects
LPALGENEFFECTS alGenEffects = NULL;
LPALDELETEEFFECTS alDeleteEffects = NULL;
LPALISEFFECT alIsEffect = NULL;
LPALEFFECTI alEffecti = NULL;
LPALEFFECTIV alEffectiv = NULL;
LPALEFFECTF alEffectf = NULL;
LPALEFFECTFV alEffectfv = NULL;
LPALGETEFFECTI alGetEffecti = NULL;
LPALGETEFFECTIV alGetEffectiv = NULL;
LPALGETEFFECTF alGetEffectf = NULL;
LPALGETEFFECTFV alGetEffectfv = NULL;

//Filter objects
LPALGENFILTERS alGenFilters = NULL;
LPALDELETEFILTERS alDeleteFilters = NULL;
LPALISFILTER alIsFilter = NULL;
LPALFILTERI alFilteri = NULL;
LPALFILTERIV alFilteriv = NULL;
LPALFILTERF alFilterf = NULL;
LPALFILTERFV alFilterfv = NULL;
LPALGETFILTERI alGetFilteri = NULL;
LPALGETFILTERIV alGetFilteriv = NULL;
LPALGETFILTERF alGetFilterf = NULL;
LPALGETFILTERFV alGetFilterfv = NULL;

// Auxiliary slot object
LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots = NULL;
LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots = NULL;
LPALISAUXILIARYEFFECTSLOT alIsAuxiliaryEffectSlot = NULL;
LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti = NULL;
LPALAUXILIARYEFFECTSLOTIV alAuxiliaryEffectSlotiv = NULL;
LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf = NULL;
LPALAUXILIARYEFFECTSLOTFV alAuxiliaryEffectSlotfv = NULL;
LPALGETAUXILIARYEFFECTSLOTI alGetAuxiliaryEffectSloti = NULL;
LPALGETAUXILIARYEFFECTSLOTIV alGetAuxiliaryEffectSlotiv = NULL;
LPALGETAUXILIARYEFFECTSLOTF alGetAuxiliaryEffectSlotf = NULL;
LPALGETAUXILIARYEFFECTSLOTFV alGetAuxiliaryEffectSlotfv = NULL;

// XRAM functions and enum values

LPEAXSETBUFFERMODE eaxSetBufferMode = NULL;
LPEAXGETBUFFERMODE eaxGetBufferMode = NULL;

ALenum eXRAMSize = 0;
ALenum eXRAMFree = 0;
ALenum eXRAMAuto = 0;
ALenum eXRAMHardware = 0;
ALenum eXRAMAccessible = 0;


// Initialization and enumeration
void ALFWInit()
{
	g_pWaveLoader = new CWaves();
}

void ALFWShutdown()
{
	if (g_pWaveLoader)
	{
		delete g_pWaveLoader;
		g_pWaveLoader = NULL;
	}

	ALchar ch = _getch();
}

ALboolean ALFWInitOpenAL()
{
	ALDeviceList *pDeviceList = NULL;
	ALCcontext *pContext = NULL;
	ALCdevice *pDevice = NULL;
	ALboolean bReturn = AL_FALSE;

	pDeviceList = new ALDeviceList();
	if ((pDeviceList) && (pDeviceList->GetNumDevices()))
	{
		pDevice = alcOpenDevice(pDeviceList->GetDeviceName(0));
		if (pDevice)
		{
			pContext = alcCreateContext(pDevice, NULL);
			if (pContext)
			{
				alcMakeContextCurrent(pContext);
				bReturn = AL_TRUE;
			}
			else
			{
				alcCloseDevice(pDevice);
			}
		}

		delete pDeviceList;
	}

	return bReturn;
}

ALboolean ALFWShutdownOpenAL()
{
	ALCcontext *pContext;
	ALCdevice *pDevice;

	pContext = alcGetCurrentContext();
	pDevice = alcGetContextsDevice(pContext);
	
	alcMakeContextCurrent(NULL);
	alcDestroyContext(pContext);
	alcCloseDevice(pDevice);

	return AL_TRUE;
}

bool ALFWGetWaveInfo(const char* szWaveFile, ALenum* format, ALsizei* freq, ALchar** data, ALsizei* size)
{
	WAVEID WaveID;

	if (!g_pWaveLoader) return AL_FALSE;
	if (FAILED(g_pWaveLoader->LoadWaveFile(szWaveFile, &WaveID))) return AL_FALSE;

	ALboolean success = AL_FALSE;
	if (SUCCEEDED(g_pWaveLoader->GetWaveSize(WaveID, (unsigned long*)size)) &&
		SUCCEEDED(g_pWaveLoader->GetWaveData(WaveID, (void**)data)) &&
		SUCCEEDED(g_pWaveLoader->GetWaveFrequency(WaveID, (unsigned long*)freq)) &&
		SUCCEEDED(g_pWaveLoader->GetWaveALBufferFormat(WaveID, &alGetEnumValue, (unsigned long*)format)))
	{
		success = AL_TRUE;
	}

	g_pWaveLoader->DeleteWaveFile(WaveID);
	return success;
}

// 只返回info，不保存全局变量
ALboolean ALFWGetWaveInfo(const char* szWaveFile, WaveInfo& info)
{
	// 获取文件扩展名
	const char* ext = strrchr(szWaveFile, '.');
	if (!ext) return AL_FALSE;
	ext++; // 跳过点号

	// 根据扩展名分流
	if (strcmp(ext, "wav") == 0)
	{
		// ---------- 原有 WAV 加载逻辑 ----------
		WAVEID WaveID;
		ALint size, freq;
		ALenum format;
		ALchar* rawData;

		if (!g_pWaveLoader) return AL_FALSE;
		if (FAILED(g_pWaveLoader->LoadWaveFile(szWaveFile, &WaveID))) return AL_FALSE;

		ALboolean success = AL_FALSE;
		if (SUCCEEDED(g_pWaveLoader->GetWaveSize(WaveID, (unsigned long*)&size)) &&
			SUCCEEDED(g_pWaveLoader->GetWaveData(WaveID, (void**)&rawData)) &&
			SUCCEEDED(g_pWaveLoader->GetWaveFrequency(WaveID, (unsigned long*)&freq)) &&
			SUCCEEDED(g_pWaveLoader->GetWaveALBufferFormat(WaveID, &alGetEnumValue, (unsigned long*)&format)))
		{
			info.format = format;
			info.freq = freq;
			info.data.assign(rawData, rawData + size);
			success = AL_TRUE;
		}

		g_pWaveLoader->DeleteWaveFile(WaveID);
		return success;
	}
	else if (strcmp(ext, "ogg") == 0)
	{
		// ---------- OGG 解码 ----------
		int channels, sampleRate;
		short* audioData = nullptr;
		int sampleCount = stb_vorbis_decode_filename(szWaveFile, &channels, &sampleRate, &audioData);

		if (!audioData) // 解码失败（文件不存在、格式不对等）
			return AL_FALSE;

		// 确定 OpenAL 格式
		ALenum format = (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

		info.format = format;
		info.freq = sampleRate;
		// 将 PCM 数据拷贝到 vector（注意类型转换）
		info.data.assign(
			reinterpret_cast<ALchar*>(audioData),
			reinterpret_cast<ALchar*>(audioData) + sampleCount * sizeof(short)
		);

		free(audioData);  // 释放解码库分配的内存
		return AL_TRUE;
	}
	else if (strcmp(ext, "mp3") == 0)
	{
		// ---------- MP3 解码 ----------
		mp3dec_t mp3d;
		mp3dec_init(&mp3d);

		mp3dec_file_info_t mp3info;
		if (mp3dec_load(&mp3d, szWaveFile, &mp3info, NULL, NULL)) // 解码失败
			return AL_FALSE;

		// 确定 OpenAL 格式（minimp3 输出为 16-bit PCM）
		ALenum format = (mp3info.channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

		info.format = format;
		info.freq = mp3info.hz;
		info.data.assign(
			reinterpret_cast<ALchar*>(mp3info.buffer),
			reinterpret_cast<ALchar*>(mp3info.buffer) + mp3info.samples * sizeof(mp3d_sample_t)
		);

		free(mp3info.buffer);  // 释放解码库分配的内存
		return AL_TRUE;
	}
	else // 不支持的格式
		return AL_FALSE;
}

// 分割函数，传入info，内部调用了alBufferData上传数据，无须手动调用，返回AL_FALSE代表超出音频范围
ALboolean ALFWSplitDataToBuffer(ALuint dstBuffer, const WaveInfo& srcInfo, int startSample, int numSamples)
{
	// 计算每帧字节数
	int channels = (srcInfo.format == AL_FORMAT_STEREO16 || srcInfo.format == AL_FORMAT_STEREO8) ? 2 : 1;
	int bytesPerSample = (srcInfo.format == AL_FORMAT_STEREO16 || srcInfo.format == AL_FORMAT_MONO16) ? 2 : 1;
	int bytesPerFrame = channels * bytesPerSample;

	int startByte = startSample * bytesPerFrame;
	int lengthBytes = numSamples * bytesPerFrame;

	if (startByte + lengthBytes > srcInfo.data.size())
		return AL_FALSE;

	alBufferData(dstBuffer, srcInfo.format, srcInfo.data.data() + startByte, lengthBytes, srcInfo.freq);

	return AL_TRUE;
}

ALboolean ALFWLoadWaveToBuffer(const char* szWaveFile, ALuint uiBufferID, ALenum eXRAMBufferMode)
{
	// 使用通用的多格式解码函数获取数据
	WaveInfo info;
	if (!ALFWGetWaveInfo(szWaveFile, info))
		return AL_FALSE; // 文件不兼容或解码失败

	// 设置 XRAM 模式（如果应用程序启用了该扩展）
	if (eaxSetBufferMode && eXRAMBufferMode)
		eaxSetBufferMode(1, &uiBufferID, eXRAMBufferMode);

	// 上传数据到 OpenAL 缓冲区
	alGetError();
	alBufferData(uiBufferID, info.format, info.data.data(), (ALsizei)info.data.size(), info.freq);
	if (alGetError() != AL_NO_ERROR)
		return AL_FALSE;

	return AL_TRUE;
}

void ALFWprintf( const char* x, ... )
{
    va_list args;
    va_start( args, x );
    vprintf( x, args ); 
    va_end( args );
}

ALchar fullPath[_MAX_PATH];
ALchar *ALFWaddMediaPath(const ALchar *filename)
{
	sprintf_s(fullPath, "%s%s", "..\\..\\Media\\", filename);
	return fullPath;
}

ALint ALFWKeyPress(void)
{
	return _kbhit();
}

// ScriptExtension Queries

ALboolean ALFWIsXRAMSupported()
{
	ALboolean bXRAM = AL_FALSE;
	
	if (alIsExtensionPresent("EAX-RAM") == AL_TRUE)
	{
		// Get X-RAM Function pointers
		eaxSetBufferMode = (EAXSetBufferMode)alGetProcAddress("EAXSetBufferMode");
		eaxGetBufferMode = (EAXGetBufferMode)alGetProcAddress("EAXGetBufferMode");

		if (eaxSetBufferMode && eaxGetBufferMode)
		{
			eXRAMSize = alGetEnumValue("AL_EAX_RAM_SIZE");
			eXRAMFree = alGetEnumValue("AL_EAX_RAM_FREE");
			eXRAMAuto = alGetEnumValue("AL_STORAGE_AUTOMATIC");
			eXRAMHardware = alGetEnumValue("AL_STORAGE_HARDWARE");
			eXRAMAccessible = alGetEnumValue("AL_STORAGE_ACCESSIBLE");

			if (eXRAMSize && eXRAMFree && eXRAMAuto && eXRAMHardware && eXRAMAccessible)
				bXRAM = AL_TRUE;
		}
	}

	return bXRAM;
}

ALboolean ALFWIsEFXSupported()
{
	ALCdevice *pDevice = NULL;
	ALCcontext *pContext = NULL;
	ALboolean bEFXSupport = AL_FALSE;

	pContext = alcGetCurrentContext();
	pDevice = alcGetContextsDevice(pContext);

	if (alcIsExtensionPresent(pDevice, (ALCchar*)ALC_EXT_EFX_NAME))
	{
		// Get function pointers
		alGenEffects = (LPALGENEFFECTS)alGetProcAddress("alGenEffects");
		alDeleteEffects = (LPALDELETEEFFECTS )alGetProcAddress("alDeleteEffects");
		alIsEffect = (LPALISEFFECT )alGetProcAddress("alIsEffect");
		alEffecti = (LPALEFFECTI)alGetProcAddress("alEffecti");
		alEffectiv = (LPALEFFECTIV)alGetProcAddress("alEffectiv");
		alEffectf = (LPALEFFECTF)alGetProcAddress("alEffectf");
		alEffectfv = (LPALEFFECTFV)alGetProcAddress("alEffectfv");
		alGetEffecti = (LPALGETEFFECTI)alGetProcAddress("alGetEffecti");
		alGetEffectiv = (LPALGETEFFECTIV)alGetProcAddress("alGetEffectiv");
		alGetEffectf = (LPALGETEFFECTF)alGetProcAddress("alGetEffectf");
		alGetEffectfv = (LPALGETEFFECTFV)alGetProcAddress("alGetEffectfv");
		alGenFilters = (LPALGENFILTERS)alGetProcAddress("alGenFilters");
		alDeleteFilters = (LPALDELETEFILTERS)alGetProcAddress("alDeleteFilters");
		alIsFilter = (LPALISFILTER)alGetProcAddress("alIsFilter");
		alFilteri = (LPALFILTERI)alGetProcAddress("alFilteri");
		alFilteriv = (LPALFILTERIV)alGetProcAddress("alFilteriv");
		alFilterf = (LPALFILTERF)alGetProcAddress("alFilterf");
		alFilterfv = (LPALFILTERFV)alGetProcAddress("alFilterfv");
		alGetFilteri = (LPALGETFILTERI )alGetProcAddress("alGetFilteri");
		alGetFilteriv= (LPALGETFILTERIV )alGetProcAddress("alGetFilteriv");
		alGetFilterf = (LPALGETFILTERF )alGetProcAddress("alGetFilterf");
		alGetFilterfv= (LPALGETFILTERFV )alGetProcAddress("alGetFilterfv");
		alGenAuxiliaryEffectSlots = (LPALGENAUXILIARYEFFECTSLOTS)alGetProcAddress("alGenAuxiliaryEffectSlots");
		alDeleteAuxiliaryEffectSlots = (LPALDELETEAUXILIARYEFFECTSLOTS)alGetProcAddress("alDeleteAuxiliaryEffectSlots");
		alIsAuxiliaryEffectSlot = (LPALISAUXILIARYEFFECTSLOT)alGetProcAddress("alIsAuxiliaryEffectSlot");
		alAuxiliaryEffectSloti = (LPALAUXILIARYEFFECTSLOTI)alGetProcAddress("alAuxiliaryEffectSloti");
		alAuxiliaryEffectSlotiv = (LPALAUXILIARYEFFECTSLOTIV)alGetProcAddress("alAuxiliaryEffectSlotiv");
		alAuxiliaryEffectSlotf = (LPALAUXILIARYEFFECTSLOTF)alGetProcAddress("alAuxiliaryEffectSlotf");
		alAuxiliaryEffectSlotfv = (LPALAUXILIARYEFFECTSLOTFV)alGetProcAddress("alAuxiliaryEffectSlotfv");
		alGetAuxiliaryEffectSloti = (LPALGETAUXILIARYEFFECTSLOTI)alGetProcAddress("alGetAuxiliaryEffectSloti");
		alGetAuxiliaryEffectSlotiv = (LPALGETAUXILIARYEFFECTSLOTIV)alGetProcAddress("alGetAuxiliaryEffectSlotiv");
		alGetAuxiliaryEffectSlotf = (LPALGETAUXILIARYEFFECTSLOTF)alGetProcAddress("alGetAuxiliaryEffectSlotf");
		alGetAuxiliaryEffectSlotfv = (LPALGETAUXILIARYEFFECTSLOTFV)alGetProcAddress("alGetAuxiliaryEffectSlotfv");

		if (alGenEffects &&	alDeleteEffects && alIsEffect && alEffecti && alEffectiv &&	alEffectf &&
			alEffectfv && alGetEffecti && alGetEffectiv && alGetEffectf && alGetEffectfv &&	alGenFilters &&
			alDeleteFilters && alIsFilter && alFilteri && alFilteriv &&	alFilterf && alFilterfv &&
			alGetFilteri &&	alGetFilteriv && alGetFilterf && alGetFilterfv && alGenAuxiliaryEffectSlots &&
			alDeleteAuxiliaryEffectSlots &&	alIsAuxiliaryEffectSlot && alAuxiliaryEffectSloti &&
			alAuxiliaryEffectSlotiv && alAuxiliaryEffectSlotf && alAuxiliaryEffectSlotfv &&
			alGetAuxiliaryEffectSloti && alGetAuxiliaryEffectSlotiv && alGetAuxiliaryEffectSlotf &&
			alGetAuxiliaryEffectSlotfv)
			bEFXSupport = AL_TRUE;
	}

	return bEFXSupport;
}
