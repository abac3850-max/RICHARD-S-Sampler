#include "PluginEditor.h"

const std::array<RASamplerAudioProcessorEditor::Skin, 10>& RASamplerAudioProcessorEditor::getSkins()
{
    static const std::array<Skin, 10> skins {{
        { "Dark Green", 0xff101713, 0xff1b2520, 0xff222e27, 0xff65e58a, 0xff286b42, 0xfff1f6f2, 0xffaab9ae, 0xff34463a },
        { "Midnight Violet", 0xff15121b, 0xff211b2b, 0xff2c2438, 0xffbd8cff, 0xff68459a, 0xfff5f0fb, 0xffb8adc5, 0xff443653 },
        { "Ocean Teal", 0xff10191a, 0xff1a292b, 0xff223537, 0xff54dfd0, 0xff267c76, 0xffeffafa, 0xffa9c1bf, 0xff355253 },
        { "Sunset Orange", 0xff1b1512, 0xff2b211b, 0xff382a22, 0xffffa45e, 0xffa9572b, 0xfffff5ed, 0xffc7b2a3, 0xff594235 },
        { "Ruby Red", 0xff1b1215, 0xff2b1b20, 0xff38232a, 0xffff7185, 0xff963b50, 0xfffff1f3, 0xffc6aab0, 0xff57343d },
        { "Arctic Silver", 0xff171a1d, 0xff252b30, 0xff30383e, 0xffd1e0e8, 0xff718a99, 0xfff5f8fa, 0xffb5c0c7, 0xff46545d },
        { "Gold Edition", 0xff19170f, 0xff2a2619, 0xff37311f, 0xffffd36b, 0xff9b7930, 0xfffff8e8, 0xffc9bd9c, 0xff5a4e2d },
        { "Electric Lime", 0xff141910, 0xff20291a, 0xff2b3821, 0xffc5f45b, 0xff718e27, 0xfff7fbe9, 0xffb7c39a, 0xff46552d },
        { "Sapphire", 0xff111620, 0xff1b2636, 0xff243348, 0xff75aaff, 0xff3c639d, 0xfff0f5ff, 0xffaabbd4, 0xff384d6b },
        { "Rose Quartz", 0xff1b1519, 0xff2b2028, 0xff392a34, 0xffffa8cf, 0xffa85e86, 0xfffff2f8, 0xffcbb0bf, 0xff594052 }
    }};
    return skins;
}

void RASamplerAudioProcessorEditor::GreenLook::drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h, float pos, float start, float end, juce::Slider& slider)
{
    juce::ignoreUnused(slider);
    auto b = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h).reduced(7.0f);
    const float diameter = juce::jmin(b.getWidth(), b.getHeight());
    auto knob = juce::Rectangle<float>(diameter, diameter).withCentre(b.getCentre());
    const float angle = start + pos * (end - start);
    g.setColour(juce::Colour(0xff080b09)); g.fillEllipse(knob.translated(0, 2));
    g.setColour(juce::Colour(0xff252d28)); g.fillEllipse(knob);
    g.setColour(juce::Colour(0xff46534a)); g.drawEllipse(knob, 1.0f);
    juce::Path arc; arc.addCentredArc(knob.getCentreX(), knob.getCentreY(), diameter * .43f, diameter * .43f, 0.0f, start, angle, true);
    g.setColour(juce::Colour(0xff65e58a)); g.strokePath(arc, juce::PathStrokeType(3.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    juce::Path pointer; pointer.addRoundedRectangle(-1.5f, -diameter * .32f, 3.0f, diameter * .19f, 1.5f);
    g.setColour(juce::Colour(0xffd8ffe2)); g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(knob.getCentreX(), knob.getCentreY()));
}

