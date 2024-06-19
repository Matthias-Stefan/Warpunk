#include "win32_warpunk_audio.h"

#include <string>

global std::string AudioFiles[] = {
    "W:/Warpunk/assets/audio/HeroLichReady1.wav",
    "W:/Warpunk/assets/audio/HeroLichDeath1.wav",
    "W:/Warpunk/assets/audio/HeroPaladinReady1.wav",
    "W:/Warpunk/assets/audio/MetalMediumBashMetal1.wav",
    "W:/Warpunk/assets/audio/ReviveHuman.wav",
    "W:/Warpunk/assets/audio/7.1auditionOutLeader v2.wav",
    "W:/Warpunk/assets/audio/KeeperOfTheGroveReady1.wav",
    "W:/Warpunk/assets/audio/HeroTaurenChieftainReady1.wav"
};

internal void
LoadAudio(audio_stream *AudioStream)
{
    u32 ChunkIndex = AudioStream->NextPlaybackIndex;
    //Assert(ChunkIndex < MAX_AUDIO_STREAM_BUFFER_COUNT);
    
    if (AudioStream->FileHandlePosition == AudioStream->FileSize)
    {
        return;
    }
    
    AudioStream->State |= AudioStreamState_Buffering;
    size_t RemainingBytes = AudioStream->FileSize - AudioStream->FileHandlePosition;
    //Assert(RemainingBytes >= 0);
    
    size_t ChunkSize = Min(RemainingBytes, AudioStream->PlaybackBufferSize);
    
    u32 Alignment = AudioStream->Alignment;
    u32 Remainder = ChunkSize % Alignment;
    u32 AlignmentAdjustment = 0;
    if (Remainder > 0)
    {
        AlignmentAdjustment = Alignment - Remainder;
    }
    
    if (!AudioStream->PlaybackBuffer[ChunkIndex])
    {
        AudioStream->PlaybackBuffer[ChunkIndex] = (u16 *)VirtualAlloc(0, AudioStream->PlaybackBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
        OutputDebugStringA("Allocated memory: AudioStream->PlaybackBuffer.\n");
    }
    
    size_t BytesRead;
    if (AudioStream->PlaybackBuffer[ChunkIndex])
    {
        fseek(AudioStream->File, AudioStream->FileHandlePosition, SEEK_SET);
        BytesRead = fread(AudioStream->PlaybackBuffer[ChunkIndex], 1, ChunkSize, AudioStream->File);
        //Assert(BytesRead == ChunkSize);
        AudioStream->FileHandlePosition += ChunkSize; 
    }
    BytesRead += AlignmentAdjustment;
    //Assert(AudioStream->FileHandlePosition <= AudioStream->FileSize);
    
    XAUDIO2_BUFFER XAudio2Buffer = {};
    XAudio2Buffer.AudioBytes = BytesRead;
    XAudio2Buffer.pAudioData = (BYTE *)AudioStream->PlaybackBuffer[ChunkIndex];
    if (RemainingBytes-ChunkSize == 0)
    {
        XAudio2Buffer.Flags = XAUDIO2_END_OF_STREAM;
    }
    
    HRESULT SubmitResult = AudioStream->SourceVoice->SubmitSourceBuffer(&XAudio2Buffer);
    //Assert(SUCCEEDED(SubmitResult));
    if (SUCCEEDED(SubmitResult))
    {
        OutputDebugStringA("Source buffer submitted successfully.\n");
    }
    AudioStream->NextPlaybackIndex = (AudioStream->NextPlaybackIndex + 1) % MAX_AUDIO_STREAM_BUFFER_COUNT;  
    
    AudioStream->State &= ~AudioStreamState_Buffering;
}

internal void
StartAudioStream(audio_stream *AudioStream, IXAudio2 *XAudio2)
{
    //Assert(AudioStream->State & AudioStreamState_Free);
    AudioStream->State &= ~AudioStreamState_Free;
    
    AudioStream->State |= audio_stream_state::AudioStreamState_Buffering;
    
    //Assert(AudioStream->FilenameIndex >= 0);
    std::string Filename = AudioFiles[AudioStream->FilenameIndex]; 
    AudioStream->File = fopen(Filename.c_str(), "rb");
    
    u8 *Data = nullptr;
    if (AudioStream->File)
    {
        Data = (u8 *)VirtualAlloc(0, sizeof(wave_sound_info), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
        if (Data)
        {
            fread(Data, 1, sizeof(wave_sound_info), AudioStream->File);
        }
    }
    
    wave_sound_info WaveSoundInfo = {};
    
    WaveSoundInfo.ChunkID = SwapEndian<u32>(ExtractWaveData<u32>(Data));
    //Assert(WaveSoundInfo.ChunkID == RIFF);
    WaveSoundInfo.ChunkSize = ExtractWaveData<u32>(Data);
    WaveSoundInfo.Format = SwapEndian<u32>(ExtractWaveData<u32>(Data));
    //Assert(WaveSoundInfo.Format == WAVE);
    
    // NOTE(matthias): fmt
    WaveSoundInfo.Subchunk1ID = SwapEndian<u32>(ExtractWaveData<u32>(Data));
    //Assert(WaveSoundInfo.Subchunk1ID == FMT);
    WaveSoundInfo.Subchunk1Size = ExtractWaveData<u32>(Data);
    WaveSoundInfo.AudioFormat = ExtractWaveData<u16>(Data);
    WaveSoundInfo.NumChannels = ExtractWaveData<u16>(Data);
    WaveSoundInfo.SampleRate = ExtractWaveData<u32>(Data);
    WaveSoundInfo.ByteRate = ExtractWaveData<u32>(Data);
    
    WaveSoundInfo.BlockAlign = ExtractWaveData<u16>(Data);
    WaveSoundInfo.BitsPerSample = ExtractWaveData<u16>(Data);
    
    /*
    Assert(WaveSoundInfo.ByteRate == (WaveSoundInfo.SampleRate * 
    (WaveSoundInfo.BitsPerSample/8) * 
        WaveSoundInfo.NumChannels));
    */
    
    if (WaveSoundInfo.Subchunk1Size > 16)
    {
        // NOTE(matthias): Extensible format
        WaveSoundInfo.SizeExtension = ExtractWaveData<u16>(Data);
        //Assert(WaveSoundInfo.SizeExtension == 22);
        WaveSoundInfo.ValidBitsPerSample = ExtractWaveData<u16>(Data); 
        WaveSoundInfo.ChannelMask = ExtractWaveData<u32>(Data);
        WaveSoundInfo.SubFormat = ExtractWaveData<GUID>(Data);
    }
    
    // NOTE(matthias): data
    WaveSoundInfo.Subchunk2ID = SwapEndian<u32>(ExtractWaveData<u32>(Data));
    //Assert(WaveSoundInfo.Subchunk2ID == DATA);
    WaveSoundInfo.Subchunk2Size = ExtractWaveData<u32>(Data);
    //Assert(WaveSoundInfo.Subchunk2Size > 0);
    
    VirtualFree(Data, 0, MEM_RELEASE);
    
    
    AudioStream->FileSize = WaveSoundInfo.Subchunk2Size;
    AudioStream->PlaybackBufferSize = AUDIO_STREAM_PLAYBACK_BUFFER_TIME * WaveSoundInfo.ByteRate;
    AudioStream->Alignment = WaveSoundInfo.BlockAlign;
    AudioStream->FileHandlePosition = WAVE_PCM_HEADER_SIZE;
    
    WAVEFORMATEXTENSIBLE WaveFormat = {};
    WaveFormat.Format.wFormatTag = WaveSoundInfo.AudioFormat;
    WaveFormat.Format.nChannels =  WaveSoundInfo.NumChannels;
    WaveFormat.Format.nSamplesPerSec = WaveSoundInfo.SampleRate;
    WaveFormat.Format.nBlockAlign = WaveSoundInfo.BlockAlign;
    WaveFormat.Format.nAvgBytesPerSec = WaveFormat.Format.nSamplesPerSec * WaveFormat.Format.nBlockAlign;
    WaveFormat.Format.wBitsPerSample = WaveSoundInfo.BitsPerSample;
    WaveFormat.Format.cbSize = WaveSoundInfo.SizeExtension;
    
    if (WaveSoundInfo.Subchunk1Size == 40)
    {
        AudioStream->FileHandlePosition = WAVE_EXTENSIBLE_HEADER_SIZE;
        
        WaveFormat.Samples.wValidBitsPerSample = WaveSoundInfo.ValidBitsPerSample;
#if 0
        WaveFormat.Samples.wSamplesPerBlock = 0;
        WaveFormat.Samples.wReserved = 0;
#endif
        WaveFormat.dwChannelMask = WaveSoundInfo.ChannelMask;
        WaveFormat.SubFormat = WaveSoundInfo.SubFormat;
    }
    
    HRESULT CreateSourceVoiceResult = XAudio2->CreateSourceVoice(&AudioStream->SourceVoice, (WAVEFORMATEX *)&WaveFormat,
                                                                 0, XAUDIO2_DEFAULT_FREQ_RATIO, (IXAudio2VoiceCallback *)AudioStream, NULL, NULL);
    
    if (SUCCEEDED(CreateSourceVoiceResult))
    {
        OutputDebugStringA("Created source voice successfully.\n");
        
        for (u32 ChunkIndex = 0; ChunkIndex < MAX_AUDIO_STREAM_BUFFER_COUNT; ++ChunkIndex)
        {
            LoadAudio(AudioStream);
        }
    }
}

internal void 
StopAudioStream(audio_stream *AudioStream)
{
    HRESULT StopSourceVoiceResult = AudioStream->SourceVoice->Stop();
    //Assert(SUCCEEDED(StopSourceVoiceResult));
    if (SUCCEEDED(StopSourceVoiceResult))
    {
        AudioStream->SourceVoice->DestroyVoice();
        if (fclose(AudioStream->File) == 0)
        {
            AudioStream->File = nullptr;
        }
        
        for (u32 Index = 0; Index < MAX_AUDIO_STREAM_BUFFER_COUNT; ++Index)
        {
            if (AudioStream->PlaybackBuffer[Index])
            {
                VirtualFree(AudioStream->PlaybackBuffer[Index], 0, MEM_RELEASE);
            }
        }
    }
    
    *AudioStream = {};
}
