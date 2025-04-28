#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// Component for IR Generator controls
class IRGeneratorPanel : public juce::Component,
    private juce::Button::Listener,
    private juce::Slider::Listener,
    private juce::ComboBox::Listener,
    private juce::Timer
{
public:
    IRGeneratorPanel(GenIRAudioProcessor&);
    ~IRGeneratorPanel() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    GenIRAudioProcessor& audioProcessor;

    // Server controls
    juce::Label serverLabel;
    juce::TextEditor serverUrlEdit;
    juce::TextButton connectButton;

    // Generation interface
    juce::Label promptLabel;
    juce::TextEditor promptEditor;

    juce::Label durationLabel;
    juce::Slider durationSlider;

    juce::Label stepsLabel;
    juce::Slider stepsSlider;

    juce::Label guidanceLabel;
    juce::Slider guidanceSlider;

    juce::Label seedLabel;
    juce::TextEditor seedTextEditor;
    juce::ToggleButton randomSeedToggle;

    juce::TextButton generateButton;
    double progress; // Progress bar variable
    juce::ProgressBar progressBar;
    juce::Label statusLabel;

    // Predefined examples
    juce::Label examplesLabel;
    juce::ComboBox examplesComboBox;
    juce::TextButton applyExampleButton;

    // Keyword categories
    juce::TabbedComponent keywordsTabs;
    std::map<juce::String, juce::Array<juce::String>> keywordsCategories;
    juce::OwnedArray<juce::TextButton> keywordButtons;

    // Interface methods
    void buttonClicked(juce::Button* button) override;
    void sliderValueChanged(juce::Slider* slider) override;
    void comboBoxChanged(juce::ComboBox* comboBox) override;

    // IR generation methods
    void startGeneration();
    void updateGenerationStatus();

    // Keyword methods
    void initializeKeywordsCategories();
    void addKeywordsToTab(juce::Component* tab, const juce::Array<juce::String>& keywords);
    void onKeywordButtonClicked(juce::Button* button);

    // Structure for example space properties
    struct SpaceProperties {
        int recommendedDuration;
        float recommendedGuidance;
    };

    // Initialize example properties
    void initializeExampleProperties();
    std::map<juce::String, SpaceProperties> exampleProperties;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IRGeneratorPanel)
};