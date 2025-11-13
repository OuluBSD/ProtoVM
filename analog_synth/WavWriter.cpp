#include "WavWriter.h"
#include <cmath>
#include <cstring>

WavWriter::WavWriter() : sampleCount(0), channels(1), bitsPerSample(16) {
    // Initialize header with default values
    memcpy(header.riff, "RIFF", 4);
    memcpy(header.wave, "WAVE", 4);
    memcpy(header.fmt, "fmt ", 4);
    memcpy(header.data, "data", 4);
    
    header.subchunk1Size = 16;  // PCM format
    header.audioFormat = 1;     // PCM
    header.numChannels = 1;
    header.sampleRate = 44100;
    header.bitsPerSample = 16;
    header.byteRate = header.sampleRate * header.numChannels * header.bitsPerSample / 8;
    header.blockAlign = header.numChannels * header.bitsPerSample / 8;
    header.subchunk2Size = 0;   // Will be updated when closing
    header.chunkSize = 36 + header.subchunk2Size;  // Will be updated when closing
}

WavWriter::~WavWriter() {
    if (isOpen()) {
        close();
    }
}

bool WavWriter::open(const std::string& filename, int sampleRate, int ch, int bits) {
    file.open(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    // Update header with provided parameters
    header.sampleRate = sampleRate;
    header.numChannels = ch;
    header.bitsPerSample = bits;
    header.byteRate = header.sampleRate * header.numChannels * header.bitsPerSample / 8;
    header.blockAlign = header.numChannels * header.bitsPerSample / 8;
    
    // Write header to file (with placeholder sizes)
    file.write(reinterpret_cast<const char*>(&header), sizeof(WavHeader));
    
    sampleCount = 0;
    channels = ch;
    bitsPerSample = bits;
    
    return true;
}

void WavWriter::writeSample(float sample) {
    if (!isOpen()) return;
    
    // Clamp sample to [-1.0, 1.0]
    if (sample > 1.0f) sample = 1.0f;
    else if (sample < -1.0f) sample = -1.0f;
    
    // Convert to integer based on bits per sample
    if (bitsPerSample == 16) {
        int16_t intSample = static_cast<int16_t>(sample * 32767.0f);
        file.write(reinterpret_cast<const char*>(&intSample), sizeof(int16_t));
    } else if (bitsPerSample == 32) {
        int32_t intSample = static_cast<int32_t>(sample * 2147483647.0f);
        file.write(reinterpret_cast<const char*>(&intSample), sizeof(int32_t));
    }
    
    sampleCount++;
}

void WavWriter::writeSamples(const float* samples, int numSamples) {
    for (int i = 0; i < numSamples; i++) {
        writeSample(samples[i]);
    }
}

bool WavWriter::close() {
    if (!isOpen()) {
        return false;
    }
    
    // Calculate final sizes
    header.subchunk2Size = sampleCount * channels * bitsPerSample / 8;
    header.chunkSize = 36 + header.subchunk2Size;
    
    // Save current position
    auto pos = file.tellp();
    
    // Go back to beginning to rewrite header with correct sizes
    file.seekp(0);
    file.write(reinterpret_cast<const char*>(&header), sizeof(WavHeader));
    
    // Go back to end
    file.seekp(pos);
    
    file.close();
    return true;
}