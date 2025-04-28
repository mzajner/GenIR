#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
GenIRAudioProcessorEditor::GenIRAudioProcessorEditor(GenIRAudioProcessor& p)
    : AudioProcessorEditor(&p),
    audioProcessor(p),
    progress(0.0),
    progressBar(progress),
    keywordsTabs(juce::TabbedButtonBar::TabsAtTop),
    mainControlsPanel(juce::Colour(0xFF333333)),   // Création avec couleur spécifiée
    irGeneratorPanel(juce::Colour(0xFF333333))     // Création avec couleur spécifiée
{
    // Set editor size
    setSize(800, 650);

    // Setup navigation buttons
    setupNavigationButtons();
    
    // Set up main panels
    setupMainControls();
    setupIRGeneratorPanel();

    // Add panels to main editor
    addAndMakeVisible(mainControlsPanel);
    addAndMakeVisible(irGeneratorPanel);
    
    // Set initial visibility
    switchToPanel(0);

    // Configure timer
    startTimer(100);

    // Check if an IR is already loaded
    juce::String currentIR = audioProcessor.getCurrentIRFileName();
    if (currentIR.isNotEmpty())
    {
        currentIRLabel.setText(currentIR, juce::dontSendNotification);
    }
}

void GenIRAudioProcessorEditor::setupNavigationButtons()
{
    // Configure Main Controls button
    mainControlsButton.setButtonText("Main Controls");
    mainControlsButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
    mainControlsButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::grey);
    mainControlsButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    mainControlsButton.setToggleable(true);
    mainControlsButton.setToggleState(true, juce::dontSendNotification);
    mainControlsButton.addListener(this);
    addAndMakeVisible(mainControlsButton);
    
    // Configure IR Generator button
    irGeneratorButton.setButtonText("IR Generator");
    irGeneratorButton.setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
    irGeneratorButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::grey);
    irGeneratorButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    irGeneratorButton.setToggleable(true);
    irGeneratorButton.setToggleState(false, juce::dontSendNotification);
    irGeneratorButton.addListener(this);
    addAndMakeVisible(irGeneratorButton);
}

void GenIRAudioProcessorEditor::switchToPanel(int panelIndex)
{
    // Store current panel
    activePanel = panelIndex;
    
    // Update button states
    mainControlsButton.setToggleState(panelIndex == 0, juce::dontSendNotification);
    irGeneratorButton.setToggleState(panelIndex == 1, juce::dontSendNotification);
    
    // Update panel visibility
    mainControlsPanel.setVisible(panelIndex == 0);
    irGeneratorPanel.setVisible(panelIndex == 1);
    
    // Force repaint
    repaint();
}

void GenIRAudioProcessorEditor::setupMainControls()
{
    // Configure sliders
    auto configureSlider = [&](juce::Slider& slider)
        {
            slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
            mainControlsPanel.addAndMakeVisible(slider);
        };

    configureSlider(inputGainSlider);
    configureSlider(dryWetSlider);
    configureSlider(outputGainSlider);
    configureSlider(dampingSlider);

    // Configure labels
    inputGainLabel.setText("Input Gain", juce::dontSendNotification);
    inputGainLabel.setJustificationType(juce::Justification::centred);
    inputGainLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    mainControlsPanel.addAndMakeVisible(inputGainLabel);

    dryWetLabel.setText("Dry/Wet", juce::dontSendNotification);
    dryWetLabel.setJustificationType(juce::Justification::centred);
    dryWetLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    mainControlsPanel.addAndMakeVisible(dryWetLabel);

    outputGainLabel.setText("Output Gain", juce::dontSendNotification);
    outputGainLabel.setJustificationType(juce::Justification::centred);
    outputGainLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    mainControlsPanel.addAndMakeVisible(outputGainLabel);

    dampingLabel.setText("Damping", juce::dontSendNotification);
    dampingLabel.setJustificationType(juce::Justification::centred);
    dampingLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    mainControlsPanel.addAndMakeVisible(dampingLabel);

    // Attachments to sync slider values with the ValueTreeState
    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "inputGain", inputGainSlider);
    dryWetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "dryWet", dryWetSlider);
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "outputGain", outputGainSlider);
    dampingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "dampingFreq", dampingSlider);

    // Configure ranges and initial values for sliders
    inputGainSlider.setRange(0.0, 2.0, 0.01);
    dryWetSlider.setRange(0.0, 1.0, 0.01);
    outputGainSlider.setRange(0.0, 2.0, 0.01);
    dampingSlider.setRange(100.0, 20000.0, 1.0);

    // Set initial values
    inputGainSlider.setValue(1.0);
    dryWetSlider.setValue(0.5);
    outputGainSlider.setValue(1.0);
    dampingSlider.setValue(8000.0);

    // Keep existing IR presets in the combo box
    irCombo.addItem("IR1 - Big Hall", 1);
    irCombo.addItem("IR2 - Plate", 2);
    irCombo.addItem("IR3 - Tiny Room", 3);
    irCombo.addItem("IR4 - Cave", 4);
    irCombo.addItem("Custom IR", 5); // Add a custom option
    irCombo.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF444444));
    irCombo.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    irCombo.addListener(this);
    mainControlsPanel.addAndMakeVisible(irCombo);

    // Add IR loading button
    loadIRButton.setButtonText("Browse for IR...");
    loadIRButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF444444));
    loadIRButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    loadIRButton.addListener(this);
    mainControlsPanel.addAndMakeVisible(loadIRButton);

    // Current IR display label
    currentIRLabel.setText("No custom IR loaded", juce::dontSendNotification);
    currentIRLabel.setJustificationType(juce::Justification::left);
    currentIRLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    mainControlsPanel.addAndMakeVisible(currentIRLabel);
}

