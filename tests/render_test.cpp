#include "../Source/KickEngine.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <iostream>
struct P{const char*n; float pu,bo,le,dr,ru;};
static void save(const std::vector<float>& d,const juce::File& f,double sr){
  f.deleteFile(); auto* os=f.createOutputStream().release(); juce::WavAudioFormat w;
  std::unique_ptr<juce::AudioFormatWriter> wr(w.createWriterFor(os,sr,1,24,{},0));
  juce::AudioBuffer<float> b(1,(int)d.size()); b.copyFrom(0,0,d.data(),(int)d.size()); wr->writeFromAudioSampleBuffer(b,0,b.getNumSamples());}
int main(){

  P ps[]={{"INDUSTRIAL_HAMMER",.85f,.45f,.45f,.8f,.15f},{"WAREHOUSE_RUMBLE",.6f,.6f,.55f,.65f,.75f},{"SCHRANZ_PUNCH",.95f,.3f,.3f,.9f,0},{"DEEP_TUNNEL",.5f,.8f,.8f,.45f,.5f},{"RAW_DISTORTION",.75f,.55f,.5f,1,.3f},{"CLEAN_SUB",.4f,.7f,.6f,.1f,0}};
  juce::File out("/home/claude/sdkick/testout"); out.createDirectory();
  for(auto&p:ps){ KickParams k; k.punch=p.pu;k.body=p.bo;k.length=p.le;k.drive=p.dr;k.rumble=p.ru;k.bpm=160;k.outputDb=-3;
    // 4 kicks in a row via engine to hear rumble between kicks
    KickEngine e; e.setParams(k); e.prepare(44100,256); int beat=(int)(44100*k.beatSeconds()); std::vector<float> v(beat*4);
    for(int b=0;b<4;b++){ e.trigger(1.f); for(int pos=0;pos<beat;pos+=256){int n=std::min(256,beat-pos); e.process(v.data()+b*beat+pos,n);} }
    float pk=0; bool bad=false; for(float x:v){pk=std::max(pk,std::abs(x)); if(!std::isfinite(x))bad=true;}
    save(v,out.getChildFile(juce::String(p.n)+".wav"),44100);
    auto one=KickEngine::renderOneShot(k,nullptr,44100,k.beatSeconds());
    auto sm=std::make_shared<KickSample>(); sm->audio.setSize(1,(int)one.size()); sm->audio.copyFrom(0,0,one.data(),(int)one.size()); sm->sampleRate=44100;
    double hz=KickSample::detectPitch(sm->audio,44100);
    std::cout<<p.n<<" peak="<<pk<<" nan="<<bad<<" f0="<<k.fundamentalHz()<<" detected="<<hz<<"\n";
  }
  // sample mode test: retune a G (49Hz) kick to F#
  KickParams g; g.key=7; g.drive=0.2f; auto one=KickEngine::renderOneShot(g,nullptr,48000,0.4);
  auto sm=std::make_shared<KickSample>(); sm->audio.setSize(1,(int)one.size()); sm->audio.copyFrom(0,0,one.data(),(int)one.size()); sm->sampleRate=48000; sm->detectedHz=KickSample::detectPitch(sm->audio,48000);
  KickParams s; s.sampleMode=true; s.key=6; s.drive=0.5f; auto r=KickEngine::renderOneShot(s,sm,44100,0.4);
  auto sm2=std::make_shared<KickSample>(); sm2->audio.setSize(1,(int)r.size()); sm2->audio.copyFrom(0,0,r.data(),(int)r.size());
  std::cout<<"sample G detected="<<sm->detectedHz<<" -> retuned detected="<<KickSample::detectPitch(sm2->audio,44100)<<" target="<<s.fundamentalHz()<<"\n";
  save(r,out.getChildFile("SAMPLE_RETUNE.wav"),44100);
}
