#include <obs-module.h>
#include <media-io/audio-io.h>
#include <vector>
#include "dsp.hpp"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("solocast-clean", "en-US")
MODULE_EXPORT const char *obs_module_description(void){ return "SoloCast Clean - lightweight voice cleanup filter for OBS"; }

struct ChannelState {
    Biquad hp, deep_shelf, body, mud, presence, air, deess_hp;
    Envelope voice_env, comp_env, deess_env;
    float gate_gain=1.f;
};

struct SoloClean {
    obs_source_t *context=nullptr;
    uint32_t sample_rate=48000;
    std::vector<ChannelState> ch;
    size_t channels=2;
    float noise_threshold_db=-42.f;
    float gate_floor_db=-24.f;
    float deep_db=2.0f;
    float clarity_db=2.5f;
    float compression=3.f;
    float deess_amount=4.f;
    float output_db=0.f;
};

static void setup_channel(ChannelState &s, float fs, float deep, float clarity){
    s.hp = make_highpass(fs, 82.f);
    // "Deep Voice" is natural tonal deepening rather than an artificial pitch shifter.
    s.deep_shelf = make_lowshelf(fs, 165.f, deep * 0.55f);
    s.body = make_peaking(fs, 135.f, 0.85f, deep * 0.40f);
    s.mud = make_peaking(fs, 260.f, 1.0f, -2.5f);
    s.presence = make_peaking(fs, 3800.f, 0.9f, clarity);
    s.air = make_highshelf(fs, 9000.f, clarity * 0.25f);
    s.deess_hp = make_highpass(fs, 5200.f, 0.707f);
    s.voice_env.set(fs, 0.008f, 0.18f);
    s.comp_env.set(fs, 0.006f, 0.10f);
    s.deess_env.set(fs, 0.002f, 0.05f);
}

static const char *filter_name(void*){ return "SoloCast Clean"; }

static void update(void *data, obs_data_t *settings){
    auto *f=(SoloClean*)data;
    f->noise_threshold_db=(float)obs_data_get_double(settings,"noise_threshold");
    f->gate_floor_db=(float)obs_data_get_double(settings,"gate_floor");
    f->deep_db=(float)obs_data_get_double(settings,"deep");
    f->clarity_db=(float)obs_data_get_double(settings,"clarity");
    f->compression=(float)obs_data_get_double(settings,"compression");
    f->deess_amount=(float)obs_data_get_double(settings,"deess");
    f->output_db=(float)obs_data_get_double(settings,"output");
    for(auto &c:f->ch) setup_channel(c,(float)f->sample_rate,f->deep_db,f->clarity_db);
}

static void *create(obs_data_t *settings, obs_source_t *source){
    auto *f=new SoloClean(); f->context=source;
    audio_t *audio=obs_get_audio();
    if(audio) f->sample_rate=audio_output_get_sample_rate(audio);
    f->channels = std::min<size_t>(audio ? audio_output_get_channels(audio) : 2, MAX_AV_PLANES);
    f->ch.resize(f->channels);
    for(auto &c:f->ch) setup_channel(c,(float)f->sample_rate,f->deep_db,f->clarity_db);
    update(f,settings); return f;
}
static void destroy(void *data){ delete (SoloClean*)data; }

static obs_properties_t *properties(void*){
    obs_properties_t *p=obs_properties_create();
    obs_properties_add_float_slider(p,"noise_threshold","Noise / desk threshold (dB)",-60.0,-20.0,1.0);
    obs_properties_add_float_slider(p,"gate_floor","Background reduction (dB)",-40.0,-6.0,1.0);
    obs_properties_add_float_slider(p,"deep","Deep Voice",0.0,6.0,0.25);
    obs_properties_add_float_slider(p,"clarity","Clarity (dB)",0.0,6.0,0.25);
    obs_properties_add_float_slider(p,"compression","Compression",1.0,6.0,0.25);
    obs_properties_add_float_slider(p,"deess","De-esser",0.0,10.0,0.5);
    obs_properties_add_float_slider(p,"output","Output gain (dB)",-6.0,6.0,0.5);
    return p;
}

