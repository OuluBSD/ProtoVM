#include "WaveWriter.h"
#include <cstring>
#include <algorithm>

WaveWriter::WaveWriter() : is_open_(false) {
}

WaveWriter::~WaveWriter() {
    if (is_open_) {
        Close();
    }
}

bool WaveWriter::Open(const std::string& filename) {
    filename_ = filename;
    file_.open(filename, std::ios::binary);
    
    if (!file_) {
        return false;
    }
    
    is_open_ = true;
    
    // Write a placeholder header - will update with correct sizes later
    WavHeader header;
    file_.write(reinterpret_cast<const char*>(&header), sizeof(header));
    
    return true;
}

bool WaveWriter::Write(const std::vector<float>& left, const std::vector<float>& right, uint32_t sample_rate) {
    if (!is_open_ || left.size() != right.size()) {
        return false;
    }

    // Calculate WAV header values
    WavHeader header;
    header.sample_rate = sample_rate;
    header.byte_rate = sample_rate * header.num_channels * header.bits_per_sample / 8;
    header.block_align = header.num_channels * header.bits_per_sample / 8;
    header.data_chunk_size = static_cast<uint32_t>(left.size()) * header.num_channels * sizeof(int16_t);
    header.riff_chunk_size = sizeof(WavHeader) - 8 + header.data_chunk_size; // minus the RIFF and size fields

    // Interleave the left and right channels as 16-bit samples
    std::vector<int16_t> interleaved;
    interleaved.reserve(left.size() * 2);

    for (size_t i = 0; i < left.size(); ++i) {
        // Convert from float [-1.0, 1.0] to int16_t [-32768, 32767]
        int16_t left_sample = static_cast<int16_t>(std::max(-1.0f, std::min(1.0f, left[i])) * 32767.0f);
        int16_t right_sample = static_cast<int16_t>(std::max(-1.0f, std::min(1.0f, right[i])) * 32767.0f);
        
        interleaved.push_back(left_sample);
        interleaved.push_back(right_sample);
    }

    // Move to beginning and write the updated header
    file_.seekp(0);
    file_.write(reinterpret_cast<const char*>(&header), sizeof(header));

    // Move to end and write the audio data
    file_.seekp(sizeof(header));
    file_.write(reinterpret_cast<const char*>(interleaved.data()), 
                interleaved.size() * sizeof(int16_t));

    return true;
}

void WaveWriter::Close() {
    if (is_open_) {
        file_.close();
        is_open_ = false;
    }
}