void GenIRAudioProcessorEditor::setupIRGeneratorPanel()
{
    // Server controls
    serverLabel.setText("Server URL:", juce::dontSendNotification);
    serverLabel.setJustificationType(juce::Justification::right);
    serverLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    irGeneratorPanel.addAndMakeVisible(serverLabel);

    serverUrlEdit.setMultiLine(false);
    serverUrlEdit.setText("https://86d451fde387122f93.gradio.live");
    serverUrlEdit.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xFF444444));
    serverUrlEdit.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    irGeneratorPanel.addAndMakeVisible(serverUrlEdit);

    connectButton.setButtonText("Connect");
    connectButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF444444));
    connectButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    connectButton.addListener(this);
    irGeneratorPanel.addAndMakeVisible(connectButton);

    // Prompt controls
    promptLabel.setText("Description:", juce::dontSendNotification);
    promptLabel.setJustificationType(juce::Justification::right);
    promptLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    irGeneratorPanel.addAndMakeVisible(promptLabel);

    promptEditor.setMultiLine(false);
    promptEditor.setText("large reverberant church stone walls");
    promptEditor.setTooltip("Describe the acoustic space with words separated by spaces (no commas)");
    promptEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xFF444444));
    promptEditor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    irGeneratorPanel.addAndMakeVisible(promptEditor);

    // Examples section
    examplesLabel.setText("Examples:", juce::dontSendNotification);
    examplesLabel.setJustificationType(juce::Justification::right);
    examplesLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    irGeneratorPanel.addAndMakeVisible(examplesLabel);

    examplesComboBox.addItem("large reverberant church stone walls", 1);
    examplesComboBox.addItem("small dry studio wooden floor", 2);
    examplesComboBox.addItem("concrete tunnel echo", 3);
    examplesComboBox.addItem("cathedral high ceiling marble floor", 4);
    examplesComboBox.addItem("bright reverberant concert hall", 5);
    examplesComboBox.addItem("dark muffled recording booth", 6);
    examplesComboBox.addItem("very-reverberant stone cave", 7);
    examplesComboBox.addItem("narrow brick passage echo", 8);
    examplesComboBox.addItem("high dome marble floor columns", 9);
    examplesComboBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xFF444444));
    examplesComboBox.setColour(juce::ComboBox::textColourId, juce::Colours::white);
    examplesComboBox.addListener(this);
    irGeneratorPanel.addAndMakeVisible(examplesComboBox);

    applyExampleButton.setButtonText("Apply Example");
    applyExampleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF444444));
    applyExampleButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    applyExampleButton.addListener(this);
    irGeneratorPanel.addAndMakeVisible(applyExampleButton);

    // Initialize example properties
    initializeExampleProperties();

    // Duration slider
    durationLabel.setText("Duration (s):", juce::dontSendNotification);
    durationLabel.setJustificationType(juce::Justification::right);
    durationLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    irGeneratorPanel.addAndMakeVisible(durationLabel);

    durationSlider.setRange(1.0, 20.0);
    durationSlider.setValue(5.0);
    durationSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    durationSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xFF444444));
    durationSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFF66AAFF));
    durationSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    durationSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xFF444444));
    irGeneratorPanel.addAndMakeVisible(durationSlider);

    // Steps slider
    stepsLabel.setText("Steps:", juce::dontSendNotification);
    stepsLabel.setJustificationType(juce::Justification::right);
    stepsLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    irGeneratorPanel.addAndMakeVisible(stepsLabel);

    stepsSlider.setRange(10.0, 100.0);
    stepsSlider.setValue(50.0);
    stepsSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    stepsSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xFF444444));
    stepsSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFF66AAFF));
    stepsSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    stepsSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xFF444444));
    irGeneratorPanel.addAndMakeVisible(stepsSlider);

    // Guidance slider
    guidanceLabel.setText("Guidance Scale:", juce::dontSendNotification);
    guidanceLabel.setJustificationType(juce::Justification::right);
    guidanceLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    irGeneratorPanel.addAndMakeVisible(guidanceLabel);

    guidanceSlider.setRange(1.2, 7.0);
    guidanceSlider.setValue(3.5);
    guidanceSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    guidanceSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xFF444444));
    guidanceSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFF66AAFF));
    guidanceSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    guidanceSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xFF444444));
    irGeneratorPanel.addAndMakeVisible(guidanceSlider);

    // Seed controls
    seedLabel.setText("Seed:", juce::dontSendNotification);
    seedLabel.setJustificationType(juce::Justification::right);
    seedLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    irGeneratorPanel.addAndMakeVisible(seedLabel);

    seedTextEditor.setMultiLine(false);
    seedTextEditor.setText("42");
    seedTextEditor.setInputRestrictions(6, "0123456789");
    seedTextEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xFF444444));
    seedTextEditor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    irGeneratorPanel.addAndMakeVisible(seedTextEditor);

    randomSeedToggle.setButtonText("Random Seed");
    randomSeedToggle.setToggleState(true, juce::dontSendNotification);
    randomSeedToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    randomSeedToggle.addListener(this);
    irGeneratorPanel.addAndMakeVisible(randomSeedToggle);

    // Disable seed control if random mode is activated
    seedTextEditor.setEnabled(!randomSeedToggle.getToggleState());

    // Generate button
    generateButton.setButtonText("Generate IR");
    generateButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF4CAF50));
    generateButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    generateButton.addListener(this);
    irGeneratorPanel.addAndMakeVisible(generateButton);

    // Setup progress bar
    progressBar.setColour(juce::ProgressBar::backgroundColourId, juce::Colour(0xFF333333));
    progressBar.setColour(juce::ProgressBar::foregroundColourId, juce::Colour(0xFF4CAF50));
    irGeneratorPanel.addAndMakeVisible(progressBar);

    // Status label
    statusLabel.setText("Ready", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    irGeneratorPanel.addAndMakeVisible(statusLabel);

    // Keywords 
    keywordsTabs.setTabBarDepth(30);
    keywordsTabs.setOutline(0);
    keywordsTabs.setColour(juce::TabbedComponent::backgroundColourId, juce::Colour(0xFF333333));
    keywordsTabs.setColour(juce::TabbedComponent::outlineColourId, juce::Colour(0xFF333333));
    irGeneratorPanel.addAndMakeVisible(keywordsTabs);

    // Initialize keyword categories
    initializeKeywordsCategories();
}

