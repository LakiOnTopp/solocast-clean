#pragma once
#include <algorithm>
#include <array>
#include <cmath>

struct Biquad {
    float b0=1.f,b1=0.f,b2=0.f,a1=0.f,a2=0.f,z1=0.f,z2=0.f;
    float process(float x) {
        float y = b0*x + z1;
        z1 = b1*x - a1*y + z2;
        z2 = b2*x - a2*y;
        return y;
    }
    void reset(){ z1=z2=0.f; }
};

Biquad make_highpass(float fs, float hz, float q=0.70710678f);
Biquad make_peaking(float fs, float hz, float q, float gain_db);
Biquad make_highshelf(float fs, float hz, float gain_db);
Biquad make_lowshelf(float fs, float hz, float gain_db);

struct Envelope {
    float value = 0.f;
    float attack = 0.01f;
    float release = 0.1f;
    float fs = 48000.f;
    void set(float sample_rate, float attack_s, float release_s) {
        fs=sample_rate; attack=attack_s; release=release_s;
    }
    float process(float x) {
        x = std::fabs(x);
        float tau = x > value ? attack : release;
        float c = std::exp(-1.f / std::max(1.f, tau * fs));
        value = c*value + (1.f-c)*x;
        return value;
    }
};

inline float db_to_gain(float db){ return std::pow(10.f, db/20.f); }
inline float gain_to_db(float g){ return 20.f*std::log10(std::max(g, 1e-9f)); }

