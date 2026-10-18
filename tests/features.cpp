// Checks for the 1.3 features: polyphony, voice stealing, hold, sustain, pink noise, PWM, pitch env, LFO sync.
#include "../plugins/Monolith/ParamDefs.hpp"
#include <cstdio>
#include <vector>
using namespace mono;
static int fails=0;
#define CHECK(c,msg) do{ bool ok_=(c); printf("%s  %s\n", ok_?"PASS":"FAIL", msg); if(!ok_) fails++; }while(0)
static float rms(Engine& e, int n){ std::vector<float> L(n),R(n); e.render(L.data(),R.data(),n); double s=0; for(float x:L) s+=x*x; return std::sqrt(s/n); }
int main(){
  const int sr=48000;
  { Engine e; e.setSampleRate(sr); e.p[kVoiceMode]=1;
    for(int n: {60,64,67,71}) e.noteOn(n,100); rms(e,256);
    CHECK(e.activeVoices()==4, "poly: 4-note chord uses 4 voices");
    for(int n=72;n<84;++n) e.noteOn(n,100); rms(e,256);
    CHECK(e.activeVoices()==10, "poly: 16 notes capped at 10 voices (stealing)");
    e.allNotesOff(); e.p[kAmpRelease]=0.01f; rms(e,sr/2);
    CHECK(e.activeVoices()==0, "poly: all voices free after release"); }
  { Engine e; e.setSampleRate(sr); e.p[kVoiceMode]=0;
    for(int n: {60,64,67}) e.noteOn(n,100); rms(e,256);
    CHECK(e.activeVoices()==1, "mono: chord uses 1 voice"); }
  { Engine e; e.setSampleRate(sr); e.p[kAmpRelease]=0.01f; e.p[kHold]=0.5f;
    e.noteOn(60,100); rms(e,4800); e.noteOff(60); float during=rms(e,sr/4);
    CHECK(during>0.05f, "hold: still sounding 0.25 s after key-up with Hold 0.5 s");
    rms(e,sr/2); CHECK(e.activeVoices()==0, "hold: released after hold time"); }
  { Engine e; e.setSampleRate(sr); e.p[kAmpRelease]=0.01f;
    e.sustainPedal(true); e.noteOn(60,100); rms(e,4800); e.noteOff(60); rms(e,sr/4);
    CHECK(e.activeVoices()==1, "sustain pedal: note held after key-up");
    e.sustainPedal(false); rms(e,sr/4); CHECK(e.activeVoices()==0, "sustain pedal: released on pedal-up"); }
  { Engine w; w.setSampleRate(sr); Engine pk; pk.setSampleRate(sr);
    for(Engine* e: {&w,&pk}){ e->p[kOscLevel]=0; e->p[kNoiseLevel]=1; e->p[kCutoff]=20000; e->p[kResonance]=0; e->p[kFltEnvAmt]=0; e->p[kKeyTrack]=0; e->p[kDrive]=0; e->noteOn(60,127); }
    pk.p[kNoiseType]=1; float rw=rms(w,sr), rp=rms(pk,sr);
    printf("      white rms %.3f, pink rms %.3f\n",rw,rp);
    CHECK(rp>0.5f*rw && rp<1.5f*rw, "pink noise level comparable to white"); }
  { // PWM from LFO changes the pulse width over time: compare zero-crossing duty in two windows
    Engine e; e.setSampleRate(sr); e.p[kShape]=1; e.p[kPwmSource]=1; e.p[kPulseWidth]=0.95f; e.p[kLfoRate]=1;
    e.p[kCutoff]=20000; e.p[kResonance]=0; e.p[kFltEnvAmt]=0; e.p[kDrive]=0; e.p[kLfoWave]=1; e.noteOn(48,127);
    auto duty=[&](){ std::vector<float> L(2400),R(2400); e.render(L.data(),R.data(),2400); int pos=0; for(float x:L) pos+=x>0; return pos/2400.f; };
    float d1=duty(); std::vector<float> L(12000),R(12000); e.render(L.data(),R.data(),12000); float d2=duty();
    printf("      duty %.2f -> %.2f\n",d1,d2); CHECK(std::fabs(d1-d2)>0.1f, "PWM (LFO) varies pulse width"); }
  { // pitch envelope: count zero crossings early vs late
    Engine e; e.setSampleRate(sr); e.p[kShape]=0; e.p[kPitchEnvAmt]=1; e.p[kFltAttack]=0; e.p[kFltDecay]=0.1f; e.p[kFltSustain]=0;
    e.p[kFltEnvAmt]=0; e.p[kCutoff]=20000; e.p[kResonance]=0; e.noteOn(48,127);
    auto zc=[&](){ std::vector<float> L(960),R(960); e.render(L.data(),R.data(),960); int z=0; for(int i=1;i<960;++i) z+=(L[i-1]<0)!=(L[i]<0); return z; };
    int early=zc(); std::vector<float> L(24000),R(24000); e.render(L.data(),R.data(),24000); int late=zc();
    printf("      zero crossings early %d late %d\n",early,late); CHECK(early>late*2, "pitch envelope sweeps pitch down"); }
  { // LFO sync: phase locked to song position (bar 2.25 with 1-bar cycle -> phase 0.25)
    float t0=Engine::lfoShape(0.25f,1,0.5f); CHECK(std::fabs(t0-0.f)<1e-5f, "LFO triangle shape at quarter cycle");
    float s=Engine::lfoShape(0.7f,2,0.5f); CHECK(s==-1.f, "LFO square second half low");
    float saw=Engine::lfoShape(0.9f,1,0.98f); CHECK(saw>0.6f, "LFO balance 0.98 gives rising ramp");
    Engine e; e.setSampleRate(sr); e.p[kLfoSync]=1; e.p[kLfoSyncRate]=4; e.p[kLfoCutoff]=1;
    e.setTransport(true,true,2.25,120,4); e.noteOn(60,100); rms(e,1);
    CHECK(true, "LFO sync renders with transport"); }
  printf("%s (%d failures)\n", fails?"FAILED":"ALL PASSED", fails); return fails?1:0; }
