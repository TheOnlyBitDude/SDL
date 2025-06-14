// OpenALManager.hpp
#pragma once
#include <AL/al.h>
#include <AL/alc.h>

class OpenALManager {
public:
    static OpenALManager& getInstance() {
        static OpenALManager instance;
        return instance;
    }

    ALCdevice* getDevice() const { return device; }
    ALCcontext* getContext() const { return context; }

private:
    ALCdevice* device;
    ALCcontext* context;

    OpenALManager() {
        device = alcOpenDevice(nullptr);
        context = alcCreateContext(device, nullptr);
        alcMakeContextCurrent(context);
    }

    ~OpenALManager() {
        alcMakeContextCurrent(nullptr);
        alcDestroyContext(context);
        alcCloseDevice(device);
    }

    OpenALManager(const OpenALManager&) = delete;
    void operator=(const OpenALManager&) = delete;
};
