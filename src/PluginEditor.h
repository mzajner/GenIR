#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class TestPluginAudioProcessorEditor
    : public juce::AudioProcessorEditor,
    private juce::ComboBox::Listener,
    private juce::Button::Listener // Add Button listener
{
public:
    TestPluginAudioProcessorEditor(TestPluginAudioProcessor&);
    ~TestPluginAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    TestPluginAudioProcessor& audioProcessor;

    // Sliders  
    juce::Slider inputGainSlider, dryWetSlider, outputGainSlider, dampingSlider;
    juce::Label inputGainLabel, dryWetLabel, outputGainLabel, dampingLabel;

    // Attachments to sync slider values with the ValueTreeState  
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dryWetAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputGainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> dampingAttachment;

    // IR selection controls
    juce::ComboBox irCombo;
    juce::TextButton loadIRButton{ "Browse for IR..." };
    juce::Label currentIRLabel;

    // File chooser
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Implement listener methods
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked(juce::Button* button) override; // Add this method

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TestPluginAudioProcessorEditor)
};