RASamplerAudioProcessorEditor::RASamplerAudioProcessorEditor(RASamplerAudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    setLookAndFeel(&look); setResizable(true, true); setResizeLimits(760, 560, 1900, 1300); setSize(1120, 760);
    title.setText("RA SAMPLER", juce::dontSendNotification); title.setFont(juce::Font(27.0f, juce::Font::bold)); addAndMakeVisible(title);
    slotTitle.setText("SLOT 1 | EMPTY", juce::dontSendNotification); addAndMakeVisible(slotTitle);
    waveformLabel.setText("WAVEFORM EDITOR  |  DRAG THE GREEN HANDLES", juce::dontSendNotification); addAndMakeVisible(waveformLabel);
    addAndMakeVisible(waveform);
    for (int i=0;i<RASamplerAudioProcessor::slotCount;++i) { slotButtons[(size_t)i]=std::make_unique<juce::TextButton>("SLOT "+juce::String(i+1)); slotButtons[(size_t)i]->onClick=[this,i]{selectSlot(i);}; addAndMakeVisible(*slotButtons[(size_t)i]); }
    loadButton.onClick=[this]{loadSelectedSample();}; addAndMakeVisible(loadButton);
    loopButton.onClick=[this]{ auto next=!processor.getSlot(selectedSlot).loopEnabled; processor.setSlotLoop(selectedSlot,next); loopButton.setButtonText(next?"LOOP: ON":"LOOP: OFF"); }; addAndMakeVisible(loopButton);
    reverseButton.setEnabled(false); addAndMakeVisible(reverseButton);
    juce::StringArray skins; for (auto& s:getSkins()) skins.add(s.name);
    skinBox.addItemList(skins,1); selectedSkin=processor.getSkinIndex(); skinBox.setSelectedId(selectedSkin+1,juce::dontSendNotification);
    skinBox.onChange=[this]{selectedSkin=skinBox.getSelectedId()-1;processor.setSkinIndex(selectedSkin);applySkin();}; addAndMakeVisible(skinBox);
    factoryBox.addItemList(presetNames,1); factoryBox.setTextWhenNothingSelected("FACTORY KITS");
#ifdef RA_FACTORY_PRESET_DIR
    factoryPresetDirectory=juce::File(RA_FACTORY_PRESET_DIR);
#endif
    factoryBox.onChange=[this]{int id=factoryBox.getSelectedId();if(id>0)loadFactoryKit(id-1);}; addAndMakeVisible(factoryBox);
    auto setupSlider=[this](juce::Slider& sl, juce::Label& lb, const juce::String& name, double min, double max, double val, bool rotary){ lb.setText(name,juce::dontSendNotification);lb.setJustificationType(juce::Justification::centred);addAndMakeVisible(lb);sl.setRange(min,max,0.001);sl.setValue(val);sl.setSliderStyle(rotary?juce::Slider::RotaryHorizontalVerticalDrag:juce::Slider::LinearHorizontal);sl.setTextBoxStyle(juce::Slider::TextBoxBelow,false,65,18);addAndMakeVisible(sl);};
    setupSlider(gainSlider,gainLabel,"SLOT LEVEL",0,2,1,true); gainSlider.onValueChange=[this]{processor.setSlotGain(selectedSlot,(float)gainSlider.getValue());};
    setupSlider(panSlider,panLabel,"PAN",-1,1,0,true); setupSlider(pitchSlider,pitchLabel,"PITCH",-24,24,0,true);
    setupSlider(fadeInSlider,fadeInLabel,"FADE IN",0,1,0,false); setupSlider(fadeOutSlider,fadeOutLabel,"FADE OUT",0,1,0,false);
    fadeInSlider.onValueChange=[this]{updateEditFromWaveform();}; fadeOutSlider.onValueChange=[this]{updateEditFromWaveform();};
    applySkin(); startTimerHz(8);
}


RASamplerAudioProcessorEditor::~RASamplerAudioProcessorEditor() { setLookAndFeel(nullptr); }

