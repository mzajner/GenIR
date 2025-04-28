#include "PluginProcessor.h"
#include "PluginEditor.h"

GenIRAudioProcessor::GenIRAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
#endif
    progressValue(0.0f), // Renomme de generationProgress
    isGenerating(false)
{
    // Creer le repertoire des IRs si necessaire
    createDefaultIRDirectories();

    // Initialiser le client TangoFlux
    tangoFluxClient = std::make_unique<TangoFluxClient>();
    tangoFluxClient->addListener(this);

    // Initialiser les IRs par defaut
    initializeDefaultIRs();

    // Configurer la convolution avec l'IR par defaut
    auto& convolution = processorChain.get<convIndex>();
    if (lastLoadedIRFile.existsAsFile())
    {
        convolution.loadImpulseResponse(
            lastLoadedIRFile,
            juce::dsp::Convolution::Stereo::yes,
            juce::dsp::Convolution::Trim::no,
            0
        );

        DBG("Loaded default IR: " + lastLoadedIRFile.getFileName());
    }
    else
    {
        DBG("No default IR found.");
    }

    // Initialiser le dry/wet mixer
    auto& mixer = processorChain.get<mixerIndex>();
    mixer.setWetMixProportion(0.5f);  // Par defaut 50% wet

    // Configurer le filtre passe-bas
    auto& lpf = processorChain.get<lpfIndex>();
    *lpf.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(44100.0, 8000.0);
}

GenIRAudioProcessor::~GenIRAudioProcessor()
{
    tangoFluxClient->removeListener(this);
}

void GenIRAudioProcessor::createDefaultIRDirectories()
{
    // Repertoire pour les IRs par defaut (a cote du plugin)
    juce::File pluginFile = juce::File::getSpecialLocation(juce::File::currentApplicationFile);
    currentIRDirectory = pluginFile.getParentDirectory().getChildFile("IRs");

    if (!currentIRDirectory.exists())
        currentIRDirectory.createDirectory();

    // Repertoire temporaire pour les IRs generees par TangoFlux
    // Utilise le dossier temporaire de l'utilisateur courant
    tempIRDirectory = juce::File::getSpecialLocation(juce::File::tempDirectory).getChildFile("GenIR");

    if (!tempIRDirectory.exists())
        tempIRDirectory.createDirectory();

    DBG("IR directory: " + currentIRDirectory.getFullPathName());
    DBG("Temp IR directory: " + tempIRDirectory.getFullPathName());
}

