#include "IRGeneratorPanel.h"

IRGeneratorPanel::IRGeneratorPanel(GenIRAudioProcessor& p)
    : audioProcessor(p),
    keywordsTabs(juce::TabbedButtonBar::TabsAtTop),
    progress(0.0),
    progressBar(progress)
{
    // Make the panel opaque with explicit background color
    setOpaque(true);
    setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xFF333333));

    // Server controls
    serverLabel.setText("Server URL:", juce::dontSendNotification);
    serverLabel.setJustificationType(juce::Justification::right);
    serverLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(serverLabel);

    serverUrlEdit.setMultiLine(false);
    serverUrlEdit.setText("https://86d451fde387122f93.gradio.live");
    serverUrlEdit.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xFF444444));
    serverUrlEdit.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    addAndMakeVisible(serverUrlEdit);

    connectButton.setButtonText("Connect");
    connectButton.addListener(this);
    connectButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF444444));
    connectButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    addAndMakeVisible(connectButton);

    // Setup prompt editor
    promptLabel.setText("Description:", juce::dontSendNotification);
    promptLabel.setJustificationType(juce::Justification::right);
    promptLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(promptLabel);

    promptEditor.setMultiLine(false);
    promptEditor.setText("large reverberant church stone walls");
    promptEditor.setTooltip("Describe the acoustic space with words separated by spaces (no commas)");
    promptEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xFF444444));
    promptEditor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    addAndMakeVisible(promptEditor);

    // Setup duration controls
    durationLabel.setText("Duration (s):", juce::dontSendNotification);
    durationLabel.setJustificationType(juce::Justification::right);
    durationLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(durationLabel);

    durationSlider.setRange(1.0, 20.0);
    durationSlider.setValue(5.0);
    durationSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    durationSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xFF444444));
    durationSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFF66AAFF));
    durationSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    durationSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xFF444444));
    durationSlider.addListener(this);
    addAndMakeVisible(durationSlider);

    // Setup steps controls
    stepsLabel.setText("Steps:", juce::dontSendNotification);
    stepsLabel.setJustificationType(juce::Justification::right);
    stepsLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(stepsLabel);

    stepsSlider.setRange(10.0, 100.0);
    stepsSlider.setValue(50.0);
    stepsSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    stepsSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xFF444444));
    stepsSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFF66AAFF));
    stepsSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    stepsSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xFF444444));
    stepsSlider.addListener(this);
    addAndMakeVisible(stepsSlider);

    // Setup guidance controls
    guidanceLabel.setText("Guidance Scale:", juce::dontSendNotification);
    guidanceLabel.setJustificationType(juce::Justification::right);
    guidanceLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(guidanceLabel);

    guidanceSlider.setRange(1.2, 7.0);
    guidanceSlider.setValue(3.5);
    guidanceSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    guidanceSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xFF444444));
    guidanceSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xFF66AAFF));
    guidanceSlider.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    guidanceSlider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xFF444444));
    guidanceSlider.addListener(this);
    addAndMakeVisible(guidanceSlider);

    // Setup seed controls
    seedLabel.setText("Seed:", juce::dontSendNotification);
    seedLabel.setJustificationType(juce::Justification::right);
    seedLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(seedLabel);

    seedTextEditor.setMultiLine(false);
    seedTextEditor.setText("42");
    seedTextEditor.setInputRestrictions(6, "0123456789");
    seedTextEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xFF444444));
    seedTextEditor.setColour(juce::TextEditor::textColourId, juce::Colours::white);
    addAndMakeVisible(seedTextEditor);

    randomSeedToggle.setButtonText("Random Seed");
    randomSeedToggle.setToggleState(true, juce::dontSendNotification);
    randomSeedToggle.setColour(juce::ToggleButton::textColourId, juce::Colours::white);
    randomSeedToggle.addListener(this);
    addAndMakeVisible(randomSeedToggle);

    // Disable seed control if random mode is activated
    seedTextEditor.setEnabled(!randomSeedToggle.getToggleState());

    // Setup generation button
    generateButton.setButtonText("Generate IR");
    generateButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF4CAF50));
    generateButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    generateButton.addListener(this);
    addAndMakeVisible(generateButton);

    // Setup progress bar
    progressBar.setColour(juce::ProgressBar::backgroundColourId, juce::Colour(0xFF333333));
    progressBar.setColour(juce::ProgressBar::foregroundColourId, juce::Colour(0xFF4CAF50));
    progressBar.setVisible(false); // Hide initially
    addAndMakeVisible(progressBar);

    // Setup status label
    statusLabel.setText("Ready", juce::dontSendNotification);
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(statusLabel);

    // Setup predefined examples
    examplesLabel.setText("Examples:", juce::dontSendNotification);
    examplesLabel.setJustificationType(juce::Justification::right);
    examplesLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(examplesLabel);

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
    addAndMakeVisible(examplesComboBox);

    applyExampleButton.setButtonText("Apply Example");
    applyExampleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xFF444444));
    applyExampleButton.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    applyExampleButton.addListener(this);
    addAndMakeVisible(applyExampleButton);

    // Initialize example properties
    initializeExampleProperties();

    // Setup tabs for keywords
    keywordsTabs.setTabBarDepth(25);
    keywordsTabs.setOutline(0);
    keywordsTabs.setColour(juce::TabbedComponent::backgroundColourId, juce::Colour(0xFF333333));
    keywordsTabs.setColour(juce::TabbedComponent::outlineColourId, juce::Colour(0xFF333333));
    addAndMakeVisible(keywordsTabs);

    // Initialize keyword categories
    initializeKeywordsCategories();

    // Start timer to update status
    startTimer(100);
}

