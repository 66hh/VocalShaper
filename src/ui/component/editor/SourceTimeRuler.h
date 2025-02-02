﻿#pragma once

#include <JuceHeader.h>
#include "../../misc/LevelMeterHub.h"

class SourceTimeRuler final
	: public juce::Component,
	public LevelMeterHub::Target {
public:
	using ScrollFunc = std::function<void(double)>;
	using ScaleFunc = std::function<void(double, double, double)>;
	using WheelFunc = std::function<void(float, bool)>;
	using WheelAltFunc = std::function<void(double, double, float, bool)>;
	using DragStartFunc = std::function<void(void)>;
	using DragProcessFunc = std::function<void(int, int, bool, bool)>;
	using DragEndFunc = std::function<void(void)>;
	SourceTimeRuler(const ScrollFunc& scrollFunc,
		const ScaleFunc& scaleFunc,
		const WheelFunc& wheelFunc,
		const WheelAltFunc& wheelAltFunc,
		const DragStartFunc& dragStartFunc,
		const DragProcessFunc& dragProcessFunc,
		const DragEndFunc& dragEndFunc);

	void updateTempoLabel();
	void updateHPos(double pos, double itemSize);
	void updateRulerTemp();
	void updateLevelMeter() override;

	void resized() override;
	void paint(juce::Graphics& g) override;

	void mouseDown(const juce::MouseEvent& event) override;
	void mouseDrag(const juce::MouseEvent& event) override;
	void mouseUp(const juce::MouseEvent& event) override;
	void mouseMove(const juce::MouseEvent& event) override;
	void mouseWheelMove(const juce::MouseEvent& event,
		const juce::MouseWheelDetails& wheel) override;
	void mouseExit(const juce::MouseEvent& event) override;

	/** Place, IsBar, barId */
	using LineItem = std::tuple<double, bool, int>;
	std::tuple<double, double> getViewArea(double pos, double itemSize) const;

	using LineItemList = juce::Array<LineItem>;
	const LineItemList getLineTemp() const;

private:
	const ScrollFunc scrollFunc;
	const ScaleFunc scaleFunc;
	const WheelFunc wheelFunc;
	const WheelAltFunc wheelAltFunc;
	const DragStartFunc dragStartFunc;
	const DragProcessFunc dragProcessFunc;
	const DragEndFunc dragEndFunc;

	double pos = 0, itemSize = 0;
	double secStart = 0, secEnd = 0;
	LineItemList lineTemp;
	std::unique_ptr<juce::Image> rulerTemp = nullptr;
	/** timeInSec, tempo, numerator, denominator, isTempo */
	using TempoLabelData = std::tuple<double, double, int, int, bool>;
	juce::Array<TempoLabelData> tempoTemp;

	double playPosSec = 0;
	double loopStartSec = 0, loopEndSec = 0;

	double mouseDownSecTemp = 0;

	int dragLabelIndex = -1;
	float labelDragOffset = 0;
	float labelDragPos = 0;

	bool viewMoving = false;

	const LineItemList createRulerLine(double pos, double itemSize) const;

	double limitTimeSec(double timeSec);
	int selectTempoLabel(const juce::Point<float> pos);

	void removeTempoLabel(int index);
	void addTempoLabel(double timeSec);
	void setTempoLabelTime(int index, double timeSec);
	void editTempoLabel(int index);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SourceTimeRuler)
};
