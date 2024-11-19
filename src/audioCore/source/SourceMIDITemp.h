﻿#pragma once

#include <JuceHeader.h>

class SourceMIDITemp final {
public:
	SourceMIDITemp() = default;

	void setData(const juce::MidiFile& data);
	void addTrack(const juce::MidiMessageSequence& track);
	void update();

	juce::MidiFile* getSourceData();

	const juce::MidiFile makeMIDIFile() const;
	const juce::MidiMessageSequence makeMIDITrack(int index) const;

	struct Note {
		uint8_t channel;
		double startSec, endSec;
		uint8_t pitch;
		uint8_t vel;
		juce::String lyrics;

		int noteOnEvent;
	};
	struct IntParam {
		uint8_t channel;
		double timeSec;
		int value;

		int event;
	};
	struct AfterTouch {
		uint8_t channel;
		double timeSec;
		uint8_t notePitch;
		uint8_t value;

		int event;
	};
	struct Controller {
		uint8_t channel;
		double timeSec;
		uint8_t number;
		uint8_t value;

		int event;
	};
	struct Misc {
		uint8_t channel;
		double timeSec;

		juce::MidiMessage message;

		int event;
	};
	
	int getTrackNum() const;

	int getNoteNum(int track) const;
	int getPitchWheelNum(int track) const;
	int getAfterTouchNum(int track) const;
	int getChannelPressureNum(int track) const;
	const std::set<uint8_t> getControllerNumbers(int track) const;
	int getControllerNum(int track, uint8_t number) const;
	int getMiscNum(int track) const;

	const Note getNote(int track, int index) const;
	const IntParam getPitchWheel(int track, int index) const;
	const AfterTouch getAfterTouch(int track, int index) const;
	const IntParam getChannelPressure(int track, int index) const;
	const Controller getController(int track, uint8_t number, int index) const;
	const Misc getMisc(int track, int index) const;

private:
	juce::MidiFile sourceData;
	int trackNum = 0;
	short timeFormat = 480;

	juce::Array<juce::Array<Note>> noteList;

	juce::Array<juce::Array<IntParam>> pitchWheelList;
	juce::Array<juce::Array<AfterTouch>> afterTouchList;
	juce::Array<juce::Array<IntParam>> channelPressureList;

	juce::Array<std::unordered_map<uint8_t, juce::Array<Controller>>> controllerList;

	juce::Array<juce::Array<Misc>> miscList;
};
