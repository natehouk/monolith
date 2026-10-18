#include "../plugins/Monolith/MonoEngine.hpp"
#include <cstdio>
using namespace mono;
int main(){ for(int mode=0;mode<3;++mode){ Engine e; e.setSampleRate(48000); e.p[kResonance]=1; e.p[kFilterMode]=mode; e.p[kNoiseLevel]=0.01f;
 e.p[kAmpSustain]=1;
 e.noteOn(60,100); e.p[kVolume]=-60; float pre=0; for(int b=0;b<10;++b){float l[512],r[512];e.render(l,r,512);} e.p[kVolume]=-6; float L[512],R[512],pk=0; bool bad=false; for(int b=0;b<400;++b){e.render(L,R,512); for(int i=0;i<512;++i){pk=std::max(pk,std::fabs(L[i])); if(!std::isfinite(L[i]))bad=true;}}
 printf("mode %d peak %.3f bad %d\n",mode,pk,bad);} }