void RASamplerAudioProcessorEditor::applySkin()
{
    const auto& s=getSkins()[(size_t)juce::jlimit(0,9,selectedSkin)]; auto c=[](juce::uint32 v){return juce::Colour(v);};
    title.setColour(juce::Label::textColourId,c(s.text)); slotTitle.setColour(juce::Label::textColourId,c(s.text)); waveformLabel.setColour(juce::Label::textColourId,c(s.muted));
    for(int i=0;i<RASamplerAudioProcessor::slotCount;++i){auto& b=*slotButtons[(size_t)i];b.setColour(juce::TextButton::buttonColourId,c(i==selectedSlot?s.accentDark:s.panelAlt));b.setColour(juce::TextButton::textColourOffId,c(s.text));b.setColour(juce::TextButton::textColourOnId,c(s.text));}
    for(auto* b:{&loadButton,&loopButton,&reverseButton}){b->setColour(juce::TextButton::buttonColourId,c(s.panelAlt));b->setColour(juce::TextButton::textColourOffId,c(s.text));b->setColour(juce::TextButton::textColourOnId,c(s.text));}
    for(auto* cb:{&skinBox,&factoryBox}){cb->setColour(juce::ComboBox::backgroundColourId,c(s.panelAlt));cb->setColour(juce::ComboBox::textColourId,c(s.text));cb->setColour(juce::ComboBox::outlineColourId,c(s.border));}
    for(auto* sl:{&gainSlider,&panSlider,&pitchSlider,&fadeInSlider,&fadeOutSlider}){sl->setColour(juce::Slider::thumbColourId,c(s.accent));sl->setColour(juce::Slider::rotarySliderFillColourId,c(s.accent));sl->setColour(juce::Slider::trackColourId,c(s.border));sl->setColour(juce::Slider::textBoxTextColourId,c(s.text));sl->setColour(juce::Slider::textBoxBackgroundColourId,c(s.panelAlt));sl->setColour(juce::Slider::textBoxOutlineColourId,c(s.border));}
    for(auto* l:{&gainLabel,&panLabel,&pitchLabel,&fadeInLabel,&fadeOutLabel})l->setColour(juce::Label::textColourId,c(s.muted));
    waveform.repaint(); repaint();
}

void RASamplerAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto& s=getSkins()[(size_t)juce::jlimit(0,9,selectedSkin)]; auto c=[](juce::uint32 v){return juce::Colour(v);};
    g.fillAll(c(s.background)); g.setColour(c(s.panel)); g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(12),14); g.setColour(c(s.border)); g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(12),14,1);
}

void RASamplerAudioProcessorEditor::resized()
{
    auto r=getLocalBounds().reduced(28); auto header=r.removeFromTop(54); title.setBounds(header.removeFromLeft(300)); skinBox.setBounds(header.removeFromRight(180).reduced(2,8)); factoryBox.setBounds(header.removeFromRight(220).reduced(3,8));
    auto slots=r.removeFromTop(62); int gap=7,w=(slots.getWidth()-gap*7)/8; for(int i=0;i<8;++i){slotButtons[(size_t)i]->setBounds(slots.removeFromLeft(w).reduced(2,3));if(i<7)slots.removeFromLeft(gap);}
    auto bottom=r.removeFromBottom(180); auto controls=bottom.removeFromBottom(126); auto left=controls.removeFromLeft(180);slotTitle.setBounds(left.removeFromTop(24));loadButton.setBounds(left.removeFromTop(36).reduced(2));loopButton.setBounds(left.removeFromTop(32).reduced(2));reverseButton.setBounds(left.removeFromTop(30).reduced(2));
    auto knobs=controls.removeFromLeft(330); gainLabel.setBounds(knobs.removeFromTop(18));gainSlider.setBounds(knobs.removeFromTop(100)); panLabel.setBounds(knobs.removeFromTop(18));panSlider.setBounds(knobs.removeFromTop(100));
    auto pitch=controls.removeFromLeft(130);pitchLabel.setBounds(pitch.removeFromTop(18));pitchSlider.setBounds(pitch.removeFromTop(100));
    auto fades=controls; fadeInLabel.setBounds(fades.removeFromTop(18));fadeInSlider.setBounds(fades.removeFromTop(45));fadeOutLabel.setBounds(fades.removeFromTop(18));fadeOutSlider.setBounds(fades.removeFromTop(45));
    waveformLabel.setBounds(r.removeFromTop(24));waveform.setBounds(r);
}

void RASamplerAudioProcessorEditor::timerCallback()
{
    const auto& s=processor.getSlot(selectedSlot); slotTitle.setText("SLOT "+juce::String(selectedSlot+1)+"  |  "+(s.loaded?s.name:"EMPTY"),juce::dontSendNotification);
    for(int i=0;i<8;++i){const auto& info=processor.getSlot(i);slotButtons[(size_t)i]->setButtonText("SLOT "+juce::String(i+1)+(info.loaded?"  •  "+info.name.substring(0,12):"  |  EMPTY"));}
    loopButton.setButtonText(s.loopEnabled?"LOOP: ON":"LOOP: OFF"); fadeInSlider.setValue(s.fadeIn,juce::dontSendNotification);fadeOutSlider.setValue(s.fadeOut,juce::dontSendNotification);waveform.repaint();
}

