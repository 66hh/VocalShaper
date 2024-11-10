﻿#include "SeqTrackContentViewer.h"
#include "../../lookAndFeel/LookAndFeelFactory.h"
#include "../../misc/AudioExtractor.h"
#include "../../misc/MainThreadPool.h"
#include "../../misc/Tools.h"
#include "../../misc/CoreActions.h"
#include "../../Utils.h"
#include "../../../audioCore/AC_API.h"

class DataImageUpdateTimer final : public juce::Timer {
public:
	DataImageUpdateTimer() = delete;
	DataImageUpdateTimer(
		juce::Component::SafePointer<SeqTrackContentViewer> parent);

	void timerCallback() override;

private:
	const juce::Component::SafePointer<SeqTrackContentViewer> parent;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DataImageUpdateTimer)
};

DataImageUpdateTimer::DataImageUpdateTimer(
	juce::Component::SafePointer<SeqTrackContentViewer> parent)
	: parent(parent) {}

void DataImageUpdateTimer::timerCallback() {
	if (this->parent) {
		this->parent->updateDataImage();
	}
}

SeqTrackContentViewer::SeqTrackContentViewer(
	const ScrollFunc& scrollFunc,
	const WheelFunc& wheelFunc,
	const WheelAltFunc& wheelAltFunc,
	const DragStartFunc& dragStartFunc,
	const DragProcessFunc& dragProcessFunc,
	const DragEndFunc& dragEndFunc,
	const SeqTrackSelectFunc& trackSelectFunc)
	: scrollFunc(scrollFunc), wheelFunc(wheelFunc), wheelAltFunc(wheelAltFunc),
	dragStartFunc(dragStartFunc), dragProcessFunc(dragProcessFunc),
	dragEndFunc(dragEndFunc), trackSelectFunc(trackSelectFunc) {
	/** Look And Feel */
	this->setLookAndFeel(
		LookAndFeelFactory::getInstance()->getLAFFor(LookAndFeelFactory::SeqBlock));

	/** Data Update Timer */
	this->blockImageUpdateTimer = std::make_unique<DataImageUpdateTimer>(this);
	/**
	 * To fix the bug in AudioExtractor.
	 * This will invoke AudioExtractor::extractAsync(...) every period of time.
	 * I currently do not have a better solution.
	 */
	this->blockImageUpdateTimer->startTimer(5000);
}

void SeqTrackContentViewer::setCompressed(bool isCompressed) {
	this->compressed = isCompressed;
	this->repaint();
}

void SeqTrackContentViewer::update(int index) {
	bool changeData = true;//this->index != index;

	this->index = index;
	if (index > -1) {
		this->updateBlock(-1);
		
		this->trackColor = quickAPI::getSeqTrackColor(index);

		auto& laf = this->getLookAndFeel();
		auto textColorLight = laf.findColour(
			juce::Label::ColourIds::textWhenEditingColourId);
		auto textColorDark = laf.findColour(
			juce::Label::ColourIds::textColourId);
		this->nameColor = utils::chooseTextColor(this->trackColor, textColorLight, textColorDark);

		if (changeData) {
			this->updateDataRef();
			this->updateData();
		}

		this->repaint();
	}
}

void SeqTrackContentViewer::updateBlock(int /*blockIndex*/) {
	/** Create Or Remove Block */
	int currentSize = this->blockTemp.size();
	int newSize = quickAPI::getBlockNum(this->index);
	if (currentSize > newSize) {
		for (int i = currentSize - 1; i >= newSize; i--) {
			this->blockTemp.remove(i);
		}
	}
	else {
		for (int i = currentSize; i < newSize; i++) {
			auto block = std::make_unique<BlockItem>();
			this->blockTemp.add(std::move(block));
		}
	}

	/** Update Blocks */
	for (int i = 0; i < this->blockTemp.size(); i++) {
		this->updateBlockInternal(i);
	}

	/** Repaint */
	this->repaint();
}

void SeqTrackContentViewer::updateHPos(double pos, double itemSize) {
	if (this->getWidth() <= 0) { return; }
	
	bool shouldRepaintDataImage = !juce::approximatelyEqual(this->itemSize, itemSize);

	this->pos = pos;
	this->itemSize = itemSize;

	std::tie(this->secStart, this->secEnd) = this->getViewArea(pos, itemSize);

	if (shouldRepaintDataImage) {
		this->updateDataImage();
	}

	this->repaint();
}

