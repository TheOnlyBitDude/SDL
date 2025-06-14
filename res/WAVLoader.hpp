#pragma once
#include <cstdint>
#include <cstring>
#include <stdexcept>

struct WAVData {
    const unsigned char* audioData;
    uint32_t audioDataSize;
    ALenum format;
    uint32_t frequency;
};

inline WAVData load_wav_from_memory(const unsigned char* data, size_t len) {
    if (len < 44) throw std::runtime_error("Archivo WAV demasiado pequeño.");

    uint16_t channels = *reinterpret_cast<const uint16_t*>(data + 22);
    uint32_t sampleRate = *reinterpret_cast<const uint32_t*>(data + 24);
    uint16_t bitsPerSample = *reinterpret_cast<const uint16_t*>(data + 34);

    // Determinar formato de OpenAL
    ALenum format = 0;
    if (channels == 1 && bitsPerSample == 8) format = AL_FORMAT_MONO8;
    else if (channels == 1 && bitsPerSample == 16) format = AL_FORMAT_MONO16;
    else if (channels == 2 && bitsPerSample == 8) format = AL_FORMAT_STEREO8;
    else if (channels == 2 && bitsPerSample == 16) format = AL_FORMAT_STEREO16;
    else throw std::runtime_error("Formato WAV no compatible con OpenAL.");

    // Buscar "data" chunk
    size_t pos = 12;
    while (pos + 8 <= len) {
        uint32_t chunkID = *reinterpret_cast<const uint32_t*>(data + pos);
        uint32_t chunkSize = *reinterpret_cast<const uint32_t*>(data + pos + 4);

        if (chunkID == 0x61746164) { // "data"
            const unsigned char* audio = data + pos + 8;
            if (pos + 8 + chunkSize > len) throw std::runtime_error("Tamaño del chunk de audio inválido.");
            return WAVData{audio, chunkSize, format, sampleRate};
        }

        pos += 8 + chunkSize;
    }

    throw std::runtime_error("Chunk 'data' no encontrado.");
}