void RASamplerAudioProcessorEditor::selectSlot(int slot)
{
    selectedSlot=slot;gainSlider.setValue(processor.getSlotGain(slot),juce::dontSendNotification);timerCallback();repaint();
}

void RASamplerAudioProcessorEditor::loadSelectedSample()
{
    chooser=std::make_unique<juce::FileChooser>("Load audio sample",juce::File{},"*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3");
    chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[this](const juce::FileChooser& fc){auto file=fc.getResult();if(file.existsAsFile()){juce::String error;if(!processor.loadSample(selectedSlot,file,error))juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,"RA Sampler",error);else timerCallback();}});
}

void RASamplerAudioProcessorEditor::loadFactoryKit(int presetIndex)
{
    if(!juce::isPositiveAndBelow(presetIndex,presetNames.size()))return;
    const juce::String fileName = juce::StringArray { "RA_Core_Drum_Kit.rapreset", "RA_HipHop_Kit.rapreset", "RA_Electronic_Kit.rapreset", "RA_Afro_Percussion_Kit.rapreset", "RA_Minimal_Kit.rapreset" }[presetIndex];
    auto presetFile=factoryPresetDirectory.getChildFile(fileName);
    if(!presetFile.existsAsFile()){juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,"Factory Kit","Could not find factory preset: "+presetFile.getFullPathName());return;}
    auto parsed=juce::JSON::parse(presetFile.loadFileAsString()); if(!parsed.isObject()){juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,"Factory Kit","Preset file is invalid.");return;}
    auto* obj=parsed.getDynamicObject(); auto* entries=obj->getProperty("slots").getArray(); if(entries==nullptr)return;
    for(int i=0;i<juce::jmin(8,entries->size());++i){auto* item=(*entries)[i].getDynamicObject();if(item==nullptr)continue;auto samplePath=item->getProperty("sample").toString();auto sampleFile=presetFile.getParentDirectory().getChildFile(samplePath).getFullPathName();juce::String error;
        if(processor.loadSample(i,juce::File(sampleFile),error)){int root=(int)item->getProperty("root_note");processor.setSlotZone(i,root,(int)item->getProperty("midi_note"),(int)item->getProperty("midi_note"));processor.setSlotLoop(i,(bool)item->getProperty("loop"));}
    }
    selectSlot(0);
}

void RASamplerAudioProcessorEditor::updateEditFromWaveform()
{
    const auto& s=processor.getSlot(selectedSlot);processor.setSlotEdit(selectedSlot,s.start,s.end,s.loopStart,s.loopEnd,(float)fadeInSlider.getValue(),(float)fadeOutSlider.getValue());waveform.repaint();
}

