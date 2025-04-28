#include "PluginEditor.h"
#include "PluginProcessor.h"

TestPluginAudioProcessorEditor::TestPluginAudioProcessorEditor(TestPluginAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Keep existing IR presets in the combo box
    irCombo.addItem("dark muffled dry hole narrow small very-reverberant", 1);
    irCombo.addItem("dark muffled dry hole narrow small", 2);
    irCombo.addItem("outside mountain trees rocks echo", 3);
    irCombo.addItem("outside tunnel courtyard park echo", 4);
    irCombo.addItem("small dry studio wooden floor", 5);
    irCombo.addItem("very-reverberant very-high very-large glass", 6);
    irCombo.addListener(this);
    addAndMakeVisible(irCombo);

    // Add the load IR button
    loadIRButton.addListener(this);
    addAndMakeVisible(loadIRButton);

    // Current IR display label
    currentIRLabel.setText("No custom IR loaded", juce::dontSendNotification);
    currentIRLabel.setJustificationType(juce::Justification::left);
    addAndMakeVisible(currentIRLabel);

    // Configure sliders (keep existing code)
    auto configureSlider = [&](juce::Slider& slider)
        {
            slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
            slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
            addAndMakeVisible(slider);
        };

    configureSlider(inputGainSlider);
    configureSlider(dryWetSlider);
    configureSlider(outputGainSlider);
    configureSlider(dampingSlider);

    // Configure labels below knobs
    inputGainLabel.setText("Input Gain", juce::dontSendNotification);
    inputGainLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(inputGainLabel);

    dryWetLabel.setText("Dry/Wet", juce::dontSendNotification);
    dryWetLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(dryWetLabel);

    outputGainLabel.setText("Output Gain", juce::dontSendNotification);
    outputGainLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(outputGainLabel);

    dampingLabel.setText("Damping", juce::dontSendNotification);
    dampingLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(dampingLabel);

    // Attachments to APVTS
    inputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "inputGain", inputGainSlider);
    dryWetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "dryWet", dryWetSlider);
    outputGainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "outputGain", outputGainSlider);
    dampingAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.apvts, "dampingFreq", dampingSlider);

    // in the constructor, after configureSlider(inputGainSlider);
    inputGainSlider.setRange(0.0, 2.0, 0.01);
    dryWetSlider.setRange(0.0, 1.0, 0.01);
    outputGainSlider.setRange(0.0, 2.0, 0.01);
    dampingSlider.setRange(100.0, 20000.0, 1.0);

    // also give each slider its initial value:
    inputGainSlider.setValue(1.0);
    dryWetSlider.setValue(0.5);
    outputGainSlider.setValue(1.0);
    dampingSlider.setValue(8000.0);


    setSize(500, 220);
}

TestPluginAudioProcessorEditor::~TestPluginAudioProcessorEditor()
{
    // Attachments will clean themselves up
}

void TestPluginAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF333333));
}

void TestPluginAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(20);

    // Layout knobs in a row with spacing
    const int numKnobs = 4;
    const int spacing = 30;
    int totalWidth = area.getWidth();
    int knobWidth = (totalWidth - spacing * (numKnobs - 1)) / numKnobs;
    int knobHeight = knobWidth;
    int labelHeight = 20;

    int x = area.getX();
    int y = area.getY();

    int pad = 80;

    // IR controls at the top
    auto irArea = area.removeFromTop(60);
    irCombo.setBounds(irArea.removeFromTop(30).removeFromLeft(200));
    loadIRButton.setBounds(irArea.removeFromLeft(150));
    currentIRLabel.setBounds(irArea);

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
}

void TestPluginAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged)
{
    if (comboBoxThatHasChanged == &irCombo)
    {
        int selectedID = irCombo.getSelectedId();

        // Only load preset IRs for IDs 1-4
        if (selectedID >= 1 && selectedID <= 4) {
            audioProcessor.loadImpulseResponseByID(selectedID);
        }
        // ID 5 is for custom IR, which is loaded via the file browser
    }
}

void TestPluginAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &loadIRButton)
    {
        // Create a file chooser for selecting IR files
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
                    // Load the selected IR file
                    audioProcessor.loadImpulseResponseFromFile(file);

                    // Update the UI
                    currentIRLabel.setText(file.getFileName(), juce::dontSendNotification);
                    irCombo.setSelectedId(5, juce::dontSendNotification); // Select "Custom IR"
                }
            });
    }
}
