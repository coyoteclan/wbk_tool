#include "wbk.h"

namespace fs = std::filesystem;

int main(int argc, char** argv)
{
    printf("WBK Tool - LemonHaze 2025\n");
    if (argc < 4 || argc > 6) {
        printf("Usage: %s -e|-r <.wbk> <.wav>\n", argv[0]);
        return -1;
    }
    
    bool extract = false;
    int replace_idx = -1;
    if (strstr(argv[1], "-e")) {
        extract = true;
    } else if (strstr(argv[1], "-r")) {
        auto idx = atoi(argv[5]) + 1;
        if (idx > INT_MIN && idx < INT_MAX)
            replace_idx = idx;
        else 
            printf("Invalid replacement index specified!\n");
    } 
    else
        return -1;

    WBK wbk;
    wbk.read(argv[2]);
    
    size_t index = 0;
    for (auto& track : wbk.tracks) {
        WBK::nslWave& entry = wbk.entries[index];
        fs::path output_path = std::string(argv[3]).append(std::to_string(index+1)).append(".wav");
        if (extract)
            WAV::writeWAV(output_path.string(), track, entry.samples_per_second / WBK::GetNumChannels(entry), WBK::GetNumChannels(entry));
        ++index;
    }
    if (extract)
        return 1;

    if (replace_idx > index || replace_idx > wbk.header.num_entries)
        printf("Invalid replacement index specified!\n");
    else
    {
        WAV replacement_wav;
        if (replacement_wav.readWAV(argv[6])) {
            if (wbk.replace(wbk, replace_idx, replacement_wav))
            {
                fs::path path = fs::path(std::string(argv[3])).replace_extension(".new.wbk").string();
                wbk.write(path);
                printf("Replaced index %d and written to %s\n", replace_idx, path.string().c_str());
            }
        }         
    }
    return 1;
}