﻿#pragma once

#include <Defs.h>

class Sequencer final : public juce::AudioProcessorGraph {
public:
	Sequencer();

private:
	juce::AudioProcessorGraph::Node::Ptr midiInputNode, midiOutputNode;
	juce::AudioProcessorGraph::Node::Ptr audioInputNode, audioOutputNode;

	juce::Array<juce::AudioProcessorGraph::Node::Ptr> audioSourceNodeList;
	juce::Array<juce::AudioProcessorGraph::Node::Ptr> instrumentNodeList;

	juce::Array<juce::AudioProcessorGraph::Connection> midiInputConnectionList;
	juce::Array<juce::AudioProcessorGraph::Connection> audioInputConnectionList;
	juce::Array<juce::AudioProcessorGraph::Connection> midiInstrumentConnectionList;
	juce::Array<juce::AudioProcessorGraph::Connection> midiSendConnectionList;
	juce::Array<juce::AudioProcessorGraph::Connection> audioSendConnectionList;
	juce::Array<juce::AudioProcessorGraph::Connection> audioOutputConnectionList;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Sequencer)
};
