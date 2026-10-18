#include "../plugins/Monolith/MonoEngine.hpp"
#include <cstdio>
#include <vector>
using namespace mono;
static void wav(const char* f, const std::vector<float>& s, int sr){
  FILE* fp=fopen(f,"wb"); int n=s.size(); int bytes=n*2;
  auto w32=[&](int v){fwrite(&v,4,1,fp);}; auto w16=[&](short v){fwrite(&v,2,1,fp);};
  fwrite("RIFF",1,4,fp); w32(36+bytes); fwrite("WAVEfmt ",1,8,fp); w32(16); w16(1); w16(1); w32(sr); w32(sr*2); w16(2); w16(16);
  fwrite("data",1,4,fp); w32(bytes); for(float x: s){ float c=x>1?1:(x<-1?-1:x); w16((short)(c*32767)); } fclose(fp);}
int main(){
  const int sr=48000; int bad=0;
  for(int preset=0; preset<3; ++preset){
    Engine e; e.setSampleRate(sr);
    if(preset==1){ e.p[kCutoff]=320; e.p[kResonance]=0.82f; e.p[kDrive]=.45f; e.p[kFltEnvAmt]=.6f; e.p[kFltDecay]=.22f; e.p[kFltSustain]=0; e.p[kGlide]=.08f; e.p[kOctave]=-1;}
    if(preset==2){ e.p[kResonance]=1.f; e.p[kFilterMode]=1; e.p[kShape]=0; }
    std::vector<float> out; float L[256],R[256]; float peak=0, rms=0;
    int seq[]={48,51,55,58,60,58,55,51};
    for(int step=0; step<16; ++step){
      e.noteOn(seq[step%8], step%3?90:127);
      for(int b=0;b<30;++b){ e.render(L,R,256); for(int i=0;i<256;++i){ if(!std::isfinite(L[i])) bad++; out.push_back(L[i]); peak=std::max(peak,std::fabs(L[i])); rms+=L[i]*L[i]; } }
      e.noteOff(seq[step%8]);
      for(int b=0;b<8;++b){ e.render(L,R,256); for(int i=0;i<256;++i){ if(!std::isfinite(L[i])) bad++; out.push_back(L[i]); } }
    }
    char fn[32]; sprintf(fn,"render%d.wav",preset); wav(fn,out,sr);
    printf("preset %d: peak %.3f rms %.3f nonfinite %d\n",preset,peak,std::sqrt(rms/out.size()),bad);
  }
}
