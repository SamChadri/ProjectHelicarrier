README: Project Helicarrier

🎵 Overview
This project is a Digital Audio Workstation (DAW) application developed using the JUCE (Jules' Utility Class Extensions) C++ framework. The application's core functionality includes a custom audio engine, a simple polyphonic synthesizer, and a step sequencer for note and beat management.

✨ Features
Custom Audio Engine (EngineAudioSource): Manages the main audio processing loop, mixing various audio sources, including the synthesizer and metronome.

Simple Polyphonic Synthesizer (SynthAudioSource): A basic synthesizer that can generate audio from incoming MIDI note messages. It supports note on/off events to produce sound.

Step Sequencer (StepEditor): A graphical component for creating and editing musical patterns. It handles the logic for a step-based grid, allowing users to input and visualize notes or beats.

Metronome (Metronome): Provides a simple click track, synchronized with the project's tempo, to aid in recording and playback.

Data Management (Data): A centralized class that holds key project state information, such as tempo and metronome settings, making it accessible across different components.

🛠️ Architecture
The application is structured into several key classes that manage different aspects of the DAW's functionality.

MainComponent.h / MainComponent.cpp: The primary GUI component of the application. It contains and manages other components like the StepEditor and handles user interactions.

EngineAudioSource.h / EngineAudioSource.cpp: This is the heart of the audio processing. It inherits from JUCE's AudioSource and uses an internal AudioSourceChannelInfo buffer to process and mix audio from its sub-sources. It handles the logic for playback, including starting and stopping the transport.

SynthAudioSource.h / SynthAudioSource.cpp: This class represents the instrument. It manages a pool of SynthesiserSound and SynthesiserVoice objects to produce polyphonic sound from MIDI input.

Metronome.h: A straightforward class that generates a click sound. It calculates when to trigger a click based on the current beat and tempo, providing a timing reference for the user.

StepEditor.h: This component handles the visual and logical representation of the step sequencer grid. It inherits from juce::Component and manages the drawing of the grid and notes, as well as handling mouse clicks for editing.

Data.h: A singleton or static class used to share global application state. It holds variables such as the current tempo (bpm) and other settings that need to be accessed by various parts of the application.

⚙️ Getting Started
To compile and run this project, you will need:

JUCE Framework: Download and install the JUCE framework from the official website.

C++ IDE: An IDE with C++ support, such as Visual Studio (Windows), Xcode (macOS), or a C++-enabled version of VS Code.

ProJucer: Use the JUCE ProJucer application to create a new project and add the provided source files.

📝 TODO
GUI Enhancements: The current GUI is basic. Future work should focus on a more comprehensive and user-friendly interface.

More Instruments: Extend the SynthAudioSource or create new audio sources to support different types of synthesizers or sample-based playback.

MIDI Recording & Playback: Implement functionality to record MIDI input from a connected device and play it back.

Audio Effects: Add support for audio effects (e.g., reverb, delay, EQ) that can be applied to the master output or individual tracks.

File I/O: Implement functionality to save and load project files, including tempo, note data, and other settings.
