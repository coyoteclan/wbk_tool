#include <iostream>
#include <fstream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <stdexcept>
#include <algorithm>

struct ImaAdpcmState {
    int valprev = 0;
    int index = 0;
};
const int stepsizeTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143,
    157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544,
    598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878,
    2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358, 5894,
    6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899, 15289, 16818,
    18499, 20350, 22385, 24623, 27086, 29794, 32767
};
const int indexTable[16] = {
    -1, -1, -1, -1, 2, 4, 6, 8,
    -1, -1, -1, -1, 2, 4, 6, 8
};

std::vector<int16_t> DecodeImaAdpcm(const std::vector<uint8_t>& ImaFileData, int numSamples) {
    int inp = 0;            // Input buffer pointer
    std::vector<int16_t> outBuff(numSamples); // Output buffer
    int outIndex = 0;       // Output buffer pointer
    int sign;               // Current ADPCM sign bit
    int delta;              // Current ADPCM output value
    int step;               // Stepsize
    int valpred;            // Predicted value
    int vpdiff;             // Current change to valpred
    int index;              // Current step change index
    int inputbuffer = 0;    // Place to keep next 4-bit value
    bool bufferstep = false; // Toggle between inputbuffer/input

    ImaAdpcmState state;
    valpred = state.valprev;
    index = state.index;
    step = stepsizeTable[index];

    for (; numSamples > 0; numSamples--) {
        // Step 1 - Get the delta value
        if (bufferstep) {
            delta = inputbuffer & 0xF;
        }
        else {
            inputbuffer = ImaFileData[inp++];
            delta = (inputbuffer >> 4) & 0xF;
        }
        bufferstep = !bufferstep;

        // Step 2 - Find new index value (for later)
        index += indexTable[delta];
        index = std::clamp(index, 0, 88);

        // Step 3 - Separate sign and magnitude
        sign = delta & 8;
        delta &= 7;

        // Step 4 - Compute difference and new predicted value
        vpdiff = step >> 3;
        if (delta & 4) vpdiff += step;
        if (delta & 2) vpdiff += step >> 1;
        if (delta & 1) vpdiff += step >> 2;

        if (sign) {
            valpred -= vpdiff;
        }
        else {
            valpred += vpdiff;
        }

        // Step 5 - Clamp output value
        valpred = std::clamp(valpred, (int)INT16_MIN, (int)INT16_MAX);

        // Step 6 - Update step value
        step = stepsizeTable[index];

        // Step 7 - Output value
        outBuff[outIndex++] = static_cast<int16_t>(valpred);
    }

    state.valprev = valpred;
    state.index = index;

    return outBuff;
}

class WBK {
public:
    struct wbk_header_t {
        char magic[8];
        char unk[8];
        int flag;
        int size;
        int sample_data_offs;
        int total_bytes;
        char name[32];
        int num_entries;
        int val5, val6, val7;
        int offs;
        int metadata_offs;
        int offs3;
        int offs4;
        int num;
        int entry_desc_offs;
    };
    struct nslWave
    {
        int hash;
        unsigned char format;
        char field_5;
        unsigned char flags;
        char field_7;
        int num_samples;
        unsigned int num_bytes;
        int field_10;
        int field_14;
        int field_18;
        int compressed_data;
        unsigned __int16 samples_per_second;
        __int16 field_22;
        int unk;
    };

    nslWave wave;
    wbk_header_t hdr;

    int num_channels = 0;
    int bits_per_sample = 0;
    int size = 0;
    int blockAlign = 0;
    
    std::vector <nslWave> entries;
    std::vector<std::vector<int16_t>> samples;

    static int GetAudioChannels(const nslWave& wave) {
        int num_channels = 0;
        if (wave.flags)
            num_channels = (((((wave.flags & 0x55) + ((wave.flags >> 1) & 0x55)) & 0x33)
                + ((((wave.flags & 0x55) + ((wave.flags >> 1) & 0x55)) >> 2) & 0x33)) & 0xF)
            + (((((wave.flags & 0x55) + ((wave.flags >> 1) & 0x55)) & 0x33)
                + ((((wave.flags & 0x55) + ((wave.flags >> 1) & 0x55)) >> 2) & 0x33)) >> 4);
        else
            num_channels = 1;
        return num_channels;
    }

    int GetAudioBytesLen()
    {
        unsigned int tmp_flag = (unsigned __int8)((((wave.flags & 0x55) + ((wave.flags >> 1) & 0x55)) & 0x33)     +
                                (((unsigned __int8)((wave.flags & 0x55) + ((wave.flags >> 1) & 0x55)) >> 2) & 0x33));
        if (wave.format == 1)
        {
            if (wave.flags)
                return ((tmp_flag & 0xF) + (tmp_flag >> 4)) * wave.num_bytes;
            else
                return wave.num_bytes;
        }
        else if (wave.format == 2)
        {
            int bytes = 0;
            if (wave.flags)
                bytes = ((tmp_flag & 0xF) + (tmp_flag >> 4)) * wave.num_bytes;
            else
                bytes = wave.num_bytes;
            return 2 * bytes;
        }
        else
            return wave.num_samples;
    }