void GenIRAudioProcessorEditor::initializeExampleProperties()
{
    // Define recommended duration and guidance values for each example
    // Format: {duration in seconds, guidance scale}

    // Large spaces (10 seconds)
    exampleProperties["large reverberant church stone walls"] = { 10, 1.2f };
    exampleProperties["cathedral high ceiling marble floor"] = { 10, 1.2f };
    exampleProperties["high dome marble floor columns"] = { 10, 1.2f };

    // Medium spaces (4-5 seconds)
    exampleProperties["concrete tunnel echo"] = { 5, 1.2f };
    exampleProperties["bright reverberant concert hall"] = { 5, 1.2f };
    exampleProperties["very-reverberant stone cave"] = { 5, 1.2f };
    exampleProperties["narrow brick passage echo"] = { 4, 1.2f };

    // Small spaces (2-3 seconds)
    exampleProperties["small dry studio wooden floor"] = { 3, 1.2f };
    exampleProperties["dark muffled recording booth"] = { 2, 1.2f };
}

void GenIRAudioProcessorEditor::initializeKeywordsCategories()
{
    // Initialize keyword categories
    keywordsCategories["materials"] = {
        "wood", "stone", "concrete", "cut-stone", "grass", "pine", "rock", "canvas", "brick",
        "bricks", "marble", "carpet", "leather", "glass", "metallic", "dirt", "snow", "rocks",
        "trees", "floor", "metal"
    };

    keywordsCategories["spatial"] = {
        "large", "high", "small", "circular", "narrow", "very-high", "very-large", "complex",
        "above"
    };

    keywordsCategories["architecture"] = {
        "arch", "steeple", "stage", "dome", "column"
    };

    keywordsCategories["place_type"] = {
        "outside", "church", "studio", "forest", "tunnel", "amphitheater", "auditorium", "cave",
        "lobby", "courtyard", "park", "recording-booth", "courtroom", "mountain", "valley",
        "movie-theater", "building", "museum", "library", "movie-set", "chapel", "hill", "road",
        "passage", "lake", "work-room", "nuclear-reactor", "factory", "into", "compartment",
        "cliff", "gorge", "stairwell", "mausoleum", "mound", "hall", "court-room", "cathedral",
        "warehouse", "shed", "hole", "tower", "office", "tennis-court", "gymnasium"
    };

    keywordsCategories["acoustic_properties"] = {
        "dark", "reverberant", "muffled", "echo", "bright", "dry", "very-reverberant"
    };

    // Clear existing tabs
    keywordsTabs.clearTabs();
    keywordButtons.clear();

    // Create and add tabs for each category - with explicit background color
    auto* materialsTab = new ColorPanel(juce::Colour(0xFF333333));
    auto* spatialTab = new ColorPanel(juce::Colour(0xFF333333));
    auto* architectureTab = new ColorPanel(juce::Colour(0xFF333333));
    auto* placeTypeTab = new ColorPanel(juce::Colour(0xFF333333));
    auto* acousticPropertiesTab = new ColorPanel(juce::Colour(0xFF333333));

    // Add keywords to each tab
    addKeywordsToTab(materialsTab, keywordsCategories["materials"]);
    addKeywordsToTab(spatialTab, keywordsCategories["spatial"]);
    addKeywordsToTab(architectureTab, keywordsCategories["architecture"]);
    addKeywordsToTab(placeTypeTab, keywordsCategories["place_type"]);
    addKeywordsToTab(acousticPropertiesTab, keywordsCategories["acoustic_properties"]);

    keywordsTabs.addTab("Materials", juce::Colours::darkgrey, materialsTab, true);
    keywordsTabs.addTab("Spatial", juce::Colours::darkgrey, spatialTab, true);
    keywordsTabs.addTab("Architecture", juce::Colours::darkgrey, architectureTab, true);
    keywordsTabs.addTab("Place Type", juce::Colours::darkgrey, placeTypeTab, true);
    keywordsTabs.addTab("Acoustic", juce::Colours::darkgrey, acousticPropertiesTab, true);
}

