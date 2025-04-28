#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

// Classes de panneaux personnalisés avec arrière-plan
class ColorPanel : public juce::Component
{
public:
    ColorPanel(juce::Colour color) : backgroundColor(color) 
    {
        setOpaque(true);
    }
    
    void paint(juce::Graphics& g) override
    {
        g.fillAll(backgroundColor);
    }
    
private:
    juce::Colour backgroundColor;
};

// Main plugin editor
class GenIRAudioProcessorEditor : public juce::AudioProcessorEditor,
    private juce::ComboBox::Listener,
    private juce::Button::Listener,
    private juce::Timer
{
public:
    GenIRAudioProcessorEditor(GenIRAudioProcessor&);
    ~GenIRAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;

private:
    GenIRAudioProcessor& audioProcessor;

    // Navigation buttons
    juce::TextButton mainControlsButton;
    juce::TextButton irGeneratorButton;
    int activePanel = 0; // 0 = Main Controls, 1 = IR Generator

    // Main control panel - now using ColorPanel
    ColorPanel mainControlsPanel;

    // IR Generator panel - now using ColorPanel
    ColorPanel irGeneratorPanel;

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
    juce::TextButton loadIRButton;
    juce::Label currentIRLabel;

    // File chooser
    std::unique_ptr<juce::FileChooser> fileChooser;

    // Server controls
    juce::Label serverLabel;
    juce::TextEditor serverUrlEdit;
    juce::TextButton connectButton;

    // Generator UI components
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
    juce::ProgressBar progressBar;
    juce::Label statusLabel;
    double progress;

    // Examples
    juce::Label examplesLabel;
    juce::ComboBox examplesComboBox;
    juce::TextButton applyExampleButton;

    // Keywords
    juce::TabbedComponent keywordsTabs;
    std::map<juce::String, juce::Array<juce::String>> keywordsCategories;

    // Store keyword buttons so they don't get deleted
    juce::OwnedArray<juce::TextButton> keywordButtons;

    // Helper methods
    void setupMainControls();
    void setupIRGeneratorPanel();
    void setupNavigationButtons();
    void switchToPanel(int panelIndex);
    void initializeExampleProperties();
    void initializeKeywordsCategories();
    void addKeywordsToTab(juce::Component* tab, const juce::Array<juce::String>& keywords);
    void onKeywordButtonClicked(juce::Button* button);
    void startGeneration();
    void updateGenerationStatus();

    // Listener methods
    void comboBoxChanged(juce::ComboBox* comboBoxThatHasChanged) override;
    void buttonClicked(juce::Button* button) override;

    // Structure for example properties
    struct SpaceProperties {
        int recommendedDuration;
        float recommendedGuidance;
    };
    std::map<juce::String, SpaceProperties> exampleProperties;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(GenIRAudioProcessorEditor)
};