#include <AL/al.h>
#include <AL/alc.h>
#include <vector>
#include <cstring>

class OpenALSound {
public:
    OpenALSound(const unsigned char* data, unsigned int len, int freq)
        : source(0), buffer(0), device(nullptr), context(nullptr) {
        device = alcOpenDevice(nullptr);
        if (!device) return;

        context = alcCreateContext(device, nullptr);
        if (!context || !alcMakeContextCurrent(context)) return;

        alGenBuffers(1, &buffer);
        alGenSources(1, &source);

        // Supone formato WAV PCM 16-bit mono o estéreo
        // Asegúrate de que el archivo generado con `xxd -i` sea un WAV válido
        // y omitir cabecera para extraer metadata real es necesario si es raw

        // Aquí usamos todo el bloque como si fuera WAV PCM mono 16-bit a 44100Hz
        alBufferData(buffer, AL_FORMAT_MONO16, data, len, freq);
        alSourcei(source, AL_BUFFER, buffer);
    }

    ~OpenALSound() {
        alDeleteSources(1, &source);
        alDeleteBuffers(1, &buffer);
        alcMakeContextCurrent(nullptr);
        if (context) alcDestroyContext(context);
        if (device) alcCloseDevice(device);
    }

    void play() {
        alSourcePlay(source);
    }

    void pause() {
        alSourcePause(source);
    }

    void stop() {
        alSourceStop(source);
    }

    bool isPlaying() const {
        ALint state;
        alGetSourcei(source, AL_SOURCE_STATE, &state);
        return state == AL_PLAYING;
    }

private:
    ALCdevice* device;
    ALCcontext* context;
    ALuint buffer;
    ALuint source;
};