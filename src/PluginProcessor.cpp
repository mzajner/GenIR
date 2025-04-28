#include "PluginProcessor.h"
#include "PluginEditor.h"

TestPluginAudioProcessor::TestPluginAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    )
#endif
{
    // Setup your convolution
    auto& convolution = processorChain.get<convIndex>();
    juce::File defaultIR("C:\\Users\\mikez\\Desktop\\Ai-Builds\\ir-generator\\impulseResponses\\IR1.wav");
    if (!defaultIR.existsAsFile())
        DBG("Default IR does NOT exist: " << defaultIR.getFullPathName());
    else
        DBG("Loaded default IR: " << defaultIR.getFileName());

    // Load the new IR:
    convolution.loadImpulseResponse(
        defaultIR,
        juce::dsp::Convolution::Stereo::yes,
        juce::dsp::Convolution::Trim::no,
        0
    );

    // Initialize the dry/wet mixer
    auto& mixer = processorChain.get<mixerIndex>();
    mixer.setWetMixProportion(0.5f);  // Default 50% wet

    // Set up the low-pass filter
    auto& lpf = processorChain.get<lpfIndex>();
    *lpf.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(44100.0, 8000.0);
}

TestPluginAudioProcessor::~TestPluginAudioProcessor()
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
TestPluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>("inputGain", "Input Gain", 0.0f, 2.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dryWet", "Dry/Wet", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("outputGain", "Output Gain", 0.0f, 2.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dampingFreq", "Damping Freq", 100.0f, 20000.0f, 8000.0f));
    return { params.begin(), params.end() };
}

// IR Loading
void TestPluginAudioProcessor::loadImpulseResponseByID(int irID)
{
    juce::File impulseFile;

    switch (irID)
    {
    case 1:
        impulseFile = juce::File("C:\\Users\\mikez\\Desktop\\Ai-Builds\\ir-generator\\impulseResponses\\IR1.wav");
        break;
    case 2:
        impulseFile = juce::File("C:\\Users\\mikez\\Desktop\\Ai-Builds\\ir-generator\\impulseResponses\\IR2.wav");
        break;
    case 3:
        impulseFile = juce::File("C:\\Users\\mikez\\Desktop\\Ai-Builds\\ir-generator\\impulseResponses\\IR3.wav");
        break;
    case 4:
        impulseFile = juce::File("C:\\Users\\mikez\\Desktop\\Ai-Builds\\ir-generator\\impulseResponses\\IR4.wav");
		break;
    case 5:
        impulseFile = juce::File("C:\\Users\\mikez\\Desktop\\Ai-Builds\\ir-generator\\impulseResponses\\IR5.wav");
        break;
    case 6:
        impulseFile = juce::File("C:\\Users\\mikez\\Desktop\\Ai-Builds\\ir-generator\\impulseResponses\\IR6.wav");
        break;
    default:
        DBG("Unknown IR ID: " << irID);
        return;
    }

    if (!impulseFile.existsAsFile())
    {
        DBG("IR file does NOT exist: " << impulseFile.getFullPathName());
        return;
    }

    // Grab the convolution from our chain:
    auto& convolution = processorChain.get<convIndex>();

    // Load the new IR:
    convolution.loadImpulseResponse(
        impulseFile,
        juce::dsp::Convolution::Stereo::yes,
        juce::dsp::Convolution::Trim::no,
        0
    );

    DBG("Loaded IR: " << impulseFile.getFileName());
}

// Add this to your PluginProcessor.cpp file
void TestPluginAudioProcessor::loadImpulseResponseFromFile(const juce::File& file)
{
    if (!file.existsAsFile())
    {
        DBG("IR file does NOT exist: " << file.getFullPathName());
        return;
    }

    // Grab the convolution from our chain:
    auto& convolution = processorChain.get<convIndex>();

    // Load the new IR:
    convolution.loadImpulseResponse(
        file,
        juce::dsp::Convolution::Stereo::yes,
        juce::dsp::Convolution::Trim::no,
        0
    );

    // Store the file for preset saving later
    lastLoadedIRFile = file;

    DBG("Loaded custom IR: " << file.getFileName());
}

//==============================================================================
const juce::String TestPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool TestPluginAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool TestPluginAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool TestPluginAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double TestPluginAudioProcessor::getTailLengthSeconds() const
{
    // For a reverb, you might want a longer tail estimate
    return 10.0;
}