static void defaults(obs_data_t *s){
    obs_data_set_default_double(s,"noise_threshold",-42.0);
    obs_data_set_default_double(s,"gate_floor",-24.0);
    obs_data_set_default_double(s,"deep",2.0);
    obs_data_set_default_double(s,"clarity",2.5);
    obs_data_set_default_double(s,"compression",3.0);
    obs_data_set_default_double(s,"deess",4.0);
    obs_data_set_default_double(s,"output",0.0);
}

static struct obs_audio_data *filter_audio(void *data, struct obs_audio_data *audio){
    auto *f=(SoloClean*)data;
    const float out_gain=db_to_gain(f->output_db);
    const float floor_gain=db_to_gain(f->gate_floor_db);
    const float comp_threshold=-18.f;
    const float gate_close_coeff = 1.f-std::exp(-1.f/(0.018f*(float)f->sample_rate));
    const float gate_open_coeff = 1.f-std::exp(-1.f/(0.070f*(float)f->sample_rate));
    constexpr float limiter_knee=0.75f;
    constexpr float limiter_ceiling=0.89125094f; // -1 dBFS

    for(size_t c=0;c<f->channels;c++){
        if(!audio->data[c]) continue;
        float *samples=(float*)audio->data[c];
        auto &s=f->ch[c];
        for(uint32_t i=0;i<audio->frames;i++){
            float x=samples[i];

            // Remove desk thumps / low-frequency rumble, then shape SoloCast voice.
            x=s.hp.process(x);
            x=s.deep_shelf.process(x);
            x=s.body.process(x);
            x=s.mud.process(x);
            x=s.presence.process(x);
            x=s.air.process(x);

            // Smooth downward expander: greatly reduces keyboard/desk noise while silent.
            float env=s.voice_env.process(x);
            float db=gain_to_db(env);
            float target=1.f;
            if(db < f->noise_threshold_db){
                float distance=f->noise_threshold_db-db;
                float atten=std::min(-f->gate_floor_db, distance*1.5f);
                target=db_to_gain(-atten);
                target=std::max(target,floor_gain);
            }
            float coeff = target < s.gate_gain ? gate_close_coeff : gate_open_coeff;
            s.gate_gain += (target-s.gate_gain)*coeff;
            x*=s.gate_gain;

            // De-esser using a high-band detector controlling broadband attenuation.
            float high=s.deess_hp.process(x);
            float de=s.deess_env.process(high);
            float de_db=gain_to_db(de);
            if(de_db>-30.f){
                float reduction=std::min(f->deess_amount,(de_db+30.f)*0.35f);
                x*=db_to_gain(-reduction);
            }

            // Soft compressor above -18 dBFS.
            float ce=s.comp_env.process(x);
            float ce_db=gain_to_db(ce);
            if(ce_db>comp_threshold && f->compression>1.01f){
                float over=ce_db-comp_threshold;
                float reduced=over-(over/f->compression);
                x*=db_to_gain(-reduced);
            }

            // Transparent soft-knee limiter with a strict -1 dBFS ceiling.
            x*=out_gain;
            const float sign=x<0.f ? -1.f : 1.f;
            float magnitude=std::fabs(x);
            if(magnitude>limiter_knee){
                const float span=limiter_ceiling-limiter_knee;
                magnitude=limiter_knee+span*(1.f-std::exp(-(magnitude-limiter_knee)/span));
            }
            samples[i]=sign*std::min(magnitude,limiter_ceiling);
        }
    }
    return audio;
}

static obs_source_info info={};

bool obs_module_load(void){
    info.id="solocast_clean_filter";
    info.type=OBS_SOURCE_TYPE_FILTER;
    info.output_flags=OBS_SOURCE_AUDIO;
    info.get_name=filter_name;
    info.create=create;
    info.destroy=destroy;
    info.update=update;
    info.get_defaults=defaults;
    info.get_properties=properties;
    info.filter_audio=filter_audio;
    obs_register_source(&info);
    blog(LOG_INFO,"SoloCast Clean loaded");
    return true;
}

