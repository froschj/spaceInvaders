#pragma once
#include <xaudio2.h>
#include <array>
#include <string>
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'


class InvaderSoundDevice 
{
public:
	enum class sfx {
		UFO,
		SHOT,
		PLAYER_DEATH,
		INVADER_DEATH,
		FLEET_MOVE_1,
		FLEET_MOVE_2,
		FLEET_MOVE_3,
		FLEET_MOVE_4,
		UFO_HIT
	};
	InvaderSoundDevice(std::string sfxFilePath);
	~InvaderSoundDevice();
	void playSound(InvaderSoundDevice::sfx whichSoundEffect);
private:
	static const int SFX_COUNT = 9;
	IXAudio2* engine;
	IXAudio2MasteringVoice* masterVoice;
	std::array<IXAudio2SourceVoice*, SFX_COUNT> sourceVoices;
	std::array<XAUDIO2_BUFFER, SFX_COUNT> buffers;
	void loadSounds(std::string sfxFilePath);
	HRESULT ReadChunkData(HANDLE hFile, void* buffer, DWORD buffersize, DWORD bufferoffset);
	HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition);
};