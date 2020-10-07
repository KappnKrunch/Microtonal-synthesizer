//
//  MicroSynth.cpp
//  microtonal synthesizer
//
//  Created by Andrea Forbush on 10/4/20.
//  Copyright © 2020 Andrea Forbush. All rights reserved.
//

#include "MicroSynth.pch"














double MusicTheory::FFF(const int &fundamentalIndex, const int &intervalIndex, const int &octaveIndex)
{
	//frequency from fundamental
	//0 = C  , 11 = B
	//0 = unison,  11 = Major 7th

	//double justIntervals[]{ 1.0,  17.0 / 16,  9.0 / 8,  6.0 / 5,  5.0 / 4,  4.0 / 3,  11.0 / 8,  3.0 / 2,  9.0 / 5,  5.0 / 3,  7.0 / 4,  15.0 / 8 };
	double justIntervals[]{ 1.0,  25.0 / 24 ,  9.0 / 8,  6.0 / 5,  5.0 / 4,  4.0 / 3,  45.0 / 32,  3.0 / 2,  8.0 / 5,  5.0 / 3,  9.0 / 5,  15.0 / 8 };

	double octaveMultiplier = pow(2.0, octaveIndex + 1);
	double pitch = fundamentalPitches[fundamentalIndex];
	double intervalFromPitch = justIntervals[intervalIndex];

	return pitch * octaveMultiplier * intervalFromPitch;
}


double MusicTheory::BaseFrequency(const int& intervalIndex, const int& octaveIndex)
{
	return fundamentalPitches[intervalIndex] * pow(2.0, octaveIndex + 1);
}

double MusicTheory::fundamentalPitches[] = { 16.35, 17.32, 18.35, 19.45, 20.60, 21.83, 23.12, 24.50, 25.96, 27.50, 29.14, 30.87 };

string MusicTheory::noteNames[] = { "C", "C#/Db", "D", "D#/Eb", "E", "F", "F#/Gb", "G", "G#/Ab", "A", "A#/Bb", "B" };






















void KeyBoard::SetFundamentalPitch(const int &fundamentalIndex)
{
	//sets the fundamental all keys in the keyboard to be a basis of fundamentalIndex
	for (int k = 0; k < 96; ++k)
	{
		int pitchClass = (k + 12 - fundamentalIndex) % 12;
		int o = (k - pitchClass) / 12;
		if (k < pitchClass) o = -1;

		keys[k].frequency = MusicTheory::FFF(fundamentalIndex, pitchClass, o);
	}
}

void KeyBoard::SetFundamentalPitch()
{
	//exactly like SetFundamentalPitch(const int &fundamentalIndex) but set to
	//equal temperament 

	for (int k = 0; k < 96; ++k)
	{
		int pitchClass = k % 12;
		int o = (k - pitchClass) / 12;
		if (k < pitchClass) o = -1;

		keys[k].frequency = MusicTheory::BaseFrequency(pitchClass, o);
	}
}


void KeyBoard::SetKeyOn(const int& keyIndex, const double& velocity)
{
	keys[keyIndex].keyDown = true;
	keys[keyIndex].active = true;
	keys[keyIndex].initialAmplitude = velocity;
	keys[keyIndex].amplitudeDelta = attackSpeed;
	keys[keyIndex].amplitude = d_cutoffAmplitude;
}


void KeyBoard::SetKeyOff(const int& keyIndex)
{
	keys[keyIndex].keyDown = false;

	if (!footPedal) keys[keyIndex].amplitudeDelta = decaySpeed;
}


//this runs every frame
void KeyBoard::KeySustain(const int& keyIndex)
{
	//run on ever key
	if (keys[keyIndex].amplitude > keys[keyIndex].initialAmplitude) keys[keyIndex].amplitudeDelta = sustainSpeed;

	keys[keyIndex].amplitude *= keys[keyIndex].amplitudeDelta;

	if (keys[keyIndex].amplitude <= d_cutoffAmplitude) keys[keyIndex].active = false;

	if (!keys[keyIndex].keyDown && !footPedal && keys[keyIndex].active) SetKeyOff(keyIndex);
}





















Synthesizer::Synthesizer()
{
	SetStreamPref(1024, 1, 44100);
	ReadConfigFile(true);

	Timbre defaultTimbre{ {1.0,'S'},{2.0,'S'},{3.0,'s'} };

	SetInstrumentTimbre(defaultTimbre);

	play();
}