void GenIRAudioProcessor::initializeDefaultIRs()
{
    // Verifier s'il y a des IRs dans le repertoire des IRs
    juce::Array<juce::File> irFiles;
    currentIRDirectory.findChildFiles(irFiles, juce::File::findFiles, false, "*.wav");

    // Si aucun fichier IR n'est trouve, utiliser un IR par defaut integre dans les ressources binaires
    if (irFiles.isEmpty())
    {
        // Ici, nous pourrions charger un IR par defaut a partir des ressources binaires
        // Pour l'instant, on utilise simplement une note dans le log
        DBG("No IR files found in directory: " + currentIRDirectory.getFullPathName());

        // Creer un IR par defaut tres simple (comme un dirac)
        juce::File defaultIR = currentIRDirectory.getChildFile("IR1.wav");

        // OPTIONNEL: Creer un fichier IR de base si necessaire
        // Cette partie serait normalement remplacee par des IRs precharges
    }
    else
    {
        // Utiliser le premier fichier IR trouve comme IR par defaut
        lastLoadedIRFile = irFiles[0];
        DBG("Using default IR: " + lastLoadedIRFile.getFileName());
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout
GenIRAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<juce::AudioParameterFloat>("inputGain", "Input Gain", 0.0f, 2.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dryWet", "Dry/Wet", 0.0f, 1.0f, 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("outputGain", "Output Gain", 0.0f, 2.0f, 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("dampingFreq", "Damping Freq", 100.0f, 20000.0f, 8000.0f));
    return { params.begin(), params.end() };
}

// Chargement d'IR
void GenIRAudioProcessor::loadImpulseResponseByID(int irID)
{
    juce::File impulseFile;

    switch (irID)
    {
    case 1:
        impulseFile = currentIRDirectory.getChildFile("IR1.wav");
        break;
    case 2:
        impulseFile = currentIRDirectory.getChildFile("IR2.wav");
        break;
    case 3:
        impulseFile = currentIRDirectory.getChildFile("IR3.wav");
        break;
    case 4:
        impulseFile = currentIRDirectory.getChildFile("IR4.wav");
        break;
    default:
        DBG("Unknown IR ID: " + juce::String(irID));
        return;
    }

    if (!impulseFile.existsAsFile())
    {
        DBG("IR file does NOT exist: " + impulseFile.getFullPathName());
        return;
    }

    // Recuperer la convolution de notre chaine
    auto& convolution = processorChain.get<convIndex>();

    // Charger le nouvel IR
    convolution.loadImpulseResponse(
        impulseFile,
        juce::dsp::Convolution::Stereo::yes,
        juce::dsp::Convolution::Trim::no,
        0
    );

    // Stocker le fichier pour une utilisation ulterieure
    lastLoadedIRFile = impulseFile;

    DBG("Loaded IR: " + impulseFile.getFileName());
}

void GenIRAudioProcessor::loadImpulseResponseFromFile(const juce::File& file)
{
    if (!file.existsAsFile())
    {
        DBG("IR file does NOT exist: " + file.getFullPathName());
        return;
    }

    // Recuperer la convolution de notre chaine
    auto& convolution = processorChain.get<convIndex>();

    // Charger le nouvel IR
    convolution.loadImpulseResponse(
        file,
        juce::dsp::Convolution::Stereo::yes,
        juce::dsp::Convolution::Trim::no,
        0
    );

    // Stocker le fichier pour une utilisation ulterieure
    lastLoadedIRFile = file;

    DBG("Loaded custom IR: " + file.getFileName());
}

juce::String GenIRAudioProcessor::getCurrentIRFileName() const
{
    return lastLoadedIRFile.getFileName();
}

// Methodes TangoFlux
void GenIRAudioProcessor::generateTangoFluxIR(const juce::String& prompt, float duration,
    int steps, float guidanceScale, int seed)
{
    if (isGenerating)
        return; // Ne pas demarrer une nouvelle generation si une est deja en cours

    isGenerating = true;
    progressValue = 0.0f; // Renomme de generationProgress

    // Preparer les parametres
    TangoFluxClient::GenerationParams params;
    params.prompt = prompt;
    params.duration = duration;
    params.steps = steps;
    params.guidanceScale = guidanceScale;
    params.seed = seed;

    // Creer un nom de fichier unique base sur l'horodatage
    juce::String timestamp = juce::Time::getCurrentTime().formatted("%Y%m%d_%H%M%S");
    juce::File outputFile = tempIRDirectory.getChildFile("GenIR_" + timestamp + ".wav");

    // Demarrer la generation
    tangoFluxClient->generateIR(params, outputFile);
}

juce::String GenIRAudioProcessor::getTangoFluxStatus() const
{
    return tangoFluxClient->getStatusMessage();
}

float GenIRAudioProcessor::getTangoFluxProgress() const
{
    return progressValue; // Renomme de generationProgress
}

bool GenIRAudioProcessor::isTangoFluxGenerating() const
{
    return isGenerating;
}

void GenIRAudioProcessor::setTangoFluxServerUrl(const juce::String& url)
{
    tangoFluxServerUrl = url;
    tangoFluxClient->setServerUrl(url);
}

// Callbacks TangoFluxClient::Listener
void GenIRAudioProcessor::generationCompleted(const juce::File& irFile)
{
    // Charger l'IR genere
    loadImpulseResponseFromFile(irFile);
    isGenerating = false;
    progressValue = 1.0f; // Renomme de generationProgress

    // Option possible: copier le fichier dans le repertoire des IRs
    // juce::File destFile = currentIRDirectory.getChildFile(irFile.getFileName());
    // irFile.copyFileTo(destFile);
}

void GenIRAudioProcessor::generationFailed(const juce::String& errorMessage)
{
    // Utiliser le parametre errorMessage au lieu de l'ignorer
    DBG("Generation failed: " + errorMessage);
    isGenerating = false;
    progressValue = 0.0f;
}

void GenIRAudioProcessor::generationProgress(float progressPercentage)
{
    progressValue = progressPercentage; // Renomme de generationProgress
}

//==============================================================================
const juce::String GenIRAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool GenIRAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool GenIRAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool GenIRAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double GenIRAudioProcessor::getTailLengthSeconds() const
{
    // Pour une reverberation, on veut une estimation de queue plus longue
    return 10.0;
}

int GenIRAudioProcessor::getNumPrograms() { return 1; }
int GenIRAudioProcessor::getCurrentProgram() { return 0; }
void GenIRAudioProcessor::setCurrentProgram(int) {}
const juce::String GenIRAudioProcessor::getProgramName(int) { return {}; }
void GenIRAudioProcessor::changeProgramName(int, const juce::String&) {}

//==============================================================================
void GenIRAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
    spec.numChannels = (juce::uint32)getTotalNumOutputChannels();

    processorChain.prepare(spec);

    // Definir une frequence de coupure initiale pour le filtre passe-bas depuis l'APVTS
    auto& lpfFilter = processorChain.get<lpfIndex>();

    if (auto* raw = apvts.getRawParameterValue("dampingFreq"))
    {
        float defaultFreq = raw->load();
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, defaultFreq);
        *lpfFilter.coefficients = *coeffs;
    }

    // Initialiser le mix dry/wet
    auto& mixer = processorChain.get<mixerIndex>();
    if (auto* mixParam = apvts.getRawParameterValue("dryWet"))
    {
        mixer.setWetMixProportion(mixParam->load());
    }
}

