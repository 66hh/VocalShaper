﻿#pragma once

#include <JuceHeader.h>

class SeqTrackContentViewer final : public juce::Component {
public:
	SeqTrackContentViewer();

	void setCompressed(bool isCompressed);

	void update(int index);
	void updateBlock(int blockIndex);
	void updateHPos(double pos, double itemSize);
	void updateDataRef();
	void updateData();
	void updateDataImage();
	
	void updateAudioImage();
	void updateMIDIImage();

	void paint(juce::Graphics& g) override;

private:
	bool compressed = false;
	int index = -1;
	double pos = 0, itemSize = 0;
	double secStart = 0, secEnd = 0;
	juce::Colour trackColor, nameColor;
	bool audioValid = false, midiValid = false;
	juce::String audioName, midiName;
	juce::String blockNameCombined;

	struct BlockItem final {
		double startTime = 0, endTime = 0, offset = 0;
	};
	juce::OwnedArray<BlockItem> blockTemp;

	std::tuple<double, juce::AudioSampleBuffer> audioDataTemp;
	juce::MidiFile midiDataTemp;

	juce::Array<juce::MemoryBlock> audioPointTemp;

	using IntSection = std::pair<int, int>;
	using ImageIndex = std::pair<IntSection, int>;
	using ImageIndexList = juce::Array<ImageIndex>;
	std::unique_ptr<juce::Image> audioImageTemp = nullptr;
	ImageIndexList audioImageIndex;
	std::unique_ptr<juce::Image> midiImageTemp = nullptr;

	std::unique_ptr<juce::Timer> blockImageUpdateTimer = nullptr;

	void updateBlockInternal(int blockIndex);
	void setAudioPointTempInternal(const juce::Array<juce::MemoryBlock>& temp);

	void setAudioImageTemp(const juce::Image& image, const ImageIndexList& index);
	void setMIDIImageTemp(const juce::Image& image);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SeqTrackContentViewer)
};
