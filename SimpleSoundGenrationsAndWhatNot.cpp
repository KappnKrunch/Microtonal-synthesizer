// SimpleSoundGenrationsAndWhatNot.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "MicroSynth.pch"

int main()
{
	Synthesizer synth;

	InputManager inputs;

	inputs.InitializeMidiConnection();

	inputs.WatchMidiInputs(synth); //check if midi inputs are valid first

	inputs.WatchKeyboardInputs(synth); //watch the keyboard inputs

	inputs.WatchConsoleInputs(synth); //if not go to console as backup

}

