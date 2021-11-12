#pragma once
#include <xaudio2.h>
#include <array>
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'

#define FILE_PATH_PATTERN "../../../sounds/#.wav"

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
	InvaderSoundDevice();
	~InvaderSoundDevice();
	void playSound(InvaderSoundDevice::sfx whichSoundEffect);
private:
	static const int SFX_COUNT = 9;
	IXAudio2* engine;
	IXAudio2MasteringVoice* masterVoice;
	std::array<IXAudio2SourceVoice*, SFX_COUNT> sourceVoices;
	std::array<XAUDIO2_BUFFER, SFX_COUNT> buffers;
	void loadSounds();
	HRESULT ReadChunkData(HANDLE hFile, void* buffer, DWORD buffersize, DWORD bufferoffset);
	HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition);
};