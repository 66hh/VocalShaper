﻿#pragma once

#include <JuceHeader.h>
#include <FlowUI.h>
#include "Scroller.h"
#include "MixerTrackComponent.h"

class MixerView final : public flowUI::FlowComponent {
public:
	MixerView();

	void resized() override;
	void paint(juce::Graphics& g) override;

	void update();

private:
	std::unique_ptr<Scroller> hScroller = nullptr;
	juce::OwnedArray<MixerTrackComponent> trackList;
	juce::Array<juce::Colour> colorTemp;

	int getViewWidth() const;
	int getTrackNum() const;
	std::tuple<int, int> getTrackWidthLimit() const;

	void updatePos(int pos, int itemSize);

	void paintTrackPreview(juce::Graphics& g, int itemIndex,
		int width, int height, bool vertical);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerView)
};