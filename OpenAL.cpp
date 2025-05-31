#include <AL/al.h>
#include <AL/alc.h>
#include <sndfile.h>
#include <iostream>
#include <vector>
#include <thread>
#include <chrono>

class Sound {
private:
    ALuint buffer, source;
    ALCdevice* device;
    ALCcontext* context;
    bool initialized;

public:
    Sound(const char* filename) : buffer(0), source(0), initialized(false) {
        device = alcOpenDevice(nullptr);
        if (!device) {
            std::cerr << "No se pudo abrir el dispositivo de audio.\n";
            return;
        }

        context = alcCreateContext(device, nullptr);
        alcMakeContextCurrent(context);

        alGenBuffers(1, &buffer);
        alGenSources(1, &source);

        // Cargar el archivo de audio usando libsndfile
        SF_INFO sfinfo;
        SNDFILE* sndfile = sf_open(filename, SFM_READ, &sfinfo);
        if (!sndfile) {
            std::cerr << "No se pudo abrir el archivo de sonido: " << filename << "\n";
            return;
        }

        std::vector<short> samples(sfinfo.frames * sfinfo.channels);
        sf_read_short(sndfile, samples.data(), samples.size());
        sf_close(sndfile);

        ALenum format = (sfinfo.channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
        alBufferData(buffer, format, samples.data(), samples.size() * sizeof(short), sfinfo.samplerate);
        alSourcei(source, AL_BUFFER, buffer);

        initialized = true;
    }

    ~Sound() {
        if (initialized) {
            alDeleteSources(1, &source);
            alDeleteBuffers(1, &buffer);
            alcMakeContextCurrent(nullptr);
            alcDestroyContext(context);
            alcCloseDevice(device);
        }
    }

    void play() {
        if (initialized) alSourcePlay(source);
    }

    void stop() {
        if (initialized) alSourceStop(source);
    }

    bool get_busy() {
        if (!initialized) return false;
        ALint state;
        alGetSourcei(source, AL_SOURCE_STATE, &state);
        return state == AL_PLAYING;
    }
};



int main() {
    Sound sonido("res/snd/smash.wav");
    sonido.play();

    while (sonido.get_busy()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}