void Synthesizer::SetStreamPref(const int& bufferSize, const int& channelCount, const int& sampleRate)
{
	m_currentSample = 0;
	samplesToStream = bufferSize;
	m_samples.reserve(samplesToStream);
	m_samples.resize(0.0, samplesToStream);
	m_sampleRate = sampleRate;

	keyboard.SetFundamentalPitch(0); //set to c in beginning so every key has a pitch

	initialize(channelCount, sampleRate);
}


void Synthesizer::SetInstrumentTimbre(Timbre &newTimbre)
{
	instrumentTimbre = newTimbre;
}


void Synthesizer::ReadConfigFile(const bool& shouldPrint)
{
	ifstream configFile;

	//decaysSpeed,
	//attackSpeed,
	//sustainSpeed,
	//{ {freq, waveform},{2.0,'S'},{ {1.0,'s'},{3.5,'T'} },

	configFile.open("synthConfig.txt");

	if (!configFile.eof() && configFile.is_open())
	{
		string inputString;

		getline(configFile, inputString);
		if (keyboard.decaySpeed != stod(inputString) || shouldPrint) cout << stod(inputString) << " decay\n";
		keyboard.decaySpeed = stod(inputString);

		getline(configFile, inputString);
		if (keyboard.attackSpeed != stod(inputString) || shouldPrint) cout << stod(inputString) << " attack\n";
		keyboard.attackSpeed = stod(inputString);

		getline(configFile, inputString);
		if (keyboard.sustainSpeed != stod(inputString) || shouldPrint) cout << stod(inputString) << " sustain\n";
		keyboard.sustainSpeed = stod(inputString);

		getline(configFile, inputString);

		Timbre timbre;

		int overtonePos = 1;//something not 0

		while (overtonePos != 0)
		{
			overtonePos = inputString.find_last_of("{");

			string rawOvertoneString = inputString.substr(overtonePos, inputString.length());

			rawOvertoneString = rawOvertoneString.substr(0, rawOvertoneString.find_last_of("}"));
			rawOvertoneString = rawOvertoneString.substr(rawOvertoneString.find_last_of("{") + 1, rawOvertoneString.length());

			Overtone newOvertone{
				stod(rawOvertoneString.substr(0,rawOvertoneString.find_last_of(","))),
				rawOvertoneString.substr(rawOvertoneString.find_last_of(",") + 1,rawOvertoneString.length())[0]
			};

			timbre.push_back(newOvertone);

			inputString = inputString.substr(0, overtonePos - 1);
		}
		reverse(timbre.begin(), timbre.end());

		bool areEqual = true;

		if (timbre.size() == instrumentTimbre.size())
		{
			for (int i(0); i < timbre.size(); i++) areEqual = areEqual && timbre[i].freq == instrumentTimbre[i].freq && timbre[i].form == instrumentTimbre[i].form;
		}

		if (!areEqual || shouldPrint)
		{
			for (int i(0); i < timbre.size(); i++)
				cout << "{" << timbre[i].freq << "," << timbre[i].form << "}" << ((i < timbre.size() - 1) ? "," : "");

			cout << endl;
		}

		SetInstrumentTimbre(timbre);
	}
	else if (shouldPrint)
	{
		cout << "no valid file inputs!" << endl;
		cout << keyboard.decaySpeed << " decay\n";
		cout << keyboard.attackSpeed << " attack\n";
		cout << keyboard.sustainSpeed << " sustain\n";

		for (int i(0); i < instrumentTimbre.size(); i++)
			cout << "{" << instrumentTimbre[i].freq << "," << instrumentTimbre[i].form << "}" << ((i < instrumentTimbre.size() - 1) ? "," : "");

		cout << endl;
	}

	configFile.close();
}


sf::Int16 Synthesizer::GenerateNoteSin(const double& time, const double& freq, const double& amp, const int& sampleRate) const
{
	double frameValueAtTime = 0; //change this to a double asap

	double tpc = sampleRate / freq;
	double cycles = time / tpc;
	double rad = twopi * cycles;

	double amplitude = 32767 * min(amp, 1.0);

	for (int h = 0; h < instrumentTimbre.size(); ++h)
	{
		amplitude *= 0.125; //dividing by two everytime and summing it, approaches 1

		//this block of code divides the note synthesis into each waveform, from the timbre datatype
		if (instrumentTimbre[h].form == 's') frameValueAtTime += amplitude * (0.5 + 0.5*sin(rad * instrumentTimbre[h].freq));
		else if (instrumentTimbre[h].form == 't') frameValueAtTime += amplitude * (0.5 + 0.5*asin(cos(rad * instrumentTimbre[h].freq)) / 1.5708);
		else if (instrumentTimbre[h].form == 'S') frameValueAtTime += amplitude * round(0.5 + 0.5*sin(rad * instrumentTimbre[h].freq));
	}

	return frameValueAtTime;
}