void GenIRAudioProcessorEditor::addKeywordsToTab(juce::Component* tab, const juce::Array<juce::String>& keywords)
{
    if (tab == nullptr)
        return;
        
    // Button colors
    const juce::Colour buttonBgColor(0xFF2A2A2A);
    const juce::Colour buttonTextColor(0xFFFFFFFF);

    // Create a grid layout directly on the tab
    const int columns = 4;
    const int buttonHeight = 30;
    int tabWidth = tab->getWidth() > 0 ? tab->getWidth() : 600; // Default width if not yet sized
    const int buttonWidth = tabWidth / columns;

    // Calculate height needed for all buttons
    const int rows = (keywords.size() + columns - 1) / columns; // Round up

    // First remove any existing buttons
    tab->removeAllChildren();

    // Add buttons directly to the tab (no viewport)
    for (int i = 0; i < keywords.size(); ++i)
    {
        juce::TextButton* keywordButton = new juce::TextButton(keywords[i]);
        keywordButton->getProperties().set("keyword", keywords[i]);
        keywordButton->addListener(this);
        keywordButton->setColour(juce::TextButton::buttonColourId, buttonBgColor);
        keywordButton->setColour(juce::TextButton::textColourOffId, buttonTextColor);
        tab->addAndMakeVisible(keywordButton);

        const int row = i / columns;
        const int col = i % columns;
        keywordButton->setBounds(col * buttonWidth, row * buttonHeight, buttonWidth, buttonHeight);

        // Store the button so it doesn't get deleted
        keywordButtons.add(keywordButton);
    }

    // Set the tab size to fit all buttons
    tab->setSize(tabWidth, rows * buttonHeight);
}

