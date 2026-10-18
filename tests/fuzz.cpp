#include "../plugins/Monolith/MonoEngine.hpp"
#include <cstdio>
#include <random>
using namespace mono;
float lo[kParamCount]={0,.05f,-2,-100,0,0,0,0,0,20,0,0,0,-1,0,0,.001f,0,.001f,0,.001f,0,.001f,0,.05f,0,0,0,-36};
float hi[kParamCount]={1,.95f,2,100,1,1,2,1,2,20000,1,1,1,1,1,5,5,1,5,5,5,1,5,1,20,1,1,12,6};
int main(){ std::mt19937 g(1); std::uniform_real_distribution<float> u(0,1); int bad=0; float pk=0;
 for(int sr: {44100,48000,96000,192000}){ Engine e; e.setSampleRate(sr);
  for(int it=0; it<3000; ++it){ for(int i=0;i<kParamCount;++i) e.p[i]= (u(g)<.3f)? (u(g)<.5f?lo[i]:hi[i]) : lo[i]+(hi[i]-lo[i])*u(g);
   if(u(g)<.5f) e.noteOn(int(u(g)*128),1+int(u(g)*126)); if(u(g)<.4f) e.noteOff(int(u(g)*128)); if(u(g)<.1f)e.pitchBend(int(u(g)*16383));
   float L[333],R[333]; e.render(L,R,333); for(float x:L){ if(!std::isfinite(x)) bad++; pk=std::max(pk,std::fabs(x)); } } }
 printf("fuzz nonfinite=%d peak=%.2f\n",bad,pk); }