void SeqTrackContentViewer::updateDataRef() {
	this->audioValid = quickAPI::isSeqTrackHasAudioData(this->index);
	this->midiValid = quickAPI::isSeqTrackHasMIDIData(this->index);

	this->audioName = this->audioValid ? quickAPI::getSeqTrackDataRefAudio(this->index) : juce::String{};
	this->midiName = this->midiValid ? quickAPI::getSeqTrackDataRefMIDI(this->index) : juce::String{};

	this->blockNameCombined = this->audioName
		+ ((this->audioValid&& this->midiValid) ? " / " : "")
		+ this->midiName;

	this->updateData();

	this->repaint();
}

void SeqTrackContentViewer::updateData() {
	/** Clear Temp */
	this->audioDataTemp = {};
	this->midiDataTemp.clear();
	//this->audioPointTemp.clear();
	//this->midiMinNote = this->midiMaxNote = 0;

	/** Get Audio Data */
	if (this->audioValid) {
		this->audioDataTemp = quickAPI::getSeqTrackAudioData(this->index);
	}

	/** Get MIDI Data */
	if (this->midiValid) {
		int currentMIDITrack = quickAPI::getSeqTrackCurrentMIDITrack(this->index);
		auto midiDataRef = quickAPI::getSeqTrackMIDIRef(this->index);
		auto midiNoteList = quickAPI::getMIDISourceNotes(midiDataRef, currentMIDITrack);

		/** Add Each Note */
		this->midiDataTemp.ensureStorageAllocated(midiNoteList.size());
		for (auto& note : midiNoteList) {
			this->midiDataTemp.add({ note.startSec, note.endSec, note.pitch });
		}
	}

	/** Update Image Temp */
	this->updateDataImage();

	/** Repaint */
	this->repaint();
}

void SeqTrackContentViewer::updateDataImage() {
	/** Audio Data */
	if (this->audioValid) {
		/** Get Data */
		auto& [sampleRate, data] = this->audioDataTemp;

		/** Prepare Callback */
		auto callback = [comp = SafePointer{ this }](const AudioExtractor::Result& result) {
			if (comp) {
				/** Set Temp */
				comp->setAudioPointTempInternal(result);

				/** Repaint */
				comp->repaint();
			}
			};

		/** Get Point Num */
		double lengthSec = data.getNumSamples() / sampleRate;
		int pointNum = std::floor(lengthSec * this->itemSize);

		/** Extract */
		AudioExtractor::getInstance() ->extractAsync(
			this, data, pointNum, callback);
	}
	
	/** MIDI Data */
	if (this->midiValid) {
		/** Update MIDI */
		this->updateMIDINoteTempInternal();

		/** Repaint */
		this->repaint();
	}
}

void SeqTrackContentViewer::resized() {
	/** Update Time */
	std::tie(this->secStart, this->secEnd) = this->getViewArea(this->pos, this->itemSize);
}

