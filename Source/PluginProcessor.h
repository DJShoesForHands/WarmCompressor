/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

enum Slope
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

//data struct of EQ settings
//extract EQ params out of AudioProcessorValueTreeState (where the sliders are)
struct EQChainSettings
{
    float peakFreq {0}, peakGainInDecibels{0}, peakQ {1.f};
    float lowCutFreq {0}, highCutFreq {0};
    Slope lowCutSlope {Slope::Slope_12}, highCutSlope {Slope::Slope_12};
};
EQChainSettings getEQChainSettings(juce::AudioProcessorValueTreeState& apvts);

//==============================================================================
/**
*/
class WarmCompressorAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    WarmCompressorAudioProcessor();
    ~WarmCompressorAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout();
    juce::AudioProcessorValueTreeState apvts {*this, nullptr, "Parameters", createParameterLayout()};

private:
    //setup filter type alias
    using Filter = juce::dsp::IIR::Filter<float>;
    
    // Chain 4 filters - each JUCE filter is configured with 12dB/Oct slope
    // need 12dB/oct * 4 filters to get up to max 48db/Oct slope for EQ
    using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;
    
    using FilterChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;
    
    FilterChain leftEQ, rightEQ;
    
    enum EQPositions
    {
        LowCut,
        Peak,
        HighCut
    };
    
    void updatePeakFilter(const EQChainSettings& eqchainSettings);
    
    using Coefficients = Filter::CoefficientsPtr;
    //static function declaration for updating peak filter coeffs
    static void updateCoefficients(Coefficients& old, const Coefficients& replacements);
    
    template<typename EQChainType, typename CoefficientType>
    
    void updateCutFilter(EQChainType& leftLowCut,
                         const CoefficientType& cutCoeffs,
                         const Slope& lowCutSlope)
    {
        leftLowCut.template setBypassed<0>(true);
        leftLowCut.template setBypassed<1>(true);
        leftLowCut.template setBypassed<2>(true);
        leftLowCut.template setBypassed<3>(true);
        
        //switch(eqChainSettings.lowCutSlope)
        switch(lowCutSlope)
        {
            //remeber to dereference pointers to objects
            case Slope_12:
                *leftLowCut.template get<0>().coefficients = *cutCoeffs[0];
                leftLowCut.template setBypassed<0>(false);
                break;
            case Slope_24:
                *leftLowCut.template get<0>().coefficients = *cutCoeffs[0];
                leftLowCut.template setBypassed<0>(false);
                *leftLowCut.template get<1>().coefficients = *cutCoeffs[1];
                leftLowCut.template setBypassed<1>(false);
                break;
            case Slope_36:
                *leftLowCut.template get<0>().coefficients = *cutCoeffs[0];
                leftLowCut.template setBypassed<0>(false);
                *leftLowCut.template get<1>().coefficients = *cutCoeffs[1];
                leftLowCut.template setBypassed<1>(false);
                *leftLowCut.template get<2>().coefficients = *cutCoeffs[2];
                leftLowCut.template setBypassed<2>(false);
                break;
            case Slope_48:
                *leftLowCut.template get<0>().coefficients = *cutCoeffs[0];
                leftLowCut.template setBypassed<0>(false);
                *leftLowCut.template get<1>().coefficients = *cutCoeffs[1];
                leftLowCut.template setBypassed<1>(false);
                *leftLowCut.template get<2>().coefficients = *cutCoeffs[2];
                leftLowCut.template setBypassed<2>(false);
                *leftLowCut.template get<3>().coefficients = *cutCoeffs[3];
                leftLowCut.template setBypassed<3>(false);
                break;
        }
    }
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WarmCompressorAudioProcessor)
};
