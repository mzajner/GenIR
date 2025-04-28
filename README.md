# GenIR

### What is GenIR?

GenIR is an application that uses machine lerning to generate simulated spaces from text descriptions. The application remotly creates Impulse Responses (IRs) that are immediately processed by an integrated convolution algorithm, allowing you to instantly simulate acoustic environments such as churches, studios, tunnels, caves, and much more. Since the generation is made on a distant server, you have to be connected to Internet in order to use it.




### Basic Usage

#### Creating a Reverb

1. **Describe the acoustic space** - Use the "Description" field, select keywords from the tabs down or chose an example description
2. **Adjust parameters** - Modify the duration, number of steps, and other parameters as needed
3. **Click "Generate"** - Wait for the processing to complete
4. **Run a song through it** - Play your song and listen to the resultant reverberation, you can alternatively listen to and download the IR alone




### Detailed Interface

#### Server Section (only for dev)
- **Server URL**: GenIR server address (pre-configured, only modify if necessary)
- **Connect**: Button to connect to the server


#### Audio Generation Parameters
- **Description**: Text describing the acoustic space (e.g., "large reverberant church stone walls")
- **Duration**: Length of the impulse response (1-20 seconds)
- **Steps**: Number of generation steps (recommended : between 25 and 50)
- **Guidance Scale**: Theoretical influence of the description on the result (recommended : 1.2)
- **Seed**: Abstract value that defines the starting point of generation. if you keep exactly the same parameters and seed, you will get the same result.
- **Random Seed**: Checkbox to generate a random seed each time (recommended : on)

#### Keywords by Category
The application offers a library of keywords organized into 5 categories:

1. **Materials**: wood, stone, concrete, marble, etc.
2. **Spatial** (Spatial characteristics): large, high, small, narrow, etc.
3. **Architecture** (Architectural elements): arch, steeple, dome, column, etc.
4. **Place Type**: church, studio, tunnel, cave, etc.
5. **Acoustic Properties**: reverberant, muffled, echo, bright, etc.

Click on any word to automatically add it to your description.


#### Predefined Examples
The application includes several examples of acoustic spaces with pre-defined parameters. Select one from the dropdown list and click "Apply Example" to add it to your description.


#### Audio Player
- **Generate**: Starts audio generation
- **Play**: Plays the generated audio
- **Stop**: Stops playback
- **Download**: Saves the IR and its metadata in the "download" folder next to where you installed the app. that's useful if you wanna keep it and use it in other contexts or applications
- **Volume**: Controls playback volume



### Usage Tips

1. **Start with examples** - Use the predefined examples to familiarize yourself with possible results

2. **Combine different categories** - A good prompt typically includes:
   - An acoustic property (e.g., "reverberant")
   - A spatial characteristic (e.g., "large")
   - A place type (e.g., "church")
   - One or more materials (e.g., "stone walls")

3. **Use Appropriate duration according to space**:

4. **Experiment with Guidance Scale and steps** - Those values are hard to get to grips with, so try differents setups

5. **Use random seeds** - Unless you have a specific reason, always keep random seeds ticked if you wanna get differents results
 


#### Audio Plugin 

Prerequisites:
CMake: Minimum version 3.5.
JUCE: Clone or download JUCE and note its path.
Compiler: A C++17-compatible compiler (Visual Studio 2019+, Xcode 11+, GCC 9+, or Clang 10+).

Setup:
Clone the repo and get JUCE:
    mkdir build && cd build
    cmake -G "<Your IDE>" -DJUCE_PATH=../JUCE ..
    cmake --build . --config Release

JUCE_PATH should point to the top-level JUCE folder.
The plugin will be output in the default VST3/AU folders (set by COPY_PLUGIN_AFTER_BUILD).

Plugin Parameters:
    Input Gain: Scales incoming signal
    Dry/Wet: Mix between dry and wet
    Output Gain: Final output level
    Damping Freq: Low-pass cutoff on wet path

###Troubleshooting

**Be sure to have a good internet connexion** - The plugin won't work without it

 


### Metadata
When you download audio, a metadata text file is also created. It contains all the parameters used for generation, allowing you to recreate exactly the same sound later.




---

*GenIR - The space between realities*
