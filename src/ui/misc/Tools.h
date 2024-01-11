﻿#pragma once

#include <JuceHeader.h>

class Tools final : private juce::DeletedAtShutdown {
public:
	Tools() = default;

	enum class Type {
		Arrow, Hand, Pencil, Magic, Scissors, Eraser
	};
	void setType(Type type);
	Type getType() const;

private:
	Type type = Type::Arrow;

public:
	static Tools* getInstance();
	static void releaseInstance();

private:
	static Tools* instance;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Tools)
};