IRGeneratorPanel::~IRGeneratorPanel()
{
    stopTimer();
}

void IRGeneratorPanel::paint(juce::Graphics& g)
{
    // Fill with the same color as the main application background
    g.fillAll(juce::Colour(0xFF333333));
}

void IRGeneratorPanel::resized()
{
    auto area = getLocalBounds().reduced(10);

    // Server URL
    auto serverArea = area.removeFromTop(30);
    serverLabel.setBounds(serverArea.removeFromLeft(100));
    serverUrlEdit.setBounds(serverArea.removeFromLeft(250));
    connectButton.setBounds(serverArea.removeFromLeft(100));

    area.removeFromTop(10);

    // Description
    auto promptArea = area.removeFromTop(30);
    promptLabel.setBounds(promptArea.removeFromLeft(100));
    promptEditor.setBounds(promptArea);

    area.removeFromTop(10);

    // Examples
    auto examplesArea = area.removeFromTop(30);
    examplesLabel.setBounds(examplesArea.removeFromLeft(100));
    applyExampleButton.setBounds(examplesArea.removeFromRight(120));
    examplesComboBox.setBounds(examplesArea);

    area.removeFromTop(20);

    // Parameter sliders
    auto createControlRow = [&](juce::Label& label, juce::Component& control, int height = 30)
    {
        auto rowArea = area.removeFromTop(height);
        label.setBounds(rowArea.removeFromLeft(100));
        control.setBounds(rowArea);
        area.removeFromTop(10);
    };

    createControlRow(durationLabel, durationSlider);
    createControlRow(stepsLabel, stepsSlider);
    createControlRow(guidanceLabel, guidanceSlider);

    // Seed and random seed toggle
    auto seedArea = area.removeFromTop(30);
    seedLabel.setBounds(seedArea.removeFromLeft(100));
    randomSeedToggle.setBounds(seedArea.removeFromRight(150));
    seedTextEditor.setBounds(seedArea);

    area.removeFromTop(20);

    // Keyword tabs - make them take up less vertical space
    keywordsTabs.setBounds(area.removeFromTop(150));
    
    // Important: After resizing the tabs component, update any tab content components 
    // This ensures the tab content components have the correct size
    for (int i = 0; i < keywordsTabs.getNumTabs(); ++i)
    {
        juce::Component* tabContent = keywordsTabs.getTabContentComponent(i);
        if (tabContent != nullptr)
        {
            // Update tab buttons after resizing - using find() instead of contains() for C++ compatibility
            if (i == 0 && keywordsCategories.find("materials") != keywordsCategories.end())
                addKeywordsToTab(tabContent, keywordsCategories["materials"]);
            else if (i == 1 && keywordsCategories.find("spatial") != keywordsCategories.end())
                addKeywordsToTab(tabContent, keywordsCategories["spatial"]);
            else if (i == 2 && keywordsCategories.find("architecture") != keywordsCategories.end())
                addKeywordsToTab(tabContent, keywordsCategories["architecture"]);
            else if (i == 3 && keywordsCategories.find("place_type") != keywordsCategories.end())
                addKeywordsToTab(tabContent, keywordsCategories["place_type"]);
            else if (i == 4 && keywordsCategories.find("acoustic_properties") != keywordsCategories.end())
                addKeywordsToTab(tabContent, keywordsCategories["acoustic_properties"]);
        }
    }

    area.removeFromTop(20);

    // Generation controls at the bottom
    auto bottomArea = area.removeFromBottom(80);

    auto statusArea = bottomArea.removeFromBottom(25);
    statusLabel.setBounds(statusArea);

    auto progressArea = bottomArea.removeFromBottom(20);
    progressBar.setBounds(progressArea);

    bottomArea.removeFromBottom(10);

    generateButton.setBounds(bottomArea.reduced(50, 0));
}

void IRGeneratorPanel::buttonClicked(juce::Button* button)
{
    if (button == &generateButton)
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
    else
    {
        // Check if it's a keyword button
        if (button->getProperties().contains("keyword"))
        {
            onKeywordButtonClicked(button);
        }
    }
}

void IRGeneratorPanel::onKeywordButtonClicked(juce::Button* button)
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

void IRGeneratorPanel::sliderValueChanged(juce::Slider* slider)
{
    // Handle slider value changes if needed
    juce::ignoreUnused(slider);
}

void IRGeneratorPanel::comboBoxChanged(juce::ComboBox* comboBox)
{
    // Handle combo box changes if needed
    juce::ignoreUnused(comboBox);
}

void IRGeneratorPanel::startGeneration()
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

