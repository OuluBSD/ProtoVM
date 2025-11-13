#ifndef WAV_WRITER_H
#define WAV_WRITER_H

#include <fstream>
#include <string>
#include <cstdint>

struct WavHeader {
    char riff[4];           // "RIFF"
    uint32_t chunkSize;     // 36 + SubChunk2Size
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    uint32_t subchunk1Size; // 16 for PCM
    uint16_t audioFormat;   // 1 for PCM
    uint16_t numChannels;   // 1 for mono, 2 for stereo
    uint32_t sampleRate;    // 44100, etc.
    uint32_t byteRate;      // SampleRate * NumChannels * BitsPerSample/8
    uint16_t blockAlign;    // NumChannels * BitsPerSample/8
    uint16_t bitsPerSample; // 16, 24, 32, etc.
    char data[4];           // "data"
    uint32_t subchunk2Size; // Data size in bytes
};

class WavWriter {
public:
    WavWriter();
    ~WavWriter();
    
    bool open(const std::string& filename, int sampleRate = 44100, int channels = 1, int bitsPerSample = 16);
    void writeSample(float sample); // For 16-bit samples
    void writeSamples(const float* samples, int numSamples);
    bool close();
    
    bool isOpen() const { return file.is_open(); }
    
private:
    std::ofstream file;
    WavHeader header;
    int sampleCount;
    int channels;
    int bitsPerSample;
};

#endif // WAV_WRITER_H