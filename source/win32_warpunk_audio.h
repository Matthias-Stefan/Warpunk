/* ========================================================================
   $File: warpunk_audio.h $
   $Date: 29.11.2023 $
   $Creator: Matthias Stefan $
   ======================================================================== */

#ifndef WARPUNK_AUDIO_H
#define WARPUNK_AUDIO_H

#define X_AUDIO2_CREATE(name) HRESULT WINAPI name(IXAudio2 **ppXAudio2, UINT32 Flags, XAUDIO2_PROCESSOR XAudio2Processor) 
typedef X_AUDIO2_CREATE(x_audio2_create);

#define WAVE_PCM_HEADER_SIZE 44
#define WAVE_EXTENSIBLE_HEADER_SIZE 68

#define MAX_AUDIO_STREAMS 5

// NOTE: Header could be bigger
struct wave_sound_info
{
    // NOTE: RIFF
    u32 ChunkID;
    u32 ChunkSize;
    // NOTE: WAVE
    u32 Format;
    
    // NOTE: fmt
    u32 Subchunk1ID;
    u32 Subchunk1Size; // NOTE: 16, 40
    u16 AudioFormat;
    u16 NumChannels;
    u32 SampleRate;
    u32 ByteRate;
    u16 BlockAlign;
    u16 BitsPerSample;
    
    u16 SizeExtension; // NOTE: Extensible = 22
    // NOTE: Extensible Format
    u16 ValidBitsPerSample; 
    u32 ChannelMask;
    GUID SubFormat;
    
    // NOTE: data
    u32 Subchunk2ID;
    u32 Subchunk2Size;
};

template <typename T>
inline T ExtractWaveData(u8* &Raw)
{
    T Data = *((T *)Raw);
    Raw += sizeof(T);
    return Data;
}

//
// Streaming
//

#define AUDIO_STREAM_PLAYBACK_BUFFER_TIME 5
#define MAX_AUDIO_STREAM_BUFFER_COUNT 3

#define RIFF 0x52494646 // "RIFF" in hexadecimal notation
#define WAVE 0x57415645 // "WAVE" in hexadecimal notation
#define FMT  0x666d7420 // "fmt " in hexadecimal notation
#define DATA 0x64617461 // "data" in hexadecimal notation

enum audio_stream_state : u32
{
    AudioStreamState_Free      = 1 << 0,
    AudioStreamState_Buffering = 1 << 1,
    AudioStreamState_Playing   = 1 << 2,
    AudioStreamState_Paused    = 1 << 3,
    AudioStreamState_Closed    = 1 << 4,
};

struct audio_stream : public IXAudio2VoiceCallback
{
    u32 State = AudioStreamState_Free;
    
    FILE *File;
    s32 FilenameIndex = -1;
    u32 FileSize;
    u32 FileHandlePosition;
    u32 Alignment;
    
    u32 NextPlaybackIndex;
    size_t PlaybackBufferSize;
    u16 *PlaybackBuffer[MAX_AUDIO_STREAM_BUFFER_COUNT];
    IXAudio2SourceVoice *SourceVoice;
    
    void OnStreamEnd()
    { 
        this->State &= ~AudioStreamState_Playing;
        this->State |= AudioStreamState_Closed; 
    } 
    void OnVoiceProcessingPassEnd(){} 
    void OnVoiceProcessingPassStart(UINT32 SamplesRequired){}
    void OnBufferEnd(void *BufferContext){}
    void OnBufferStart(void *BufferContext){} 
    void OnLoopEnd(void *BufferContext){}
    void OnVoiceError(void *BufferContext, HRESULT Error){}
};

#endif //WARPUNK_AUDIO_H
