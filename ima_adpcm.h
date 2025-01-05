#pragma once
#include <vector>

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

// taken from ALSA
static std::vector<char> EncodeImaAdpcm(std::vector<uint8_t>& rawData)
{
    auto conv = [](const std::vector<uint8_t>& rawBytes) -> std::vector<int16_t> {
        std::vector<int16_t> pcmSamples;
        for (size_t i = 0; i < rawBytes.size(); i += 2) {
            pcmSamples.push_back((int16_t)rawBytes[i] | (rawBytes[i + 1] << 8));
        }
        return pcmSamples;
    };
    std::vector<int16_t> samples = conv(rawData);

    std::vector<char> outBuff;
    ImaAdpcmState state;
    for (auto sl : samples) {
        short diff;		/* Difference between sl and predicted sample */
        short pred_diff;	/* Predicted difference to next sample */
        unsigned char sign;	/* sign of diff */
        short step;		/* holds previous StepSize value */
        unsigned char adjust_idx;	/* Index to IndexAdjust lookup table */
        int i;
        /* Compute difference to previous predicted value */
        diff = sl - state.valprev;
        sign = (diff < 0) ? 0x8 : 0x0;
        if (sign) {
            diff = -diff;
        }
        /*
         * This code *approximately* computes:
         *    adjust_idx = diff * 4 / step;
         *    pred_diff = (adjust_idx + 0.5) * step / 4;
         *
         * But in shift step bits are dropped. The net result of this is
         * that even if you have fast mul/div hardware you cannot put it to
         * good use since the fix-up would be too expensive.
         */
        step = stepsizeTable[state.index];
        /* Divide and clamp */
        pred_diff = step >> 3;
        for (adjust_idx = 0, i = 0x4; i; i >>= 1, step >>= 1) {
            if (diff >= step) {
                adjust_idx |= i;
                diff -= step;
                pred_diff += step;
            }
        }
        /* Update and clamp previous predicted value */
        state.valprev += sign ? -pred_diff : pred_diff;
        if (state.valprev > 32767) {
            state.valprev = 32767;
        }
        else if (state.valprev < -32768) {
            state.valprev = -32768;
        }
        /* Update and clamp StepSize lookup table index */
        state.index += indexTable[adjust_idx];
        if (state.index < 0) {
            state.index = 0;
        }
        else if (state.index > 88) {
            state.index = 88;
        }
        int8_t b = sign | adjust_idx;
        outBuff.push_back(b);
    }
    return outBuff;
}

std::vector<int16_t> DecodeImaAdpcm(const std::vector<uint8_t>& samples)
{
    std::vector<int16_t> outBuff;
    ImaAdpcmState state;

    for (auto code : samples) {
        short pred_diff;	/* Predicted difference to next sample */
        short step;		/* holds previous StepSize value */
        char sign;
        /* Separate sign and magnitude */
        sign = code & 0x8;
        code &= 0x7;
        /*
         * Computes pred_diff = (code + 0.5) * step / 4,
         * but see comment in adpcm_coder.
         */
        step = stepsizeTable[state.index];
        /* Compute difference and new predicted value */
        pred_diff = step >> 3;
        for (int i = 0x4; i; i >>= 1, step >>= 1) {
            if (code & i) {
                pred_diff += step;
            }
        }
        state.valprev += (sign) ? -pred_diff : pred_diff;
        /* Clamp output value */
        if (state.valprev > 32767) {
            state.valprev = 32767;
        }
        else if (state.valprev < -32768) {
            state.valprev = -32768;
        }
        /* Find new StepSize index value */
        state.index += indexTable[code];
        if (state.index < 0) {
            state.index = 0;
        }
        else if (state.index > 88) {
            state.index = 88;
        }
        int16_t val = static_cast<int16_t>(state.valprev);
        outBuff.push_back(val);
    }
    return outBuff;
}
