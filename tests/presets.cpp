// Render every factory preset (a short phrase / chord) and report peak and RMS level.
#include "../plugins/Monolith/ParamDefs.hpp"
#include "../plugins/Monolith/Presets.hpp"
#include <cstdio>
#include <vector>
using namespace mono;
int main(){ const int sr=48000; int bad=0;
 for(uint32_t i=0;i<kNumPresets;++i){ Engine e; e.setSampleRate(sr);
  std::copy(presets()[i].v, presets()[i].v+kParamCount, e.p); e.setTransport(true,true,0,120,4);
  const bool poly=e.p[kVoiceMode]>0.5f; std::vector<float> L(sr*2),R(sr*2);
  int notes[]={48,55,60,64}; for(int k=0;k<(poly?4:1);++k) e.noteOn(notes[k]+(poly?0:0),100);
  e.render(L.data(),R.data(),sr); for(int k=0;k<4;++k) e.noteOff(notes[k]); e.render(L.data()+sr,R.data()+sr,sr);
  double s=0; float pk=0; bool nf=false; for(float x:L){ s+=x*x; pk=std::max(pk,std::fabs(x)); nf|=!std::isfinite(x);} float r=std::sqrt(s/L.size());
  bool ok=!nf && pk>0.02f; bad+=!ok;
  printf("%-15s %-13s peak %5.2f  rms %5.3f %s\n",presets()[i].name,presets()[i].category,pk,r,ok?"":"<-- CHECK"); }
 printf("%u presets, %d flagged\n",kNumPresets,bad); return bad; }