void GenIRAudioProcessor::releaseResources()
{
    // Pas strictement necessaire pour la convolution, mais on implemente quand meme
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool GenIRAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // Autoriser uniquement mono ou stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

#if !JucePlugin_IsSynth
    // Doit correspondre a la disposition d'entree
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void GenIRAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& midiMessages)
{
    // Indiquer que le parametre midiMessages est intentionnellement non utilise
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;

    const int numInputCh = getTotalNumInputChannels();
    const int numOutputCh = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    // (0) effacer les canaux inutilises
    for (int ch = numInputCh; ch < numOutputCh; ++ch)
        buffer.clear(ch, 0, numSamples);

    // (1) recuperer toutes les valeurs de parametres une fois
    auto inAP = apvts.getRawParameterValue("inputGain");
    auto dwAP = apvts.getRawParameterValue("dryWet");
    auto outAP = apvts.getRawParameterValue("outputGain");
    auto dmpAP = apvts.getRawParameterValue("dampingFreq");

    float inGain = inAP ? inAP->load() : 1.0f;
    float dryWet = dwAP ? dwAP->load() : 0.5f;
    float outGain = outAP ? outAP->load() : 1.0f;
    float cutoffHz = dmpAP ? dmpAP->load() : 8000.0f;

    // Creer un bloc audio a partir du buffer
    juce::dsp::AudioBlock<float> block(buffer);

    // Appliquer le gain d'entree
    block.multiplyBy(inGain);

    // Stocker les echantillons secs dans le mixer
    auto& mixer = processorChain.get<mixerIndex>();
    mixer.pushDrySamples(block);

    // Traiter le signal humide (convolution + lpf)
    juce::dsp::ProcessContextReplacing<float> context(block);

    // Mettre a jour les coefficients du filtre d'amortissement
    auto& lpf = processorChain.get<lpfIndex>();
    *lpf.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(getSampleRate(), cutoffHz);

    // Traiter a travers la convolution et le filtre
    processorChain.get<convIndex>().process(context);
    processorChain.get<lpfIndex>().process(context);

    // Definir le ratio de mix et melanger les echantillons humides avec les echantillons secs stockes
    mixer.setWetMixProportion(dryWet);
    mixer.mixWetSamples(block);

    // Appliquer le gain de sortie
    block.multiplyBy(outGain);
}

//==============================================================================
bool GenIRAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* GenIRAudioProcessor::createEditor()
{
    return new GenIRAudioProcessorEditor(*this);
}

//==============================================================================
void GenIRAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    // Stocker les parametres
    auto state = apvts.copyState();

    // Ajouter le chemin IR personnalise si un a ete charge
    if (lastLoadedIRFile.existsAsFile())
    {
        state.setProperty("customIRPath", lastLoadedIRFile.getFullPathName(), nullptr);
    }

    // Ajouter les parametres TangoFlux
    state.setProperty("tangoFluxURL", tangoFluxClient->getServerUrl(), nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void GenIRAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    // Restaurer les parametres
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml.get() != nullptr && xml->hasTagName(apvts.state.getType()))
    {
        juce::ValueTree newState = juce::ValueTree::fromXml(*xml);
        apvts.replaceState(newState);

        // Verifier si nous avons un chemin IR personnalise a charger
        if (newState.hasProperty("customIRPath"))
        {
            juce::String irPath = newState.getProperty("customIRPath");
            juce::File irFile(irPath);

            if (irFile.existsAsFile())
            {
                loadImpulseResponseFromFile(irFile);
            }
        }

        // Restaurer les parametres TangoFlux
        if (newState.hasProperty("tangoFluxURL"))
        {
            juce::String url = newState.getProperty("tangoFluxURL");
            tangoFluxClient->setServerUrl(url);
        }
    }
}

void GenIRAudioProcessor::reset()
{
    processorChain.reset();
}

// Ceci cree l'instance du plugin et est appele par l'hote.
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GenIRAudioProcessor();
}