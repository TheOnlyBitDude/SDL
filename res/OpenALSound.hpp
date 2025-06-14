#pragma once

#include <AL/al.h>
#include <AL/alc.h>
#include <cstdint>
#include <stdexcept>
#include <cstring>

//
// -------- OpenAL Context Manager --------
// Singleton para inicializar y cerrar OpenAL automáticamente
//
class OpenALManager {
public:
    static OpenALManager& getInstance() {
        static OpenALManager instance;
        return instance;
    }

private:
    ALCdevice* device;
    ALCcontext* context;

    OpenALManager() {
        device = alcOpenDevice(nullptr);
        if (!device) throw std::runtime_error("No se pudo abrir el dispositivo OpenAL.");

        context = alcCreateContext(device, nullptr);
        if (!context || !alcMakeContextCurrent(context))
            throw std::runtime_error("No se pudo crear o activar el contexto OpenAL.");
    }

    ~OpenALManager() {
        alcMakeContextCurrent(nullptr);
        if (context) alcDestroyContext(context);
        if (device) alcCloseDevice(device);
    }

    OpenALManager(const OpenALManager&) = delete;
    void operator=(const OpenALManager&) = delete;
};


//
// -------- WAV Loader desde memoria --------
// Extrae datos de audio PCM de un array generado con xxd -i
//
struct WAVData {
    const unsigned char* audioData;
    uint32_t audioDataSize;
    ALenum format;
    uint32_t frequency;
};

inline WAVData load_wav_from_memory(const unsigned char* data, size_t len) {
    if (len < 44) throw std::runtime_error("Archivo WAV demasiado pequeño.");

    uint16_t channels     = *reinterpret_cast<const uint16_t*>(data + 22);
    uint32_t sampleRate   = *reinterpret_cast<const uint32_t*>(data + 24);
    uint16_t bitsPerSample = *reinterpret_cast<const uint16_t*>(data + 34);

    ALenum format = 0;
    if (channels == 1 && bitsPerSample == 8)       format = AL_FORMAT_MONO8;
    else if (channels == 1 && bitsPerSample == 16) format = AL_FORMAT_MONO16;
    else if (channels == 2 && bitsPerSample == 8)  format = AL_FORMAT_STEREO8;
    else if (channels == 2 && bitsPerSample == 16) format = AL_FORMAT_STEREO16;
    else throw std::runtime_error("Formato WAV no soportado por OpenAL.");

    // Buscar el chunk "data"
    size_t pos = 12;
    while (pos + 8 <= len) {
        uint32_t chunkID   = *reinterpret_cast<const uint32_t*>(data + pos);
        uint32_t chunkSize = *reinterpret_cast<const uint32_t*>(data + pos + 4);

        if (chunkID == 0x61746164) { // 'data'
            const unsigned char* audio = data + pos + 8;
            if (pos + 8 + chunkSize > len) throw std::runtime_error("Chunk 'data' inválido.");
            return WAVData{audio, chunkSize, format, sampleRate};
        }
        pos += 8 + chunkSize;
    }

    throw std::runtime_error("No se encontró el chunk 'data' en el archivo WAV.");
}


//
// -------- Clase de sonido OpenAL --------
// Usa WAVData y OpenALManager para reproducir sonido embebido
//
class OpenALSound {
public:
    OpenALSound(const unsigned char* data, unsigned int len)
        : buffer(0), source(0) {
        OpenALManager::getInstance(); // asegura inicialización

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