void GenIRAudioProcessorEditor::onKeywordButtonClicked(juce::Button* button)
{
    juce::String keyword = button->getProperties()["keyword"];

    // Add keyword to prompt field with a space
    juce::String currentText = promptEditor.getText();

    if (currentText.isEmpty())
    {
        promptEditor.setText(keyword);
    }
    else if (!currentText.endsWithChar(' '))
    {
        promptEditor.setText(currentText + " " + keyword);
    }
    else
    {
        promptEditor.setText(currentText + keyword);
    }
}

void GenIRAudioProcessorEditor::startGeneration()
{
    // Disable button during generation
    generateButton.setEnabled(false);

    // Generation parameters
    juce::String prompt = promptEditor.getText();
    float duration = (float)durationSlider.getValue();
    int steps = (int)stepsSlider.getValue();
    float guidanceScale = (float)guidanceSlider.getValue();

    // Determine which seed to use
    int seed;
    if (randomSeedToggle.getToggleState())
    {
        // Generate random seed
        juce::Random random;
        seed = random.nextInt(1000000);

        // Update seed display for information
        seedTextEditor.setText(juce::String(seed), false);
    }
    else
    {
        // Use user-defined seed
        seed = seedTextEditor.getText().getIntValue();
    }

    // Start generation
    audioProcessor.generateTangoFluxIR(prompt, duration, steps, guidanceScale, seed);
}

void GenIRAudioProcessorEditor::updateGenerationStatus()
{
    // Update progress bar
    progress = audioProcessor.getTangoFluxProgress();
    progressBar.setVisible(audioProcessor.isTangoFluxGenerating());

    // Update status label
    statusLabel.setText(audioProcessor.getTangoFluxStatus(), juce::dontSendNotification);

    // Enable/disable generation button
    generateButton.setEnabled(!audioProcessor.isTangoFluxGenerating());
}

GenIRAudioProcessorEditor::~GenIRAudioProcessorEditor()
{
    stopTimer();
}

void GenIRAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Fill the background with a dark color
    g.fillAll(juce::Colour(0xFF333333));
    
    // Les panneaux ont maintenant leur propre méthode paint,
    // nous n'avons donc plus besoin de les peindre ici
}

