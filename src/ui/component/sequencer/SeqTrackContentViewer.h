﻿#pragma once

#include <JuceHeader.h>

class SeqTrackContentViewer final : public juce::Component {
public:
	using ScrollFunc = std::function<void(double)>;
	using WheelFunc = std::function<void(float, bool)>;
	using WheelAltFunc = std::function<void(double, double, float, bool)>;
	using DragStartFunc = std::function<void(void)>;
	using DragProcessFunc = std::function<void(int, int, bool, bool)>;
	using DragEndFunc = std::function<void(void)>;
	using SeqTrackSelectFunc = std::function<void(int)>;
	SeqTrackContentViewer(const ScrollFunc& scrollFunc,
		const WheelFunc& wheelFunc,
		const WheelAltFunc& wheelAltFunc,
		const DragStartFunc& dragStartFunc,
		const DragProcessFunc& dragProcessFunc,
		const DragEndFunc& dragEndFunc,
		const SeqTrackSelectFunc& trackSelectFunc);

	void setCompressed(bool isCompressed);

	void update(int index);
	void updateBlock(int blockIndex);
	void updateHPos(double pos, double itemSize);
	void updateDataRef();
	void updateData();
	void updateDataImage();

	void resized() override;
	void paint(juce::Graphics& g) override;

	void mouseMove(const juce::MouseEvent& event) override;
	void mouseDrag(const juce::MouseEvent& event) override;
	void mouseDown(const juce::MouseEvent& event) override;
	void mouseUp(const juce::MouseEvent& event) override;
	void mouseExit(const juce::MouseEvent& event) override;
	void mouseDoubleClick(const juce::MouseEvent& event) override;
	void mouseWheelMove(const juce::MouseEvent& event,
		const juce::MouseWheelDetails& wheel) override;

private:
	const ScrollFunc scrollFunc;
	const WheelFunc wheelFunc;
	const WheelAltFunc wheelAltFunc;
	const DragStartFunc dragStartFunc;
	const DragProcessFunc dragProcessFunc;
	const DragEndFunc dragEndFunc;
	const SeqTrackSelectFunc trackSelectFunc;

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
	juce::MidiMessageSequence midiDataTemp;

	juce::Array<juce::MemoryBlock> audioPointTemp;
	int midiMinNote = 0, midiMaxNote = 0;

	std::unique_ptr<juce::Timer> blockImageUpdateTimer = nullptr;

	enum class DragType {
		None, Add, Move, Left, Right
	};
	bool viewMoving = false;
	DragType dragType = DragType::None;
	int pressedBlockIndex = -1;
	double mousePressedSecond = 0;
	double mouseCurrentSecond = -1;
	bool blockValid = true;
	bool copyMode = false;

	void updateBlockInternal(int blockIndex);
	void setAudioPointTempInternal(const juce::Array<juce::MemoryBlock>& temp);
	void updateMIDINoteTempInternal();

	enum class BlockControllerType {
		None, Left, Right, Inside
	};
	std::tuple<BlockControllerType, int> getBlockController(float posX) const;
	std::tuple<BlockControllerType, int> getBlockControllerWithoutEdge(float posX) const;

	double limitTimeSec(double timeSec) const;
	bool checkBlockValid(double startSec, double endSec, int excludeIndex = -1) const;

	void addBlock(double startSec, double endSec, double offset = 0);
	void setBlock(int blockIndex, double startSec, double endSec, double offset);
	void splitBlock(int blockIndex, double timeSec);
	void removeBlock(int blockIndex);

	void showMenu(double seconds, int blockIndex);
	juce::PopupMenu createMenu(double seconds, int blockIndex);

	std::tuple<double, double> getViewArea(double pos, double itemSize) const;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SeqTrackContentViewer)
};
