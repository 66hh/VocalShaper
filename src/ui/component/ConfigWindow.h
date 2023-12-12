﻿#pragma once

#include <JuceHeader.h>

class ConfigWindow final : public juce::DocumentWindow {
public:
	ConfigWindow();
	~ConfigWindow();

	void resized() override;
	void closeButtonPressed() override;

	void setPage(int index);

private:
	juce::Image iconTemp;
	juce::OpenGLContext renderer;
	std::unique_ptr<juce::TooltipWindow> toolTip = nullptr;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConfigWindow)
};
