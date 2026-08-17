#include "dsp.hpp"

static constexpr float PI = 3.14159265358979323846f;

Biquad make_highpass(float fs, float hz, float q){
    Biquad f;
    const float w0=2.f*PI*hz/fs, c=std::cos(w0), s=std::sin(w0), a=s/(2.f*q);
    float b0=(1.f+c)/2.f, b1=-(1.f+c), b2=(1.f+c)/2.f;
    float a0=1.f+a, a1=-2.f*c, a2=1.f-a;
    f.b0=b0/a0; f.b1=b1/a0; f.b2=b2/a0; f.a1=a1/a0; f.a2=a2/a0; return f;
}

Biquad make_peaking(float fs, float hz, float q, float gain_db){
    Biquad f;
    float A=std::pow(10.f,gain_db/40.f), w0=2.f*PI*hz/fs, c=std::cos(w0), s=std::sin(w0), a=s/(2.f*q);
    float b0=1.f+a*A, b1=-2.f*c, b2=1.f-a*A;
    float a0=1.f+a/A, a1=-2.f*c, a2=1.f-a/A;
    f.b0=b0/a0; f.b1=b1/a0; f.b2=b2/a0; f.a1=a1/a0; f.a2=a2/a0; return f;
}

Biquad make_highshelf(float fs, float hz, float gain_db){
    Biquad f;
    float A=std::pow(10.f,gain_db/40.f), w0=2.f*PI*hz/fs, c=std::cos(w0), s=std::sin(w0);
    float beta=std::sqrt(A+A);
    float b0=A*((A+1)+(A-1)*c+beta*s);
    float b1=-2*A*((A-1)+(A+1)*c);
    float b2=A*((A+1)+(A-1)*c-beta*s);
    float a0=(A+1)-(A-1)*c+beta*s;
    float a1=2*((A-1)-(A+1)*c);
    float a2=(A+1)-(A-1)*c-beta*s;
    f.b0=b0/a0; f.b1=b1/a0; f.b2=b2/a0; f.a1=a1/a0; f.a2=a2/a0; return f;
}


Biquad make_lowshelf(float fs, float hz, float gain_db){
    Biquad f;
    float A=std::pow(10.f,gain_db/40.f), w0=2.f*PI*hz/fs, c=std::cos(w0), s=std::sin(w0);
    float beta=std::sqrt(A+A);
    float b0=A*((A+1)-(A-1)*c+beta*s);
    float b1=2*A*((A-1)-(A+1)*c);
    float b2=A*((A+1)-(A-1)*c-beta*s);
    float a0=(A+1)+(A-1)*c+beta*s;
    float a1=-2*((A-1)+(A+1)*c);
    float a2=(A+1)+(A-1)*c-beta*s;
    f.b0=b0/a0; f.b1=b1/a0; f.b2=b2/a0; f.a1=a1/a0; f.a2=a2/a0; return f;
}