void SeqTrackContentViewer::paint(juce::Graphics& g) {
	if (this->getWidth() <= 0 || this->getHeight() <= 0) { return; }

	/** Size */
	auto screenSize = utils::getScreenSize(this);
	float paddingHeight = screenSize.getHeight() * 0.0025;
	float blockRadius = screenSize.getHeight() * 0.005;
	float outlineThickness = screenSize.getHeight() * 0.0015;

	float blockPaddingHeight = screenSize.getHeight() * 0.01;
	float blockPaddingWidth = screenSize.getWidth() * 0.01;

	float blockNameFontHeight = screenSize.getHeight() * 0.015;

	float noteMaxHeight = screenSize.getHeight() * 0.015;

	float cursorLineThickness = screenSize.getWidth() * 0.001;
	float cursorLineDashLength = screenSize.getHeight() * 0.005;
	float cursorLineSkipLength = cursorLineDashLength;
	float cursorLineArray[2] = { cursorLineDashLength, cursorLineSkipLength };

	/** Color */
	auto& laf = this->getLookAndFeel();
	juce::Colour outlineColor = laf.findColour(
		juce::Label::ColourIds::outlineColourId);

	/** Font */
	juce::Font blockNameFont(juce::FontOptions{ blockNameFontHeight });

	/** Data Scale Ratio */
	auto& [sampleRate, data] = this->audioDataTemp;
	double dstLengthSec = data.getNumSamples() / sampleRate;
	int dstPointNum = std::floor(dstLengthSec * this->itemSize);
	double dstPointPerSec = this->itemSize;

	double imgScaleRatio = 1;
	if (this->audioPointTemp.size() > 0) {
		auto& data = this->audioPointTemp.getReference(0);
		imgScaleRatio = dstPointNum / (double)(data.getSize() / 2 / sizeof(float));
	}

	/** Block Paint Func */
	auto paintBlockFunc = [this, &g, &blockNameFont, paddingHeight, blockRadius, outlineColor, outlineThickness,
		blockPaddingWidth, blockPaddingHeight, blockNameFontHeight, dstPointPerSec, imgScaleRatio, noteMaxHeight]
	(double blockStartSec, double blockEndSec, double blockOffset, float blockAlpha = 1.f) {
		if (this->secStart >= this->secEnd) { return; }
		float startPos = (blockStartSec - this->secStart) / (this->secEnd - this->secStart) * this->getWidth();
		float endPos = (blockEndSec - this->secStart) / (this->secEnd - this->secStart) * this->getWidth();

		/** Rect */
		juce::Rectangle<float> blockRect(
			startPos, paddingHeight,
			endPos - startPos, this->getHeight() - paddingHeight * 2);
		g.setColour(this->trackColor.withAlpha(0.5f).withMultipliedAlpha(blockAlpha));
		g.fillRoundedRectangle(blockRect, blockRadius);
		g.setColour(outlineColor.withMultipliedAlpha(blockAlpha));
		g.drawRoundedRectangle(blockRect, blockRadius, outlineThickness);

		/** Name */
		float nameStartPos = std::max(startPos, 0.f) + blockPaddingWidth;
		float nameEndPos = std::min(endPos, (float)this->getWidth()) - blockPaddingWidth;
		juce::Rectangle<float> nameRect(
			nameStartPos, this->compressed ? 0 : blockPaddingHeight,
			nameEndPos - nameStartPos, this->compressed ? this->getHeight() : blockNameFontHeight);
		g.setColour(this->nameColor.withMultipliedAlpha(blockAlpha));
		g.setFont(blockNameFont);
		g.drawFittedText(this->blockNameCombined, nameRect.toNearestInt(),
			juce::Justification::centredLeft, 1, 1.f);

		/** Wave */
		if (!this->compressed) {
			/** Select Time */
			double startSec = std::max(blockStartSec, this->secStart) + blockOffset;
			double endSec = std::min(blockEndSec, this->secEnd) + blockOffset;
			int startPixel = startSec * dstPointPerSec / imgScaleRatio;
			int endPixel = endSec * dstPointPerSec / imgScaleRatio;

			/** Paint Each Channel */
			g.setColour(this->nameColor.withMultipliedAlpha(blockAlpha));
			float wavePosY = blockPaddingHeight + blockNameFontHeight + blockPaddingHeight;
			float channelHeight = (this->getHeight() - blockPaddingHeight - wavePosY) / (float)this->audioPointTemp.size();
			for (int i = 0; i < this->audioPointTemp.size(); i++) {
				float channelPosY = wavePosY + channelHeight * i;
				auto& data = this->audioPointTemp.getReference(i);
				int dataSize = data.getSize() / 2 / sizeof(float);

				/** Paint Each Point */
				for (int j = std::max(startPixel, 0); j <= endPixel && j < dataSize; j++) {
					/** Get Value */
					float minVal = 0, maxVal = 0;
					data.copyTo(&minVal, (j * 2 + 0) * (int)sizeof(float), sizeof(float));
					data.copyTo(&maxVal, (j * 2 + 1) * (int)sizeof(float), sizeof(float));

					/** Paint Point */
					double pixelSec = j * imgScaleRatio / dstPointPerSec;
					float pixelPosX = (pixelSec - blockOffset - this->secStart) / (this->secEnd - this->secStart) * this->getWidth();
					juce::Rectangle<float> pointRect(
						pixelPosX,
						channelPosY + channelHeight / 2.f - (maxVal / 1.f) * channelHeight / 2.f,
						imgScaleRatio,
						channelHeight * ((maxVal - minVal) / 2.f));
					g.fillRect(pointRect);
				}
			}
		}

		/** Note */
		if (!this->compressed) {
			/** Select Time */
			double startSec = std::max(blockStartSec, this->secStart) + blockOffset;
			double endSec = std::min(blockEndSec, this->secEnd) + blockOffset;

			/** Content Area */
			float notePosY = blockPaddingHeight + blockNameFontHeight + blockPaddingHeight;
			float noteAreaHeight = this->getHeight() - blockPaddingHeight - notePosY;
			juce::Rectangle<float> noteContentRect(0, notePosY, this->getWidth(), noteAreaHeight);

			/** Limit Note Height */
			double minNoteID = this->midiMinNote, maxNoteID = this->midiMaxNote;
			float noteHeight = noteAreaHeight / (maxNoteID - minNoteID + 1);
			if (noteHeight > noteMaxHeight) {
				noteHeight = noteMaxHeight;

				double centerNoteID = minNoteID + (maxNoteID - minNoteID) / 2;
				minNoteID = centerNoteID - ((noteAreaHeight / noteHeight + 1) / 2 - 1);
				maxNoteID = centerNoteID + ((noteAreaHeight / noteHeight + 1) / 2 - 1);
			}

			/** Paint Each Note */
			g.setColour(this->nameColor.withMultipliedAlpha(blockAlpha));

			std::array<double, 128> noteStartTime{};
			std::fill(noteStartTime.begin(), noteStartTime.end(), -1.0);

			for (auto& [noteStartSec, noteEndSec, noteNum] : this->midiDataTemp) {
				double noteStart = std::max(noteStartSec, startSec);
				double noteEnd = std::min(noteEndSec, endSec);
				juce::Rectangle<float> noteRect(
					(noteStart - blockOffset - this->secStart) / (this->secEnd - this->secStart) * this->getWidth(),
					notePosY + (maxNoteID - noteNum) * noteHeight,
					(noteEnd - noteStart) / (this->secEnd - this->secStart) * this->getWidth(),
					noteHeight);
				g.fillRect(noteRect);
			}
		}
		};

	/** Blocks */
	for (int i = 0; i < this->blockTemp.size(); i++) {
		auto block = this->blockTemp.getUnchecked(i);
		if ((block->startTime <= this->secEnd)
			&& (block->endTime >= this->secStart)) {
			/** Get Block Time */
			double blockStartTime = block->startTime;
			double blockEndTime = block->endTime;
			double blockOffset = block->offset;
			if (i == this->pressedBlockIndex) {
				if (!(this->dragType == DragType::Move && this->copyMode)) {
					switch (this->dragType) {
					case DragType::Move: {
						double delta = this->mouseCurrentSecond - this->mousePressedSecond;
						blockStartTime += delta;
						blockEndTime += delta;
						blockOffset -= delta;
						break;
					}
					case DragType::Left: {
						blockStartTime = this->mouseCurrentSecond;
						blockStartTime = std::min(blockStartTime, blockEndTime);
						break;
					}
					case DragType::Right: {
						blockEndTime = this->mouseCurrentSecond;
						blockEndTime = std::max(blockEndTime, blockStartTime);
						break;
					}
					default:
						break;
					}
				}
			}

			/** Get Alpha */
			float alpha = 1.f;
			if (this->dragType != DragType::None && i == this->pressedBlockIndex) {
				if (!(this->dragType == DragType::Move && this->copyMode)) {
					alpha = 0.75f;

					/** Valid */
					if (!this->blockValid) {
						alpha = 0.5f;
					}
				}
			}

			/** Paint Block */
			paintBlockFunc(blockStartTime, blockEndTime, blockOffset, alpha);
		}
	}

	/** Add Block */
	if (this->dragType == DragType::Add) {
		/** Get Block Time */
		double blockStartSec = std::min(this->mousePressedSecond, this->mouseCurrentSecond);
		double blockEndSec = std::max(this->mousePressedSecond, this->mouseCurrentSecond);

		/** Get Alpha */
		float alpha = 0.75f;

		/** Valid */
		if (!this->blockValid) {
			alpha = 0.5f;
		}

		/** Paint Block */
		paintBlockFunc(blockStartSec, blockEndSec, 0, alpha);
	}

	/** Copy Mode */
	if (this->dragType == DragType::Move && this->copyMode) {
		if (this->pressedBlockIndex >= 0 && this->pressedBlockIndex < this->blockTemp.size()) {
			auto block = this->blockTemp.getUnchecked(this->pressedBlockIndex);

			/** Get Block Time */
			double delta = this->mouseCurrentSecond - this->mousePressedSecond;
			double blockStartTime = block->startTime + delta;
			double blockEndTime = block->endTime + delta;
			double blockOffset = block->offset - delta;
			
			/** Get Alpha */
			float alpha = 0.75f;

			/** Valid */
			if (!this->blockValid) {
				alpha = 0.5f;
			}

			/** Paint Block */
			paintBlockFunc(blockStartTime, blockEndTime, blockOffset, alpha);
		}
	}

	/** Cursor Line */
	if (this->mouseCurrentSecond >= this->secStart &&
		this->mouseCurrentSecond <= this->secEnd) {
		float cursorPosX = (this->mouseCurrentSecond - this->secStart) / (this->secEnd - this->secStart) * this->getWidth();
		juce::Line<float> sciLine(
			cursorPosX, paddingHeight,
			cursorPosX, this->getHeight() - paddingHeight);
		g.setColour(this->nameColor);
		g.drawDashedLine(sciLine, cursorLineArray,
			sizeof(cursorLineArray) / sizeof(float),
			cursorLineThickness, 0);
	}
}

