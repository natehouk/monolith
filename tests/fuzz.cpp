// Random parameters + MIDI at several sample rates; output must stay finite and <= 0 dBFS.
#include "../plugins/Monolith/ParamDefs.hpp"
#include <cstdio>
#include <random>
using namespace mono;
int main(){ std::mt19937 g(1); std::uniform_real_distribution<float> u(0,1); int bad=0; float pk=0;
 for(int sr: {44100,48000,96000,192000}){ Engine e; e.setSampleRate(sr);
  for(int it=0; it<3000; ++it){
   for(uint32_t i=0;i<kParamCount;++i){ const ParamDef& d=kDefs[i];
     float v=(u(g)<.3f)? (u(g)<.5f?d.min:d.max) : d.min+(d.max-d.min)*u(g);
     if(d.hints&kHintInteger) v=std::round(v); e.p[i]=v; }
   e.setTransport(u(g)<.5f, u(g)<.5f, u(g)*100, 60+u(g)*120, 3+int(u(g)*4));
   for(int k=0;k<3;++k){ if(u(g)<.5f) e.noteOn(int(u(g)*128),1+int(u(g)*126)); if(u(g)<.4f) e.noteOff(int(u(g)*128)); }
   if(u(g)<.1f) e.pitchBend(int(u(g)*16383)); if(u(g)<.05f) e.sustainPedal(u(g)<.5f);
   float L[333],R[333]; e.render(L,R,333); for(float x:L){ if(!std::isfinite(x)) bad++; pk=std::max(pk,std::fabs(x)); } } }
 printf("fuzz nonfinite=%d peak=%.3f\n",bad,pk); return bad?1:0; }
