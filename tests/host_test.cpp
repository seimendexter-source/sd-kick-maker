#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <iostream>
int main(){
  juce::ScopedJuceInitialiser_GUI init;
  auto proc=std::make_unique<SDKickMakerProcessor>();
  proc->setPlayConfigDetails(0,2,48000,480);
  proc->prepareToPlay(48000,480);
  juce::AudioBuffer<float> buf(2,480); juce::MidiBuffer midi; bool bad=false; float pk=0;
  int beat=(int)(48000*60/160.0);
  for(int b=0;b<400;b++){ buf.clear(); midi.clear(); long long start=(long long)b*480;
    for(long long s=start; s<start+480; s++) if(s%beat==0) midi.addEvent(juce::MidiMessage::noteOn(1,36,(juce::uint8)110),(int)(s-start));
    proc->processBlock(buf,midi);
    for(int c=0;c<2;c++) for(int i=0;i<480;i++){float x=buf.getSample(c,i); if(!std::isfinite(x)) bad=true; pk=std::max(pk,std::abs(x));}}
  std::cout<<"process ok nan="<<bad<<" peak="<<pk<<" latency="<<proc->getLatencySamples()<<"\n";
  proc->loadPreset(1); std::cout<<"preset rumble="<<proc->currentParams().rumble<<"\n";
  bool ok=proc->loadSampleFile(juce::File("/home/claude/sdkick/testout/SAMPLE_RETUNE.wav"));
  std::cout<<"sample load="<<ok<<" info="<<proc->getSampleInfo()<<"\n";
  juce::MemoryBlock mb; proc->getStateInformation(mb);
  auto p2=std::make_unique<SDKickMakerProcessor>(); p2->setStateInformation(mb.getData(),(int)mb.getSize());
  std::cout<<"restore rumble="<<p2->currentParams().rumble<<" mode="<<p2->currentParams().sampleMode<<" info="<<p2->getSampleInfo()<<"\n";
  proc->toggleAB(); proc->loadPreset(2); proc->toggleAB(); std::cout<<"AB back to A rumble="<<proc->currentParams().rumble<<" onB="<<proc->isOnB()<<"\n";
  std::cout<<"export="<<proc->exportWav(juce::File("/home/claude/sdkick/testout/EXPORT.wav"))<<"\n";
  proc->randomizeParameters(); auto rp=proc->currentParams(); std::cout<<"random punch="<<rp.punch<<" drive="<<rp.drive<<" tune="<<rp.tune<<" out="<<rp.outputDb<<" dw="<<rp.dryWet<<"\n";
  // dry/wet 0 in sample mode with phase invert -> output should equal inverted sample (approx)
  proc->apvts.getParameter("dryWet")->setValueNotifyingHost(0.0f); proc->apvts.getParameter("phaseInvert")->setValueNotifyingHost(1.0f);
  { auto p=proc->currentParams(); auto d=KickEngine::renderOneShot(p,proc->getSamplePtr(),44100,0.3); float mx=0,mn=0; for(float x:d){mx=std::max(mx,x);mn=std::min(mn,x);} std::cout<<"dry-only inverted max="<<mx<<" min="<<mn<<"\n"; }
  proc->apvts.getParameter("dryWet")->setValueNotifyingHost(1.0f); proc->apvts.getParameter("phaseInvert")->setValueNotifyingHost(0.0f);
  proc->apvts.getParameter("output")->setValueNotifyingHost(1.0f);
  { juce::AudioBuffer<float> b2(2,480); juce::MidiBuffer m2; m2.addEvent(juce::MidiMessage::noteOn(1,36,(juce::uint8)127),0); proc->processBlock(b2,m2); std::cout<<"clip flag (out +6dB)="<<proc->isClipped()<<"\n"; proc->resetClip(); }
  proc->apvts.getParameter("output")->setValueNotifyingHost(proc->apvts.getParameter("output")->convertTo0to1(-3.0f));
  proc->loadPreset(0);
  { std::unique_ptr<juce::AudioProcessorEditor> ed(proc->createEditor()); ed->setVisible(true);
    for(int i=0;i<10;i++) juce::MessageManager::getInstance()->runDispatchLoopUntil(30);
    auto img=ed->createComponentSnapshot(ed->getLocalBounds(),true,2.0f);
    juce::File f("/home/claude/sdkick/testout/ui.png"); f.deleteFile(); juce::FileOutputStream os(f); juce::PNGImageFormat png; png.writeImageToStream(img,os);
    std::cout<<"ui "<<img.getWidth()<<"x"<<img.getHeight()<<"\n"; }
  proc->releaseResources();
}