void GenIRAudioProcessorEditor::resized()
{
    // Define sizes and positions
    auto area = getLocalBounds();
    
    // Navigation bar at top
    auto navBarArea = area.removeFromTop(30);
    int buttonWidth = navBarArea.getWidth() / 2;
    
    mainControlsButton.setBounds(navBarArea.removeFromLeft(buttonWidth));
    irGeneratorButton.setBounds(navBarArea);
    
    // Content area below nav bar
    auto contentArea = area;
    
    // Size the panels to match the content area
    mainControlsPanel.setBounds(contentArea);
    irGeneratorPanel.setBounds(contentArea);

    // Main Controls layout
    auto mainPanelArea = mainControlsPanel.getLocalBounds().reduced(20);

    // IR controls at top
    auto irArea = mainPanelArea.removeFromTop(60);
    irCombo.setBounds(irArea.removeFromTop(30).removeFromLeft(200));
    loadIRButton.setBounds(irArea.removeFromLeft(150));
    currentIRLabel.setBounds(irArea);

    // Layout of potentiometers in a row with spacing
    const int numKnobs = 4;
    const int spacing = 30;
    int totalWidth = mainPanelArea.getWidth();
    int knobWidth = (totalWidth - spacing * (numKnobs - 1)) / numKnobs;
    int knobHeight = knobWidth;
    int labelHeight = 20;

    int x = mainPanelArea.getX();
    int y = mainPanelArea.getY();

    int pad = 80;

    // Input Gain
    inputGainSlider.setBounds(x, y + pad, knobWidth, knobHeight);
    inputGainLabel.setBounds(x, y + knobHeight + pad, knobWidth, labelHeight);
    x += knobWidth + spacing;

    // Dry/Wet
    dryWetSlider.setBounds(x, y + pad, knobWidth, knobHeight);
    dryWetLabel.setBounds(x, y + knobHeight + pad, knobWidth, labelHeight);
    x += knobWidth + spacing;

    // Output Gain
    outputGainSlider.setBounds(x, y + pad, knobWidth, knobHeight);
    outputGainLabel.setBounds(x, y + knobHeight + pad, knobWidth, labelHeight);
    x += knobWidth + spacing;

    // Damping
    dampingSlider.setBounds(x, y + pad, knobWidth, knobHeight);
    dampingLabel.setBounds(x, y + knobHeight + pad, knobWidth, labelHeight);

    // IR Generator Panel Layout
    auto genArea = irGeneratorPanel.getLocalBounds().reduced(10);

    // Server URL
    auto serverArea = genArea.removeFromTop(30);
    serverLabel.setBounds(serverArea.removeFromLeft(100));
    serverUrlEdit.setBounds(serverArea.removeFromLeft(250));
    connectButton.setBounds(serverArea.removeFromLeft(100));

    genArea.removeFromTop(10);

    // Description
    auto promptArea = genArea.removeFromTop(30);
    promptLabel.setBounds(promptArea.removeFromLeft(100));
    promptEditor.setBounds(promptArea);

    genArea.removeFromTop(10);

    // Examples
    auto examplesArea = genArea.removeFromTop(30);
    examplesLabel.setBounds(examplesArea.removeFromLeft(100));
    applyExampleButton.setBounds(examplesArea.removeFromRight(120));
    examplesComboBox.setBounds(examplesArea);

    genArea.removeFromTop(20);

    // Slider controls
    auto durationArea = genArea.removeFromTop(30);
    durationLabel.setBounds(durationArea.removeFromLeft(100));
    durationSlider.setBounds(durationArea);

    genArea.removeFromTop(10);

    auto stepsArea = genArea.removeFromTop(30);
    stepsLabel.setBounds(stepsArea.removeFromLeft(100));
    stepsSlider.setBounds(stepsArea);

    genArea.removeFromTop(10);

    auto guidanceArea = genArea.removeFromTop(30);
    guidanceLabel.setBounds(guidanceArea.removeFromLeft(100));
    guidanceSlider.setBounds(guidanceArea);

    genArea.removeFromTop(10);

    // Seed
    auto seedArea = genArea.removeFromTop(30);
    seedLabel.setBounds(seedArea.removeFromLeft(100));
    randomSeedToggle.setBounds(seedArea.removeFromRight(150));
    seedTextEditor.setBounds(seedArea);

    genArea.removeFromTop(20);

    // Keywords tabs
    keywordsTabs.setBounds(genArea.removeFromTop(150));
    
    // After resizing keywordsTabs, refresh tab content if needed
    for (int i = 0; i < keywordsTabs.getNumTabs(); ++i)
    {
        if (juce::Component* tab = keywordsTabs.getTabContentComponent(i))
        {
            // Get keywords for this tab index
            juce::Array<juce::String> keywords;
            if (i == 0 && keywordsCategories.find("materials") != keywordsCategories.end())
                keywords = keywordsCategories["materials"];
            else if (i == 1 && keywordsCategories.find("spatial") != keywordsCategories.end())
                keywords = keywordsCategories["spatial"];
            else if (i == 2 && keywordsCategories.find("architecture") != keywordsCategories.end())
                keywords = keywordsCategories["architecture"];
            else if (i == 3 && keywordsCategories.find("place_type") != keywordsCategories.end())
                keywords = keywordsCategories["place_type"];
            else if (i == 4 && keywordsCategories.find("acoustic_properties") != keywordsCategories.end())
                keywords = keywordsCategories["acoustic_properties"];
                
            if (keywords.size() > 0)
                addKeywordsToTab(tab, keywords);
        }
    }

    genArea.removeFromTop(20);

    // Generation controls
    auto bottomArea = genArea.removeFromBottom(120);  // Augmentation de l'espace pour les contrôles du bas

    auto statusArea = bottomArea.removeFromBottom(25);
    statusLabel.setBounds(statusArea);

    auto progressArea = bottomArea.removeFromBottom(20);
    progressBar.setBounds(progressArea);

    bottomArea.removeFromBottom(15);

    // Bouton Generate - agrandi et mieux positionné
    generateButton.setBounds(bottomArea.reduced(40, 0));
    generateButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF4CAF50));
    generateButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
}

void GenIRAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &irCombo)
    {
        int selectedID = irCombo.getSelectedId();

        // Only load predefined IRs for IDs 1-4
        if (selectedID >= 1 && selectedID <= 4) {
            audioProcessor.loadImpulseResponseByID(selectedID);
            currentIRLabel.setText(audioProcessor.getCurrentIRFileName(), juce::dontSendNotification);
        }
        // ID 5 is for custom IR, loaded via file browser
    }
    else if (comboBoxThatHasChanged == &examplesComboBox)
    {
        // Do nothing here, handled by apply button
    }
}

void GenIRAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &mainControlsButton)
    {
        // Switch to Main Controls panel
        switchToPanel(0);
    }
    else if (button == &irGeneratorButton)
    {
        // Switch to IR Generator panel
        switchToPanel(1);
    }
    else if (button == &loadIRButton)
    {
        // Create a file chooser to select IR files
        fileChooser = std::make_unique<juce::FileChooser>(
            "Please select an impulse response file...",
            juce::File::getSpecialLocation(juce::File::userHomeDirectory),
            "*.wav;*.aif;*.aiff"
        );

        auto folderChooserFlags =
            juce::FileBrowserComponent::openMode |
            juce::FileBrowserComponent::canSelectFiles;

        fileChooser->launchAsync(folderChooserFlags, [this](const juce::FileChooser& chooser)
            {
                auto file = chooser.getResult();

                if (file.existsAsFile())
                {
                    // Load selected IR file
                    audioProcessor.loadImpulseResponseFromFile(file);

                    // Update user interface
                    currentIRLabel.setText(file.getFileName(), juce::dontSendNotification);
                    irCombo.setSelectedId(5, juce::dontSendNotification); // Select "Custom IR"
                }
            });
    }
    else if (button == &connectButton)
    {
        // Handle server connection
        juce::String url = serverUrlEdit.getText();
        if (url.isNotEmpty())
        {
            // Update server URL in audio processor
            audioProcessor.setTangoFluxServerUrl(url);
            statusLabel.setText("Connected to server: " + url, juce::dontSendNotification);
        }
        else
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                "Error", "Server URL cannot be empty");
        }
    }
    else if (button == &generateButton)
    {
        startGeneration();
    }
    else if (button == &randomSeedToggle)
    {
        // Enable/disable seed control based on checkbox state
        seedTextEditor.setEnabled(!randomSeedToggle.getToggleState());
    }
    else if (button == &applyExampleButton)
    {
        juce::String selectedExample = examplesComboBox.getText();

        // Set prompt text
        promptEditor.setText(selectedExample);

        // Set recommended duration and guidance if the example exists in our map
        auto it = exampleProperties.find(selectedExample);
        if (it != exampleProperties.end())
        {
            // Update duration
            durationSlider.setValue(it->second.recommendedDuration);

            // Reset guidance scale
            guidanceSlider.setValue(it->second.recommendedGuidance);
        }
        else
        {
            // Default values if example not found
            durationSlider.setValue(5.0);
            guidanceSlider.setValue(1.2);
        }
    }
    else
    {
        // Check if it's a keyword button
        if (button->getProperties().contains("keyword"))
        {
            onKeywordButtonClicked(button);
        }
    }
}

void GenIRAudioProcessorEditor::timerCallback()
{
    // Update IR label if needed
    juce::String currentIR = audioProcessor.getCurrentIRFileName();
    if (currentIR.isNotEmpty() && currentIR != currentIRLabel.getText())
    {
        currentIRLabel.setText(currentIR, juce::dontSendNotification);
        irCombo.setSelectedId(5, juce::dontSendNotification); // Select "Custom IR"
    }

    // Update generation status if needed
    updateGenerationStatus();
}