void Synthesizer::SampleActiveKeys(KeyBoard &keyboard, const int& sampleChunkSize)
{
	//this gets called whenever it needs to sample a new chunk

	double totalAmplitude = 0;
	double frame = 0;

	//anything in this loop happens faster than at least 44100 ticks per second
	for (int f = 0; f < sampleChunkSize; ++f)
	{
		totalAmplitude = 0;
		frame = 0;

		for (int k = 0; k < 96; ++k)
		{
			if (keyboard.keys[k].active)
			{
				frame += GenerateNoteSin(m_sampleTime, keyboard.keys[k].frequency, keyboard.keys[k].amplitude, m_sampleRate);
				keyboard.KeySustain(k);
				totalAmplitude += keyboard.keys[k].amplitude;
			}
		}
		if (totalAmplitude > 1.0)frame /= totalAmplitude;

		m_sampleTime = (m_sampleTime + 1) % SIZE_MAX;

		m_samples[f] = frame;
	}
}


bool Synthesizer::onGetData(Chunk& data)
{
	data.samples = &m_samples[m_currentSample];
	data.sampleCount = samplesToStream;

	m_currentSample = 0;

	SampleActiveKeys(keyboard, samplesToStream);

	return true;
}


void Synthesizer::onSeek(sf::Time timeOffset)
{
	// compute the corresponding sample index according to the sample rate and channel count
	m_currentSample = static_cast<std::size_t>(timeOffset.asSeconds() * getSampleRate() * getChannelCount());
}






















ChordIndices InputManager::DeconstructStringInput(const string &rawInput)
{
	ChordIndices deconstructedChord;
	string probablyANumber;

	for (int c = 0; c < rawInput.size(); ++c)
	{
		if (rawInput[c] == ' ')
		{
			deconstructedChord.push_back(stoi(probablyANumber));
			probablyANumber = "";
		}
		else if (c == rawInput.size() - 1)
		{
			probablyANumber += rawInput[c];
			deconstructedChord.push_back(stoi(probablyANumber));
			probablyANumber = "";
		}
		else
		{
			probablyANumber += rawInput[c];
		}
	}

	return deconstructedChord;
}


void InputManager::InitializeMidiConnection()
{
	midiEnabled = false;
	midiIn = new RtMidiIn();

	unsigned int nPorts = midiIn->getPortCount();
	if (nPorts > 0)
	{
		for (int i = 0; i < nPorts; i++) cout << "midi source " << i << ":" << midiIn->getPortName(i) << ", " << endl;

		cout << endl << "input source: ";

		int source;
		cin >> source;

		cout << "Open on midi port 1\n" << endl;
		midiIn->openPort(source);
		midiEnabled = true;
		midiIn->getMessage(&message);
		nBytes = message.size();
	}
	else
	{
		cout << "No midi source found" << endl << endl;
	}
}


void InputManager::WatchMidiInputs(Synthesizer& synth)
{
	//midi messages are constructed per each note, one at a time
	//every message is for one note and contains 3 bytes
	//first in on/off;  144 = on, 128 = off
	//second is note index
	//thrid is velocity ; release velocity seams to remain 64

	//footpedal has an [1] index of 64 as well, however the [2] on/off value is either 127 or 0, no idea why

	vector<int> queue;

	while (midiEnabled)
	{
		midiIn->getMessage(&message);
		nBytes = message.size();

		for (int b = 0; b < nBytes; b += 3)
		{
			//cout << "click " << static_cast<int>(message[0+b]) << endl;

			if ((int(message[0 + b]) == 144))
			{
				//regular keyboard on switches
				if (int(message[1 + b]) > 35 && int(message[1 + b]) < 108)
				{
					synth.keyboard.SetKeyOn(int(message[1 + b]) - 12, ((double)message[2 + b]) / 130);
				}
				//pitch change keys, on swithes
				if (int(message[1 + b]) < 35 && int(message[1 + b]) > 20)
				{
					//cout << static_cast<int>(message[1 + b]) % 12 << endl;
					synth.keyboard.SetFundamentalPitch(int(message[1 + b]) % 12);
				}
			}
			else if ((int(message[0 + b]) == 128))
			{
				if (int(message[1 + b]) > 35 && int(message[1 + b]) < 108)
				{
					synth.keyboard.SetKeyOff(int(message[1 + b]) - 12);
				}
			}

			//on switch for foot pedal
			if (int(message[1 + b]) == 64 && int(message[2 + b]) == 127)
			{
				synth.keyboard.footPedal = true;
			}

			//off switch for footpedal
			if (int(message[1 + b]) == 64 && int(message[2 + b]) == 0)
			{
				synth.keyboard.footPedal = false;
			}
		}
	}
}