void SeqTrackContentViewer::mouseMove(const juce::MouseEvent& event) {
	/** Current Method Also Invoked by mouseUp() */

	/** Block */
	float posX = event.position.getX();
	std::tuple<SeqTrackContentViewer::BlockControllerType, int> blockController;
	switch (Tools::getInstance()->getType()) {
	case Tools::Type::Arrow:
		blockController = this->getBlockControllerWithoutEdge(posX);
		break;
	case Tools::Type::Pencil:
		blockController = this->getBlockController(posX);
		break;
	}

	switch (std::get<0>(blockController)) {
	case BlockControllerType::Left:
		this->setMouseCursor(juce::MouseCursor::LeftEdgeResizeCursor);
		break;
	case BlockControllerType::Right:
		this->setMouseCursor(juce::MouseCursor::RightEdgeResizeCursor);
		break;
	case BlockControllerType::Inside:
		this->setMouseCursor(juce::MouseCursor::PointingHandCursor);
		break;
	default:
		this->setMouseCursor(juce::MouseCursor::NormalCursor);
		break;
	}

	/** Mouse Second */
	this->mouseCurrentSecond = this->secStart + (event.position.x / (double)this->getWidth()) * (this->secEnd - this->secStart);
	this->mouseCurrentSecond = this->limitTimeSec(this->mouseCurrentSecond);

	this->repaint();
}