    void read(std::string path)
    {
        std::ifstream stream(path, std::ios::binary);
        if (stream.good()) {
            stream.read(reinterpret_cast<char*>(&hdr), sizeof wbk_header_t);
            printf("Bank Name: %s\n", std::string(hdr.name).c_str());

            for (int index = 0; index < hdr.num_entries; ++index) {
                
                stream.seekg(0x100 + (index * sizeof nslWave), std::ios::beg);
                stream.read(reinterpret_cast<char*>(&wave), sizeof nslWave);
                
                num_channels = GetAudioChannels(wave);

                // calc bits per sample & blockAlign
                if (wave.format == 1 || wave.format == 2) {
                    bits_per_sample = 8 * (wave.format != 1) + 8;
                    blockAlign = (num_channels * bits_per_sample) / 8;
                }
                else if (wave.format == 4) {
                    bits_per_sample = 16;
                    blockAlign = (bits_per_sample * num_channels) / 8;
                }
                else if (wave.format == 5) {
                    bits_per_sample = 4;
                    blockAlign = 36 * num_channels;
                }
                else if (wave.format == 7) {
                    bits_per_sample = 16;
                    blockAlign = (bits_per_sample * num_channels) / 8;
                    //auto avg_bytes_per_sec = wave.samples_per_second * (bits_per_sample * num_channels);
                    //printf("avg_bytes_per_sec = %d\n", avg_bytes_per_sec);
                }

                // calc size depending on format
                int fmt_type = wave.format - 4;
                if (fmt_type) {
                    int tmp_type = fmt_type - 1;
                    if (!tmp_type)
                        size = blockAlign * (wave.num_bytes >> 6);
                    else if (tmp_type != 2)
                        size = wave.num_bytes;
                    else
                        size = 4 * wave.num_samples;
                }
                else
                    size = 2 * wave.num_bytes;

                printf("Hash: 0x%08X Format=%d num_samples=%d num_channels=%d rate=%dHz len=%d bps=%d\n", wave.hash, wave.format, wave.num_samples, num_channels, wave.samples_per_second, GetAudioBytesLen(), bits_per_sample);
                
                // PCM(?)
                if (wave.format == 1 || wave.format == 2) {
                    stream.seekg(0x1000, std::ios::beg);
                    if (hdr.size) {
                        std::vector<int16_t> tmp;
                        for (int i = 0; i < hdr.size / 4; ++i) {
                            int16_t sample1, sample2;
                            stream.read(reinterpret_cast<char*>(&sample1), 2);
                            stream.read(reinterpret_cast<char*>(&sample2), 2);
                            tmp.push_back(sample1);
                            tmp.push_back(sample2);
                        }
                        samples.push_back(tmp);
                    }
                }
                // IMA ADPCM
                else if (wave.format == 7)
                {
                    std::vector<uint8_t> bdata;
                    stream.seekg(wave.compressed_data, std::ios::beg);
                    bdata.resize(size);
                    stream.read((char*)bdata.data(), size);
                    samples.push_back(DecodeImaAdpcm(bdata, wave.num_samples));
                }
                // @todo: codecs 5 (BINK) and 4 remaining
                else
                {
                    printf("Unsupported codec (%d)!\n", wave.format);
                }

                
                entries.push_back(wave);
            }
            stream.close();
        }
    }
};

static void writeWAV(const std::string& filename, const std::vector<int16_t>& samples, uint32_t sampleRate, int nchannels = 1) {
    struct WAVHeader {
        char riff[4] = { 'R', 'I', 'F', 'F' };
        uint32_t chunkSize;
        char wave[4] = { 'W', 'A', 'V', 'E' };
        char fmt[4] = { 'f', 'm', 't', ' ' };
        uint32_t subchunk1Size = 16;
        uint16_t audioFormat = 1;
        uint16_t numChannels = 1;
        uint32_t sampleRate;
        uint32_t byteRate;
        uint16_t blockAlign;
        uint16_t bitsPerSample = 16;
        char data[4] = { 'd', 'a', 't', 'a' };
        uint32_t subchunk2Size;
    } header;
    header.sampleRate = sampleRate;
    header.numChannels = nchannels;
    header.bitsPerSample = 16;
    header.blockAlign = header.numChannels * (header.bitsPerSample / 8);
    header.byteRate = header.sampleRate * header.blockAlign;
    header.subchunk2Size = samples.size() * sizeof(int16_t);
    header.chunkSize = 36 + header.subchunk2Size;

    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile) {
        std::cerr << "Failed to open file for writing: " << filename << std::endl;
        return;
    }
    outFile.write(reinterpret_cast<const char*>(&header), sizeof(WAVHeader));
    outFile.write(reinterpret_cast<const char*>(samples.data()), samples.size() * sizeof(int16_t));
    outFile.close();

    std::cout << "WAV file written: " << filename << std::endl;
}

int main(int argc, char** argv)
{
    printf("WBK Tool - LemonHaze 2025\n");
    if (argc != 3) {
        printf("Usage: %s <.wbk> <.wav>\n", argv[0]);
        return -1;
    }

    WBK wbk;
    wbk.read(argv[1]);
    
    size_t index = 0;
    for (auto& bank : wbk.samples) {
        WBK::nslWave& entry = wbk.entries[index];
        std::filesystem::path output_path = std::string(argv[2]).append(std::to_string(index)).append(".wav");
        writeWAV(output_path.string(), bank, entry.samples_per_second, WBK::GetAudioChannels(entry));
        ++index;
    }
    return 1;
}