#pragma once

#include <JuceHeader.h>
#include "TangoFluxClient.h"

//==============================================================================
class GenIRAudioProcessor : public juce::AudioProcessor,
    private TangoFluxClient::Listener
{
public:
    //==============================================================================
    GenIRAudioProcessor();
    ~GenIRAudioProcessor() override;

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

    // Methodes de gestion des IRs
    void loadImpulseResponseByID(int irID);
    void loadImpulseResponseFromFile(const juce::File& file);
    juce::String getCurrentIRFileName() const;

    // Methodes specifiques a TangoFlux
    void generateTangoFluxIR(const juce::String& prompt, float duration,
        int steps, float guidanceScale, int seed);
    juce::String getTangoFluxStatus() const;
    float getTangoFluxProgress() const;
    bool isTangoFluxGenerating() const;
    void setTangoFluxServerUrl(const juce::String& url);

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

    // IR files storage
    juce::File lastLoadedIRFile;
    juce::File currentIRDirectory;

    // TangoFlux client
    std::unique_ptr<TangoFluxClient> tangoFluxClient;
    juce::File tempIRDirectory;
    float progressValue; // Renomme de generationProgress pour eviter le conflit
    bool isGenerating;
    juce::String tangoFluxServerUrl = "https://86d451fde387122f93.gradio.live";

    // Implementation des methodes de TangoFluxClient::Listener
    void generationCompleted(const juce::File& irFile) override;
    void generationFailed(const juce::String& errorMessage) override;
    void generationProgress(float progressPercentage) override;

    // Methodes privees
    void initializeDefaultIRs();
    void createDefaultIRDirectories();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GenIRAudioProcessor)
};