int TestPluginAudioProcessor::getNumPrograms() { return 1; }
int TestPluginAudioProcessor::getCurrentProgram() { return 0; }
void TestPluginAudioProcessor::setCurrentProgram(int) {}
const juce::String TestPluginAudioProcessor::getProgramName(int) { return {}; }
void TestPluginAudioProcessor::changeProgramName(int, const juce::String&) {}

//==============================================================================
void TestPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
    spec.numChannels = (juce::uint32)getTotalNumOutputChannels();

    processorChain.prepare(spec);

    // Set an initial cutoff for the low-pass filter from the APVTS
    auto& lpfFilter = processorChain.get<lpfIndex>();

    if (auto* raw = apvts.getRawParameterValue("dampingFreq"))
    {
        float defaultFreq = raw->load();
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, defaultFreq);
        *lpfFilter.coefficients = *coeffs;
    }

    // Initialize dry/wet mix
    auto& mixer = processorChain.get<mixerIndex>();
    if (auto* mixParam = apvts.getRawParameterValue("dryWet"))
    {
        mixer.setWetMixProportion(mixParam->load());
    }
}

void TestPluginAudioProcessor::releaseResources()
{
    // Not strictly needed for convolution, but we implement anyway
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool TestPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // Only allow mono or stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

#if !JucePlugin_IsSynth
    // Must match input layout
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void TestPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    const int numInputCh = getTotalNumInputChannels();
    const int numOutputCh = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    // (0) clear unused channels
    for (int ch = numInputCh; ch < numOutputCh; ++ch)
        buffer.clear(ch, 0, numSamples);

    // (1) pull all parameter values once
    auto inAP = apvts.getRawParameterValue("inputGain");
    auto dwAP = apvts.getRawParameterValue("dryWet");
    auto outAP = apvts.getRawParameterValue("outputGain");
    auto dmpAP = apvts.getRawParameterValue("dampingFreq");

    float inGain = inAP ? inAP->load() : 1.0f;
    float dryWet = dwAP ? dwAP->load() : 0.5f;
    float outGain = outAP ? outAP->load() : 1.0f;
    float cutoffHz = dmpAP ? dmpAP->load() : 8000.0f;

    // Create audio block from the buffer
    juce::dsp::AudioBlock<float> block(buffer);

    // Apply input gain
    block.multiplyBy(inGain);

    // Store dry samples in the mixer
    auto& mixer = processorChain.get<mixerIndex>();
    mixer.pushDrySamples(block);

    // Process wet signal (convolution + lpf)
    juce::dsp::ProcessContextReplacing<float> context(block);

    // Update the damping filter coefficients
    auto& lpf = processorChain.get<lpfIndex>();
    *lpf.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(getSampleRate(), cutoffHz);

    // Process through convolution and filter
    processorChain.get<convIndex>().process(context);
    processorChain.get<lpfIndex>().process(context);

    // Set mix ratio and mix wet samples with stored dry samples
    mixer.setWetMixProportion(dryWet);
    mixer.mixWetSamples(block);

    // Apply output gain
    block.multiplyBy(outGain);
}

//==============================================================================
bool TestPluginAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* TestPluginAudioProcessor::createEditor()
{
    return new TestPluginAudioProcessorEditor(*this);
}

//==============================================================================
void TestPluginAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Store parameters
    auto state = apvts.copyState();

    // Add the custom IR path if one has been loaded
    if (lastLoadedIRFile.existsAsFile())
    {
        state.setProperty("customIRPath", lastLoadedIRFile.getFullPathName(), nullptr);
    }

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void TestPluginAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Restore parameters
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType()))
    {
        juce::ValueTree newState = juce::ValueTree::fromXml(*xml);
        apvts.replaceState(newState);

        // Check if we have a custom IR path to load
        if (newState.hasProperty("customIRPath"))
        {
            juce::String irPath = newState.getProperty("customIRPath");
            juce::File irFile(irPath);

            if (irFile.existsAsFile())
            {
                loadImpulseResponseFromFile(irFile);
            }
        }
    }
}

void TestPluginAudioProcessor::reset()
{
    processorChain.reset();
}

// This creates the plugin instance and is called by the host.
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new TestPluginAudioProcessor();
}