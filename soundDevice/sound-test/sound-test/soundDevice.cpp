/*
 * Audio Engine for SPace Invaders Game
 */

#include "soundDevice.h"
#include <xaudio2.h>
#include <Windows.h>



InvaderSoundDevice::InvaderSoundDevice()
{
	HRESULT hr;
	hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	if (FAILED(hr))
		throw "failure initiailizing COM";
	// create engine instance
	engine = nullptr;
	if (FAILED(hr = XAudio2Create(&engine, 0, XAUDIO2_DEFAULT_PROCESSOR)))
		throw "failure creating xaudio2 engine";
	masterVoice = nullptr;
	if (FAILED(hr = engine->CreateMasteringVoice(&masterVoice)))
		throw "failure creating mastering voice";
	this->loadSounds();
}

void InvaderSoundDevice::loadSounds()
{
	HRESULT hr;
	// open the sound file
	HANDLE soundFile;
	char filePath[] = "../../../sounds/1.wav";
	soundFile = CreateFileA(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (soundFile == INVALID_HANDLE_VALUE)
		throw "error opening audio file";
	if (INVALID_SET_FILE_POINTER == SetFilePointer(soundFile, 0, NULL, FILE_BEGIN))
		throw "error setting file pointer";

	// structures to populate
	WAVEFORMATEXTENSIBLE wfx = { 0 };
	buffer = { 0 };

	// checking RIFF chunk for file type
	DWORD dwChunkSize;
	DWORD dwChunkPosition;
	//check the file type, should be fourccWAVE or 'XWMA'
	FindChunk(soundFile, fourccRIFF, dwChunkSize, dwChunkPosition);
	DWORD filetype;
	ReadChunkData(soundFile, &filetype, sizeof(DWORD), dwChunkPosition);
	if (filetype != fourccWAVE)
		throw "bad audio file type";

	// copy fmt schunk int WAVEFORMATEXTENSIBLE
	FindChunk(soundFile, fourccFMT, dwChunkSize, dwChunkPosition);
	ReadChunkData(soundFile, &wfx, dwChunkSize, dwChunkPosition);

	//fill out the audio data buffer with the contents of the fourccDATA chunk
	FindChunk(soundFile, fourccDATA, dwChunkSize, dwChunkPosition);
	BYTE* pDataBuffer = new BYTE[dwChunkSize];
	ReadChunkData(soundFile, pDataBuffer, dwChunkSize, dwChunkPosition);

	buffer.AudioBytes = dwChunkSize;  //size of the audio buffer in bytes
	buffer.pAudioData = pDataBuffer;  //buffer containing audio data
	buffer.Flags = XAUDIO2_END_OF_STREAM; // tell the source voice not to expect any data after this buffer

	// create source voice
	sourceVoice = nullptr;
	if (FAILED(hr = engine->CreateSourceVoice(&sourceVoice, (WAVEFORMATEX*)& wfx)))
		throw "could not create source voice";
	if (FAILED(hr = sourceVoice->SubmitSourceBuffer(&buffer)))
		throw "could not submit audio source buffer";
}

void InvaderSoundDevice::playSound()
{
	sourceVoice->Stop();
	HRESULT hr;
	if (FAILED(hr = sourceVoice->SubmitSourceBuffer(&buffer)))
		throw "could not submit audio source buffer";
	sourceVoice->Start(0);
}

InvaderSoundDevice::~InvaderSoundDevice()
{
	engine->Release();
}


// https://docs.microsoft.com/en-us/windows/win32/xaudio2/how-to--load-audio-data-files-in-xaudio2
HRESULT InvaderSoundDevice::FindChunk(HANDLE hFile, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition)
{
	HRESULT hr = S_OK;
	if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN))
		return HRESULT_FROM_WIN32(GetLastError());

	DWORD dwChunkType;
	DWORD dwChunkDataSize;
	DWORD dwRIFFDataSize = 0;
	DWORD dwFileType;
	DWORD bytesRead = 0;
	DWORD dwOffset = 0;

	while (hr == S_OK)
	{
		DWORD dwRead;
		if (0 == ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL))
			hr = HRESULT_FROM_WIN32(GetLastError());

		if (0 == ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL))
			hr = HRESULT_FROM_WIN32(GetLastError());

		switch (dwChunkType)
		{
		case fourccRIFF:
			dwRIFFDataSize = dwChunkDataSize;
			dwChunkDataSize = 4;
			if (0 == ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL))
				hr = HRESULT_FROM_WIN32(GetLastError());
			break;

		default:
			if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT))
				return HRESULT_FROM_WIN32(GetLastError());
		}

		dwOffset += sizeof(DWORD) * 2;

		if (dwChunkType == fourcc)
		{
			dwChunkSize = dwChunkDataSize;
			dwChunkDataPosition = dwOffset;
			return S_OK;
		}

		dwOffset += dwChunkDataSize;

		if (bytesRead >= dwRIFFDataSize) return S_FALSE;

	}

	return S_OK;

}

HRESULT InvaderSoundDevice::ReadChunkData(HANDLE hFile, void* buffer, DWORD buffersize, DWORD bufferoffset)
{
	HRESULT hr = S_OK;
	if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, bufferoffset, NULL, FILE_BEGIN))
		return HRESULT_FROM_WIN32(GetLastError());
	DWORD dwRead;
	if (0 == ReadFile(hFile, buffer, buffersize, &dwRead, NULL))
		hr = HRESULT_FROM_WIN32(GetLastError());
	return hr;
}