void RASamplerAudioProcessorEditor::WaveformView::paint(juce::Graphics& g)
{
    const auto& skin=owner.getSkins()[(size_t)juce::jlimit(0,9,owner.selectedSkin)];auto col=[](juce::uint32 c){return juce::Colour(c);};
    auto bounds=getLocalBounds().toFloat();g.setColour(col(skin.panelAlt));g.fillRoundedRectangle(bounds,9);g.setColour(col(skin.border));g.drawRoundedRectangle(bounds,9,1);
    auto plot=bounds.reduced(14,20);g.setColour(col(skin.border).withAlpha(.5f));for(int i=0;i<=10;++i){float x=plot.getX()+plot.getWidth()*i/10.0f;g.drawVerticalLine((int)x,plot.getY(),plot.getBottom());}g.drawHorizontalLine((int)plot.getCentreY(),plot.getX(),plot.getRight());
    const auto& slot=owner.processor.getSlot(owner.selectedSlot);
    if(!slot.loaded||!slot.audio){g.setColour(col(skin.muted));g.drawText("LOAD A SAMPLE TO VIEW ITS WAVEFORM",getLocalBounds(),juce::Justification::centred);return;}
    const auto& audio=*slot.audio;int n=audio.getNumSamples();if(n<1)return;auto* data=audio.getReadPointer(0);juce::Path wave;const int pixels=juce::jmax(1,(int)plot.getWidth());
    for(int x=0;x<pixels;++x){int a=(int)((double)x/pixels*n),b=juce::jmax(a+1,(int)((double)(x+1)/pixels*n));float lo=1,hi=-1;for(int j=a;j<juce::jmin(b,n);++j){lo=juce::jmin(lo,data[j]);hi=juce::jmax(hi,data[j]);}float xx=plot.getX()+(float)x;float y1=plot.getCentreY()-hi*plot.getHeight()*.46f,y2=plot.getCentreY()-lo*plot.getHeight()*.46f;wave.startNewSubPath(xx,y1);wave.lineTo(xx,y2);}
    g.setColour(col(skin.accent).withAlpha(.95f));g.strokePath(wave,juce::PathStrokeType(1.15f));
    auto marker=[&](float norm,juce::Colour colour,float width){float x=plot.getX()+norm*plot.getWidth();g.setColour(colour);g.drawVerticalLine((int)x,plot.getY()-5,plot.getBottom()+5);juce::Path tri;tri.addTriangle(x-5,plot.getY()-8,x+5,plot.getY()-8,x,plot.getY()+1);g.fillPath(tri);};
    marker(slot.start,col(skin.accent),2);marker(slot.end,col(skin.accent),2);marker(slot.loopStart,juce::Colours::orange,1);marker(slot.loopEnd,juce::Colours::orange,1);
    float fi=slot.start+(slot.end-slot.start)*slot.fadeIn,fo=slot.end-(slot.end-slot.start)*slot.fadeOut;g.setColour(col(skin.accent).withAlpha(.16f));g.fillRect(plot.withRight(plot.getX()+fi*plot.getWidth()).withLeft(plot.getX()+slot.start*plot.getWidth()));g.fillRect(plot.withLeft(plot.getX()+fo*plot.getWidth()).withRight(plot.getX()+slot.end*plot.getWidth()));
    g.setColour(col(skin.muted));g.setFont(11);g.drawText("START",(int)(plot.getX()+slot.start*plot.getWidth()-24),2,50,14,juce::Justification::centred);g.drawText("END",(int)(plot.getX()+slot.end*plot.getWidth()-20),2,40,14,juce::Justification::centred);
}

void RASamplerAudioProcessorEditor::WaveformView::mouseDown(const juce::MouseEvent& e)
{
    const auto& s=owner.processor.getSlot(owner.selectedSlot);auto plot=getLocalBounds().toFloat().reduced(14,20);float norm=juce::jlimit(0.0f,1.0f,(e.position.x-plot.getX())/plot.getWidth());
    std::array<std::pair<Handle,float>,6> handles{{{start,s.start},{end,s.end},{fadeIn,s.start+(s.end-s.start)*s.fadeIn},{fadeOut,s.end-(s.end-s.start)*s.fadeOut},{loopStart,s.loopStart},{loopEnd,s.loopEnd}}};float best=.06f;active=none;for(auto h:handles){float d=std::abs(norm-h.second);if(d<best){best=d;active=h.first;}}
}
void RASamplerAudioProcessorEditor::WaveformView::mouseDrag(const juce::MouseEvent& e)
{
    if(active==none)return;auto s=owner.processor.getSlot(owner.selectedSlot);auto plot=getLocalBounds().toFloat().reduced(14,20);float v=juce::jlimit(0.0f,1.0f,(e.position.x-plot.getX())/plot.getWidth());float st=s.start,en=s.end,ls=s.loopStart,le=s.loopEnd,fi=s.fadeIn,fo=s.fadeOut;
    switch(active){case start:st=juce::jmin(v,en-.01f);break;case end:en=juce::jmax(v,st+.01f);break;case fadeIn:fi=juce::jlimit(0.0f,1.0f,(v-st)/(en-st));owner.fadeInSlider.setValue(fi,juce::dontSendNotification);break;case fadeOut:fo=juce::jlimit(0.0f,1.0f,(en-v)/(en-st));owner.fadeOutSlider.setValue(fo,juce::dontSendNotification);break;case loopStart:ls=juce::jlimit(st,le-.01f,v);break;case loopEnd:le=juce::jlimit(ls+.01f,en,v);break;default:break;}
    owner.processor.setSlotEdit(owner.selectedSlot,st,en,ls,le,fi,fo);repaint();
}