void SeqTrackContentViewer::mouseDrag(const juce::MouseEvent& event) {
	/** Auto Scroll */
	float xPos = event.position.getX();
	if (!this->viewMoving) {
		double delta = 0;
		if (xPos > this->getWidth()) {
			delta = xPos - this->getWidth();
		}
		else if (xPos < 0) {
			delta = xPos;
		}

		if (delta != 0) {
			this->scrollFunc(delta / 4);
		}
	}

	/** Mouse Second */
	this->mouseCurrentSecond = this->secStart + (event.position.x / (double)this->getWidth()) * (this->secEnd - this->secStart);
	this->mouseCurrentSecond = this->limitTimeSec(this->mouseCurrentSecond);

	if (event.mods.isLeftButtonDown()) {
		/** Move View */
		if (this->viewMoving) {
			int distanceX = event.getDistanceFromDragStartX();
			int distanceY = event.getDistanceFromDragStartY();
			this->dragProcessFunc(distanceX, distanceY, true, true);
		}

		/** Edit Block */
		else if (this->dragType != DragType::None) {
			switch (this->dragType) {
			case DragType::Add: {
				double blockStartSec = std::min(this->mousePressedSecond, this->mouseCurrentSecond);
				double blockEndSec = std::max(this->mousePressedSecond, this->mouseCurrentSecond);

				this->blockValid = this->checkBlockValid(blockStartSec, blockEndSec, -1);
				break;
			}
			case DragType::Left:
			case DragType::Right:
			case DragType::Move: {
				if (this->pressedBlockIndex >= 0 && this->pressedBlockIndex < this->blockTemp.size()) {
					auto block = this->blockTemp.getUnchecked(this->pressedBlockIndex);

					double blockStartTime = block->startTime;
					double blockEndTime = block->endTime;
					double blockOffset = block->offset;

					switch (this->dragType) {
					case DragType::Move: {
						double delta = this->mouseCurrentSecond - this->mousePressedSecond;
						blockStartTime += delta;
						blockEndTime += delta;
						blockOffset -= delta;
						break;
					}
					case DragType::Left: {
						blockStartTime = this->mouseCurrentSecond;
						blockStartTime = std::min(blockStartTime, blockEndTime);
						break;
					}
					case DragType::Right: {
						blockEndTime = this->mouseCurrentSecond;
						blockEndTime = std::max(blockEndTime, blockStartTime);
						break;
					}
					default:
						break;
					}

					this->blockValid = this->checkBlockValid(blockStartTime, blockEndTime,
						this->copyMode ? -1 : this->pressedBlockIndex);
				}

				break;
			}
			default:
				break;
			}
		}
	}

	this->repaint();
}

