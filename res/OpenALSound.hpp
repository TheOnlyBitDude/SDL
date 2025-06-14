#pragma once
#include "OpenALManager.hpp"
#include "WAVLoader.hpp"

class OpenALSound {
public:
    OpenALSound(const unsigned char* data, unsigned int len)
        : buffer(0), source(0) {
        OpenALManager::getInstance();

        WAVData wav = load_wav_from_memory(data, len);

        alGenBuffers(1, &buffer);
        alGenSources(1, &source);

        alBufferData(buffer, wav.format, wav.audioData, wav.audioDataSize, wav.frequency);
        alSourcei(source, AL_BUFFER, buffer);
    }

    ~OpenALSound() {
        alDeleteSources(1, &source);
        alDeleteBuffers(1, &buffer);
    }

    void play(bool loop = false) {
        alSourcei(source, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
        alSourcePlay(source);
    }

    void stop() {
        alSourceStop(source);
    }

    void pause() {
        alSourcePause(source);
    }

    bool isPlaying() const {
        ALint state;
        alGetSourcei(source, AL_SOURCE_STATE, &state);
        return state == AL_PLAYING;
    }

private:
    ALuint buffer;
    ALuint source;
};
