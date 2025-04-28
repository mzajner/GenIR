#pragma once

#include <JuceHeader.h>

//==============================================================================
class TestPluginAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    TestPluginAudioProcessor();
    ~TestPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

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
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // 1) Declare the method that the Editor will call:
    void loadImpulseResponseByID(int irID);

    // Add a new method to load IRs from any file
    void loadImpulseResponseFromFile(const juce::File& file);

    void reset() override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts{ *this, nullptr, "PARAMS",
                                              createParameterLayout() };

private:
    enum
    {
        convIndex,  // Index 0
        lpfIndex,   // Index 1
        mixerIndex  // Index 2
    };

    // Convolution + IIR filter for damping + DryWet mixer
    juce::dsp::ProcessorChain<
        juce::dsp::Convolution,
        juce::dsp::IIR::Filter<float>,
        juce::dsp::DryWetMixer<float>
    > processorChain;

    // Instead of using a simple IIR::Filter
    using Filter = juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
        juce::dsp::IIR::Coefficients<float>>;

    juce::File lastLoadedIRFile;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestPluginAudioProcessor)
};