void SeqTrackContentViewer::mouseDown(const juce::MouseEvent& event) {
	float posX = event.position.x;

	/** Get Tool Type */
	auto tool = Tools::getInstance()->getType();

	if (event.mods.isLeftButtonDown()) {
		if (event.mods.isAltDown()) {
			/** Move View Area */
			this->viewMoving = true;
			this->setMouseCursor(juce::MouseCursor::DraggingHandCursor);
			this->dragStartFunc();
		}
		else {
			/** Check Tool Type */
			switch (tool) {
			case Tools::Type::Pencil: {
				/** Edit */
				auto [controller, index] = this->getBlockController(posX);

				this->pressedBlockIndex = index;
				if (controller == BlockControllerType::Inside) {
					this->dragType = DragType::Move;
				}
				else if (controller == BlockControllerType::Left) {
					this->dragType = DragType::Left;
				}
				else if (controller == BlockControllerType::Right) {
					this->dragType = DragType::Right;
				}
				else {
					this->dragType = DragType::Add;
				}
				this->mousePressedSecond = this->secStart + (posX / (double)this->getWidth()) * (this->secEnd - this->secStart);
				this->mousePressedSecond = this->limitTimeSec(this->mousePressedSecond);
				this->mouseCurrentSecond = this->mousePressedSecond;
				this->copyMode = (controller == BlockControllerType::Inside) && event.mods.isCtrlDown();

				this->repaint();

				break;
			}
			case Tools::Type::Arrow: {
				/** TODO */
				break;
			}
			}
		}
	}

	/** Repaint */
	this->repaint();
}

void SeqTrackContentViewer::mouseUp(const juce::MouseEvent& event) {
	float posX = event.position.x;

	/** Get Tool Type */
	auto tool = Tools::getInstance()->getType();

	if (event.mods.isLeftButtonDown()) {
		/** Move View */
		if (this->viewMoving) {
			this->viewMoving = false;
			this->mouseMove(event);/**< Update Mouse Cursor */
			this->dragEndFunc();
		}

		/** Check Drag Type */
		switch (this->dragType) {
		case DragType::Add: {
			double currentSec = this->secStart + (posX / (double)this->getWidth()) * (this->secEnd - this->secStart);
			currentSec = this->limitTimeSec(currentSec);

			double blockStartSec = std::min(this->mousePressedSecond, currentSec);
			double blockEndSec = std::max(this->mousePressedSecond, currentSec);

			if (this->checkBlockValid(blockStartSec, blockEndSec, -1)) {
				this->addBlock(blockStartSec, blockEndSec);
			}
			break;
		}
		case DragType::Left:
		case DragType::Right: 
		case DragType::Move: {
			double currentSec = this->secStart + (posX / (double)this->getWidth()) * (this->secEnd - this->secStart);
			currentSec = this->limitTimeSec(currentSec);

			if (this->pressedBlockIndex >= 0 && this->pressedBlockIndex < this->blockTemp.size()) {
				auto block = this->blockTemp.getUnchecked(this->pressedBlockIndex);

				double blockStartTime = block->startTime;
				double blockEndTime = block->endTime;
				double blockOffset = block->offset;

				switch (this->dragType) {
				case DragType::Move: {
					double delta = currentSec - this->mousePressedSecond;
					blockStartTime += delta;
					blockEndTime += delta;
					blockOffset -= delta;
					break;
				}
				case DragType::Left: {
					blockStartTime = currentSec;
					blockStartTime = std::min(blockStartTime, blockEndTime);
					break;
				}
				case DragType::Right: {
					blockEndTime = currentSec;
					blockEndTime = std::max(blockEndTime, blockStartTime);
					break;
				}
				default:
					break;
				}

				if (this->copyMode) {
					if (this->checkBlockValid(blockStartTime, blockEndTime, -1)) {
						this->addBlock(blockStartTime, blockEndTime, blockOffset);
					}
				}
				else {
					if (this->checkBlockValid(blockStartTime, blockEndTime, this->pressedBlockIndex)) {
						this->setBlock(this->pressedBlockIndex,
							blockStartTime, blockEndTime, blockOffset);
					}
				}
			}

			break;
		}
		default:
			break;
		}

		/** Other */
		this->dragType = DragType::None;
		this->pressedBlockIndex = -1;
		this->mousePressedSecond = 0;
		this->blockValid = true;
		this->copyMode = false;

		/** Repaint */
		this->repaint();
	}
	else if (event.mods.isRightButtonDown()) {
		/** Check Tool Type */
		switch (tool) {
		case Tools::Type::Pencil: {
			/** Remove Block */
			auto blockController = this->getBlockControllerWithoutEdge(posX);
			if (std::get<0>(blockController) == BlockControllerType::Inside) {
				this->removeBlock(std::get<1>(blockController));
			}
			break;
		}
		case Tools::Type::Arrow: {
			/** Show Menu */
			auto [controller, index] = this->getBlockController(posX);
			double currentSec = this->secStart + (posX / (double)this->getWidth()) * (this->secEnd - this->secStart);
			currentSec = this->limitTimeSec(currentSec);
			this->showMenu(currentSec, index);
			break;
		}
		}
	}
}

