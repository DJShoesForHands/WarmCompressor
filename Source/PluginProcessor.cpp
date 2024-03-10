/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
WarmCompressorAudioProcessor::WarmCompressorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

WarmCompressorAudioProcessor::~WarmCompressorAudioProcessor()
{
}

//==============================================================================
const juce::String WarmCompressorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool WarmCompressorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool WarmCompressorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool WarmCompressorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double WarmCompressorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int WarmCompressorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int WarmCompressorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void WarmCompressorAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String WarmCompressorAudioProcessor::getProgramName (int index)
{
    return {};
}

void WarmCompressorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void WarmCompressorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    
    juce::dsp::ProcessSpec spec;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = 1;
    spec.sampleRate = sampleRate;
    
    leftEQ.prepare(spec);
    rightEQ.prepare(spec);
    
    updateFilters();

}

void WarmCompressorAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool WarmCompressorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void WarmCompressorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    updateFilters();
    
    // create audio block representing individual channel
    // and context wrapper around each block
    // pass context to EQ filter chain
    juce::dsp::AudioBlock<float> block(buffer);
  
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);
    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);
    leftEQ.process(leftContext);
    rightEQ.process(rightContext);
    
    
}

//==============================================================================
bool WarmCompressorAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* WarmCompressorAudioProcessor::createEditor()
{
    // return new WarmCompressorAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void WarmCompressorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void WarmCompressorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}
//load parameters from GUI into filters
EQChainSettings getEQChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    EQChainSettings settings;
    //apvts.getParameter("LowCut Freq")->getValue(); do NOT use this method, returns normalized value
    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Freq")->load();
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Freq")->load();
    settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
    settings.peakGainInDecibels = apvts.getRawParameterValue("Peak Gain")->load();
    settings.peakQ = apvts.getRawParameterValue("Peak Q Factor")->load();
    //need to cast to Slope data type
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LowCut Slope")->load());
    settings.highCutSlope= static_cast<Slope>(apvts.getRawParameterValue("HighCut Slope")->load());
    return settings;
}

void WarmCompressorAudioProcessor::updatePeakFilter(const EQChainSettings &eqChainSettings)
{
    auto peakCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(),
                                                                          eqChainSettings.peakFreq,
                                                                          eqChainSettings.peakQ,
                                                                          juce::Decibels::decibelsToGain(eqChainSettings.peakGainInDecibels));
   
    updateCoefficients(leftEQ.get<EQPositions::Peak>().coefficients, peakCoeffs);
    updateCoefficients(rightEQ.get<EQPositions::Peak>().coefficients, peakCoeffs);
    
}

void WarmCompressorAudioProcessor::updateCoefficients(Coefficients &old, const Coefficients &replacements)
{
    //reference counted objects allocated on heap - dereference to get underlying object
    *old = *replacements;
}

void WarmCompressorAudioProcessor::updateLowCutFilters(const EQChainSettings &eqChainSettings)
{
    auto lowCutCoeffs = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(eqChainSettings.lowCutFreq,
                                                                                                 getSampleRate(),
                                                                                                 (eqChainSettings.lowCutSlope + 1) * 2);
    auto& leftLowCut = leftEQ.get<EQPositions::LowCut>();
    auto& rightLowCut = rightEQ.get<EQPositions::LowCut>();
    
    updateCutFilter(rightLowCut, lowCutCoeffs, eqChainSettings.lowCutSlope);
    updateCutFilter(leftLowCut, lowCutCoeffs, eqChainSettings.lowCutSlope);
}

void WarmCompressorAudioProcessor::updateHighCutFilters(const EQChainSettings &eqChainSettings)
{
    auto highCutCoeffs = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(eqChainSettings.highCutFreq,
                                                                                                 getSampleRate(),
                                                                                                 (eqChainSettings.highCutSlope + 1) * 2);
    auto& leftHighCut = leftEQ.get<EQPositions::HighCut>();
    auto& rightHighCut = rightEQ.get<EQPositions::HighCut>();
    
    updateCutFilter(rightHighCut, highCutCoeffs, eqChainSettings.highCutSlope);
    updateCutFilter(leftHighCut, highCutCoeffs, eqChainSettings.highCutSlope);
}

void WarmCompressorAudioProcessor::updateFilters()
{
    auto eqChainSettings = getEQChainSettings(apvts);
    updatePeakFilter(eqChainSettings);
    updateLowCutFilters(eqChainSettings);
    updateHighCutFilters(eqChainSettings);
}

juce::AudioProcessorValueTreeState::ParameterLayout
    WarmCompressorAudioProcessor::createParameterLayout()
{
        juce::AudioProcessorValueTreeState::ParameterLayout layout;
        //low cut freq range of 20Hz to 20kHz, default of 20Hz step size of 1Hz
        layout.add(std::make_unique<juce::AudioParameterFloat>("LowCut Freq",
                                                               "LowCut Freq",
                                                               juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20.f));
        //high cut freq range of 20Hz to 20kHz, default of 20kHz step size of 1Hz
        layout.add(std::make_unique<juce::AudioParameterFloat>("HighCut Freq",
                                                               "HighCut Freq",
                                                               juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20000.f));
        //peak freq range of 20Hz to 20kHz, default of 750Hz step size of 1Hz
        layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Freq",
                                                               "Peak Freq",
                                                               juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 750.f));
        // gain with range of -24dB to 24dB, default value of 0 w/ step of 0.5dB
        layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Gain",
                                                               "Peak Gain",
                                                               juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f), 0.0f));
        // q with range of 0.1 to 10, steps of 0.05 w/ default of 1
        layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Q Factor",
                                                               "Peak Q Factor",
                                                               juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f), 1.f));
        // create string array of 12, 24, 36, 48 db/Oct
        juce::StringArray stringArray;
        for( int i = 0; i < 4; ++i )
        {
            juce::String str;
            str << (12 + i*12);
            str << " db/Oct";
            stringArray.add(str);
        }
        //add parameter choice for high/low cut slope in db per octave - defined by stringArray
        layout.add(std::make_unique<juce::AudioParameterChoice>("LowCut Slope", "LowCut Slope", stringArray, 0));
        layout.add(std::make_unique<juce::AudioParameterChoice>("HighCut Slope", "HighCut Slope", stringArray, 0));
        
        return layout;
};

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WarmCompressorAudioProcessor();
}
