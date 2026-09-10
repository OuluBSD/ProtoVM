#ifndef WAVEWRITER_H
#define WAVEWRITER_H

#include <string>
#include <fstream>
#include <vector>
#include <cstdint>

class WaveWriter {
public:
    struct WavHeader {
        char riff_chunk_id[4] = {'R', 'I', 'F', 'F'};
        uint32_t riff_chunk_size = 0;
        char wave_format[8] = {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '};
        uint32_t fmt_chunk_size = 16;
        uint16_t audio_format = 1; // PCM
        uint16_t num_channels = 2; // Stereo
        uint32_t sample_rate = 44100;
        uint32_t byte_rate = 0;
        uint16_t block_align = 0;
        uint16_t bits_per_sample = 16;
        char data_chunk_id[4] = {'d', 'a', 't', 'a'};
        uint32_t data_chunk_size = 0;
    };

    WaveWriter();
    ~WaveWriter();

    bool Open(const std::string& filename);
    bool Write(const std::vector<float>& left, const std::vector<float>& right, uint32_t sample_rate);
    void Close();

private:
    std::ofstream file_;
    std::string filename_;
    bool is_open_;
};

#endif // WAVEWRITER_H