void SeqTrackContentViewer::mouseExit(const juce::MouseEvent& event) {
	/** Move View */
	if (this->viewMoving) {
		this->viewMoving = false;
		this->mouseMove(event);/**< Update Mouse Cursor */
		this->dragEndFunc();
	}

	this->dragType = DragType::None;
	this->pressedBlockIndex = -1;
	this->mousePressedSecond = 0;
	this->blockValid = true;
	this->copyMode = false;
	this->mouseCurrentSecond = -1;
	this->repaint();
}

void SeqTrackContentViewer::mouseDoubleClick(const juce::MouseEvent& event) {
	if (event.mods.isLeftButtonDown()) {
		this->trackSelectFunc(this->index);
	}
}

void SeqTrackContentViewer::mouseWheelMove(const juce::MouseEvent& event,
	const juce::MouseWheelDetails& wheel) {
	if (event.mods.isAltDown()) {
		double thumbPer = event.position.getX() / (double)this->getWidth();
		double centerNum = this->secStart + (this->secEnd - this->secStart) * thumbPer;

		this->wheelAltFunc(centerNum, thumbPer, wheel.deltaY, wheel.isReversed);
	}
	else {
		this->wheelFunc(wheel.deltaY, wheel.isReversed);
	}
}

void SeqTrackContentViewer::updateBlockInternal(int blockIndex) {
	if (auto temp = this->blockTemp[blockIndex]) {
		std::tie(temp->startTime, temp->endTime, temp->offset)
			= quickAPI::getBlock(this->index, blockIndex);
	}
}

void SeqTrackContentViewer::setAudioPointTempInternal(
	const juce::Array<juce::MemoryBlock>& temp) {
	this->audioPointTemp = temp;
}

void SeqTrackContentViewer::updateMIDINoteTempInternal() {
	/** Get Note Zone */
	uint8_t minNote = 127, maxNote = 0;
	for (auto& [start, end, num] : this->midiDataTemp) {
		minNote = std::min(minNote, num);
		maxNote = std::max(maxNote, num);
	}
	if (maxNote < minNote) { maxNote = minNote = 0; }
	this->midiMinNote = minNote;
	this->midiMaxNote = maxNote;
}

std::tuple<SeqTrackContentViewer::BlockControllerType, int> 
SeqTrackContentViewer::getBlockController(float posX) const {
	/** Size */
	auto screenSize = utils::getScreenSize(this);
	int blockJudgeWidth = screenSize.getWidth() * 0.005;

	/** Get Each Block */
	for (int i = 0; i < this->blockTemp.size(); i++) {
		/** Get Block Time */
		auto block = this->blockTemp.getUnchecked(i);
		float startX = (block->startTime - this->secStart) / (this->secEnd - this->secStart) * this->getWidth();
		float endX = (block->endTime - this->secStart) / (this->secEnd - this->secStart) * this->getWidth();

		/** Judge Area */
		float judgeSSX = startX - blockJudgeWidth, judgeSEX = startX + blockJudgeWidth;
		float judgeESX = endX - blockJudgeWidth, judgeEEX = endX + blockJudgeWidth;
		if (endX - startX < blockJudgeWidth * 2) {
			judgeSEX = startX + (endX - startX) / 2;
			judgeESX = endX - (endX - startX) / 2;
		}
		if (i > 0) {
			auto lastBlock = this->blockTemp.getUnchecked(i - 1);
			float lastEndX = (lastBlock->endTime - this->secStart) / (this->secEnd - this->secStart) * this->getWidth();
			if (startX - lastEndX < blockJudgeWidth * 2) {
				judgeSSX = startX - (startX - lastEndX) / 2;
			}
		}
		if (i < this->blockTemp.size() - 1) {
			auto nextBlock = this->blockTemp.getUnchecked(i + 1);
			float nextStartX = (nextBlock->startTime - this->secStart) / (this->secEnd - this->secStart) * this->getWidth();
			if (nextStartX - endX < blockJudgeWidth * 2) {
				judgeEEX = endX + (nextStartX - endX) / 2;
			}
		}

		/** Get Controller */
		if (posX >= judgeSSX && posX < judgeSEX) {
			return { BlockControllerType::Left, i };
		}
		else if (posX >= judgeESX && posX < judgeEEX) {
			return { BlockControllerType::Right, i };
		}
		else if (posX >= judgeSEX && posX < judgeESX) {
			return { BlockControllerType::Inside, i };
		}
	}

	/** None */
	return { BlockControllerType::None, -1 };
}