void InputManager::WatchConsoleInputs(Synthesizer& synth)
{
	bool changingRoot = false;
	bool changingNotes = false;
	consoleInputEnabled = true;

	//program part of the program
	while (consoleInputEnabled)
	{
		cout << "Enter 'f' or 'c' to change either the fundamental or chord" << endl;
		string line;

		cin >> line;

		if (line == "f") changingRoot = true;
		if (line == "c") changingNotes = true;

		while (changingRoot)
		{
			cout << "Enter fundamental by index, C = 0, B = 11" << endl << "'b' and enter goes back" << endl;
			cin >> line;

			if (line == "b") changingRoot = false;
			else
			{
				synth.keyboard.SetFundamentalPitch(stoi(line));
			}

			line = "";
		}

		while (changingNotes)
		{
			cout << "Enter notes by index, C = 0, B = 11, C = 60, B = 71" << endl << "'b' and enter goes back" << endl;
			cin >> line;

			if (line == "b") changingNotes = false;
			else
			{
				ChordIndices chord = DeconstructStringInput(line);
				for (int n = 0; n < chord.size(); ++n)
				{
					if (chord[n] > 0) synth.keyboard.SetKeyOn(chord[n] % 96, 1);
					else synth.keyboard.SetKeyOff(abs(chord[n]) % 96);
				}
			}

			line = "";
		}
	}
}


void InputManager::WatchKeyboardInputs(Synthesizer& synth)
{
	vector<sf::Keyboard::Key> keyMap{
		sf::Keyboard::A,sf::Keyboard::W, sf::Keyboard::S, sf::Keyboard::E, sf::Keyboard::D, sf::Keyboard::F,
		sf::Keyboard::T ,sf::Keyboard::G ,sf::Keyboard::Y ,sf::Keyboard::H, sf::Keyboard::U, sf::Keyboard::J,
		sf::Keyboard::K, sf::Keyboard::O, sf::Keyboard::L }; //three more keys

	vector<bool> lastKeyStates(15, false);

	string removal;

	bool playingThroughKeyboard = true;

	bool changingRoot = false;

	int lastOctaveKeyState = 0;

	int octave = 4;

	cout << "wasd and whatnot to play notes. 'z' changes the fundamental." << endl;
	cout << "'space' controls the sustain pedal, 'c' sets to 12 tone, period '.' exits" << endl << endl;

	while (playingThroughKeyboard)
	{
		synth.ReadConfigFile(false);

		for (int k(0); k < 15; k++)
		{
			if (sf::Keyboard::isKeyPressed(keyMap[k]) && !lastKeyStates[k])
			{
				if (!changingRoot)
				{
					synth.keyboard.SetKeyOn((octave * 12) + k, 1);
				}
				else
				{
					cout << "fundamental now " << MusicTheory::noteNames[k] << endl;

					synth.keyboard.SetFundamentalPitch(k % 12);

					changingRoot = false;
				}

				lastKeyStates[k] = true;
			}
			else if (!sf::Keyboard::isKeyPressed(keyMap[k]) && !changingRoot && lastKeyStates[k])
			{
				//for every octave
				for (int i(1); i < 7; i++)
				{
					synth.keyboard.SetKeyOff((i * 12) + k);
				}

				lastKeyStates[k] = false;
			}

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Z)) changingRoot = true;

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::Period))
			{
				playingThroughKeyboard = false;

				cout << endl;

				break;
			}

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::C) &&
				!(synth.keyboard.keys[0].frequency == MusicTheory::fundamentalPitches[0] * 2.0 &&
					synth.keyboard.keys[1].frequency == MusicTheory::fundamentalPitches[1] * 2.0))
			{
				cout << "fundamental now equal" << endl;
				synth.keyboard.SetFundamentalPitch();
			}

			if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) && lastOctaveKeyState == 0)
			{
				octave = max(octave - 1, 1);

				lastOctaveKeyState = -1;
			}
			else if (sf::Keyboard::isKeyPressed(sf::Keyboard::RShift) && lastOctaveKeyState == 0)
			{
				octave = min(octave + 1, 6);

				lastOctaveKeyState = 1;
			}
			else if (!(sf::Keyboard::isKeyPressed(sf::Keyboard::LShift) || sf::Keyboard::isKeyPressed(sf::Keyboard::RShift))) lastOctaveKeyState = 0;

			synth.keyboard.footPedal = sf::Keyboard::isKeyPressed(sf::Keyboard::Space);
		}
	}

	cout << endl << "press enter to continue: " << endl;

	cin.ignore(100000, '.');

}