void IRGeneratorPanel::updateGenerationStatus()
{
    // Update progress bar
    progress = audioProcessor.getTangoFluxProgress();
    progressBar.setVisible(audioProcessor.isTangoFluxGenerating());

    // Update status label
    statusLabel.setText(audioProcessor.getTangoFluxStatus(), juce::dontSendNotification);

    // Enable/disable generation button
    generateButton.setEnabled(!audioProcessor.isTangoFluxGenerating());
}

void IRGeneratorPanel::timerCallback()
{
    updateGenerationStatus();
}

void IRGeneratorPanel::initializeExampleProperties()
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

void IRGeneratorPanel::initializeKeywordsCategories()
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

    // Clear existing tabs and buttons
    keywordsTabs.clearTabs();
    keywordButtons.clear();

    // Configure keyword tabs component
    keywordsTabs.setTabBarDepth(25);
    keywordsTabs.setOutline(0);
    keywordsTabs.setColour(juce::TabbedComponent::backgroundColourId, juce::Colour(0xFF333333));
    keywordsTabs.setColour(juce::TabbedComponent::outlineColourId, juce::Colour(0xFF333333));

    // Create tabs for each category and ensure they're properly owned
    auto* materialsTab = new juce::Component();
    materialsTab->setOpaque(true);
    materialsTab->setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xFF333333));

    auto* spatialTab = new juce::Component();
    spatialTab->setOpaque(true);
    spatialTab->setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xFF333333));

    auto* architectureTab = new juce::Component();
    architectureTab->setOpaque(true);
    architectureTab->setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xFF333333));

    auto* placeTypeTab = new juce::Component();
    placeTypeTab->setOpaque(true);
    placeTypeTab->setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xFF333333));

    auto* acousticPropertiesTab = new juce::Component();
    acousticPropertiesTab->setOpaque(true);
    acousticPropertiesTab->setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xFF333333));

    // Add each tab to the component - ownership IS transferred here
    keywordsTabs.addTab("Materials", juce::Colours::darkgrey, materialsTab, true);
    keywordsTabs.addTab("Spatial", juce::Colours::darkgrey, spatialTab, true);
    keywordsTabs.addTab("Architecture", juce::Colours::darkgrey, architectureTab, true);
    keywordsTabs.addTab("Place Type", juce::Colours::darkgrey, placeTypeTab, true);
    keywordsTabs.addTab("Acoustic", juce::Colours::darkgrey, acousticPropertiesTab, true);
    
    // IMPORTANT: Add buttons AFTER the tabs are added to the component
    // This ensures each tab is properly sized before we add buttons
    addKeywordsToTab(keywordsTabs.getTabContentComponent(0), keywordsCategories["materials"]);
    addKeywordsToTab(keywordsTabs.getTabContentComponent(1), keywordsCategories["spatial"]);
    addKeywordsToTab(keywordsTabs.getTabContentComponent(2), keywordsCategories["architecture"]);
    addKeywordsToTab(keywordsTabs.getTabContentComponent(3), keywordsCategories["place_type"]);
    addKeywordsToTab(keywordsTabs.getTabContentComponent(4), keywordsCategories["acoustic_properties"]);
}

void IRGeneratorPanel::addKeywordsToTab(juce::Component* tab, const juce::Array<juce::String>& keywords)
{
    if (tab == nullptr)
        return;
        
    // Make sure tab is opaque with correct background
    tab->setOpaque(true);
    tab->setColour(juce::ResizableWindow::backgroundColourId, juce::Colour(0xFF333333));
    
    // Button colors
    const juce::Colour buttonBgColor(0xFF2A2A2A);
    const juce::Colour buttonTextColor(0xFFFFFFFF);
    
    // Create a grid layout for buttons
    const int columns = 4;
    const int buttonHeight = 30;
    
    // Use the actual width or default to 600px
    int tabWidth = tab->getWidth() > 0 ? tab->getWidth() : 600;
    const int buttonWidth = tabWidth / columns;

    // Calculate rows needed (round up)
    const int rows = (keywords.size() + columns - 1) / columns;
    
    // First, remove any existing buttons from this tab
    tab->removeAllChildren();

    // Create new buttons and add them to the tab
    for (int i = 0; i < keywords.size(); ++i)
    {
        // Create a new button for this keyword
        juce::TextButton* keywordButton = new juce::TextButton(keywords[i]);
        keywordButton->getProperties().set("keyword", keywords[i]);
        keywordButton->addListener(this);
        keywordButton->setColour(juce::TextButton::buttonColourId, buttonBgColor);
        keywordButton->setColour(juce::TextButton::textColourOffId, buttonTextColor);
        
        // Calculate position in grid
        const int row = i / columns;
        const int col = i % columns;
        keywordButton->setBounds(col * buttonWidth, row * buttonHeight, buttonWidth, buttonHeight);
        
        // Add to the tab component
        tab->addAndMakeVisible(keywordButton);
        
        // Store button in our owned array so it doesn't get deleted
        keywordButtons.add(keywordButton);
    }
    
    // Set the tab content size to fit all buttons
    tab->setSize(tabWidth, rows * buttonHeight);
}