std::tuple<SeqTrackContentViewer::BlockControllerType, int>
SeqTrackContentViewer::getBlockControllerWithoutEdge(float posX) const {
	/** Get Each Block */
	for (int i = 0; i < this->blockTemp.size(); i++) {
		/** Get Block Time */
		auto block = this->blockTemp.getUnchecked(i);
		float startX = (block->startTime - this->secStart) / (this->secEnd - this->secStart) * this->getWidth();
		float endX = (block->endTime - this->secStart) / (this->secEnd - this->secStart) * this->getWidth();

		/** Set Cursor */
		if (posX >= startX && posX < endX) {
			return { BlockControllerType::Inside, i };
		}
	}

	/** None */
	return { BlockControllerType::None, -1 };
}

double SeqTrackContentViewer::limitTimeSec(double timeSec) const {
	timeSec = std::max(timeSec, 0.0);

	double level = Tools::getInstance()->getAdsorb();
	return quickAPI::limitTimeSec(timeSec, level);
}

bool SeqTrackContentViewer::checkBlockValid(
	double startSec, double endSec, int excludeIndex) const {
	if (endSec <= startSec) { return false; }

	for (int i = 0; i < this->blockTemp.size(); i++) {
		if (i != excludeIndex) {
			auto block = this->blockTemp.getUnchecked(i);
			if (block->endTime > startSec &&
				block->startTime < endSec) {
				return false;
			}
		}
	}
	return true;
}

void SeqTrackContentViewer::addBlock(double startSec, double endSec, double offset) {
	if (endSec > startSec) {
		CoreActions::insertSeqBlock(this->index, startSec, endSec, offset);
	}
}

void SeqTrackContentViewer::setBlock(int blockIndex,
	double startSec, double endSec, double offset) {
	if (endSec > startSec) {
		CoreActions::setSeqBlock(this->index, blockIndex,
			startSec, endSec, offset);
	}
}

void SeqTrackContentViewer::splitBlock(int blockIndex, double timeSec) {
	CoreActions::splitSeqBlock(this->index, blockIndex, timeSec);
}

void SeqTrackContentViewer::removeBlock(int blockIndex) {
	CoreActions::removeSeqBlock(this->index, blockIndex);
}

enum SeqBlockMenuActionType {
	Remove = 1, Split, OpenInEditor
};

void SeqTrackContentViewer::showMenu(double seconds, int blockIndex) {
	auto menu = this->createMenu(seconds, blockIndex);
	int result = menu.show();

	switch (result) {
	case SeqBlockMenuActionType::Remove:
		this->removeBlock(blockIndex);
		break;
	case SeqBlockMenuActionType::Split:
		this->splitBlock(blockIndex, seconds);
		break;
	case SeqBlockMenuActionType::OpenInEditor:
		this->trackSelectFunc(this->index);
		break;
	}
}

juce::PopupMenu SeqTrackContentViewer::createMenu(double seconds, int blockIndex) {
	/** Title */
	juce::String title = juce::String{ seconds, 2 } + "s";
	if (blockIndex >= 0) {
		title += (", #" + juce::String{ blockIndex });
	}

	/** Menu */
	juce::PopupMenu menu;

	menu.addSectionHeader(title);
	menu.addItem(SeqBlockMenuActionType::Remove, TRANS("Remove Block"), (blockIndex >= 0));
	menu.addItem(SeqBlockMenuActionType::Split, TRANS("Split Block"), (blockIndex >= 0));
	menu.addItem(SeqBlockMenuActionType::OpenInEditor, TRANS("Open Source In Editor"), true);

	return menu;
}

std::tuple<double, double> SeqTrackContentViewer::getViewArea(double pos, double itemSize) const {
	double secStart = pos / itemSize;
	double secLength = this->getWidth() / itemSize;
	return { secStart, secStart + secLength };
}
