﻿#pragma once

#include <JuceHeader.h>

#include "PluginDecorator.h"

class PluginDock final : public juce::AudioProcessorGraph {
public:
	PluginDock() = delete;
	PluginDock(const juce::AudioChannelSet& type = juce::AudioChannelSet::stereo());
	~PluginDock() override;
	
	/**
	 * @brief	Insert a plugin onto the plugin dock.
	 */
	bool insertPlugin(std::unique_ptr<juce::AudioPluginInstance> processor, int index = -1);
	/**
	 * @brief	Remove a plugin from the plugin dock.
	 */
	void removePlugin(int index);

	int getPluginNum() const;
	PluginDecorator* getPluginProcessor(int index) const;
	void setPluginBypass(int index, bool bypass);

	/**
	 * @brief	Add an audio input bus onto the plugin dock.
	 */
	bool addAdditionalAudioBus();
	/**
	 * @brief	Remove the last audio input bus from the plugin dock.
	 */
	bool removeAdditionalAudioBus();

	void addAdditionalBusConnection(int pluginIndex, int srcChannel, int dstChannel);
	void removeAdditionalBusConnection(int pluginIndex, int srcChannel, int dstChannel);

	using PluginState = std::tuple<juce::String, bool>;
	using PluginStateList = juce::Array<PluginState>;
	PluginStateList getPluginList() const;

	void prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) override;
	void setPlayHead(juce::AudioPlayHead* newPlayHead) override;

private:
	juce::AudioProcessorGraph::Node::Ptr audioInputNode, audioOutputNode;
	juce::AudioProcessorGraph::Node::Ptr midiInputNode;
	juce::AudioChannelSet audioChannels;

	juce::Array<juce::AudioProcessorGraph::Node::Ptr> pluginNodeList;

	juce::Array<juce::AudioProcessorGraph::Connection> additionalConnectionList;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginDock)
};