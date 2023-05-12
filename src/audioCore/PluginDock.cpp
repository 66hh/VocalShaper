﻿#include "PluginDock.h"

PluginDock::PluginDock(const juce::AudioChannelSet& type) 
	: audioChannels(type) {
	/** Set Channel Layout */
	this->setChannelLayoutOfBus(true, 0, type);
	this->setChannelLayoutOfBus(false, 0, type);

	/** The Main Audio IO Node Of The Dock */
	this->audioInputNode = this->addNode(
		std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
			juce::AudioProcessorGraph::AudioGraphIOProcessor::audioInputNode));
	this->audioOutputNode = this->addNode(
		std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
			juce::AudioProcessorGraph::AudioGraphIOProcessor::audioOutputNode));

	/** The Main MIDI Input Node Of The Dock */
	this->midiInputNode = this->addNode(
		std::make_unique<juce::AudioProcessorGraph::AudioGraphIOProcessor>(
			juce::AudioProcessorGraph::AudioGraphIOProcessor::midiInputNode));

	/** Connect The Audio Input And Output Node */
	int mainBusChannels = this->getMainBusNumInputChannels();
	for (int i = 0; i < mainBusChannels; i++) {
		this->addConnection(
			{ {this->audioInputNode->nodeID, i}, {this->audioOutputNode->nodeID, i} });
	}
}

PluginDock::~PluginDock() {
	for (auto& i : this->pluginNodeList) {
		if (auto processor = i->getProcessor()) {
			if (auto editor = processor->getActiveEditor()) {
				if (editor) { delete editor; }
			}
		}
	}
}

bool PluginDock::insertPlugin(std::unique_ptr<juce::AudioProcessor> processor, int index) {
	/** Add To The Graph */
	auto ptrNode = this->addNode(std::move(processor));
	if (ptrNode) {
		/** Limit Index */
		if (index < 0 || index > this->pluginNodeList.size()) {
			index = this->pluginNodeList.size();
		}

		/** Connect Node To The Audio Bus */
		{
			/** Find Hot Spot Nodes */
			juce::AudioProcessorGraph::Node::Ptr lastNode, nextNode;
			if (index == 0) {
				lastNode = this->audioInputNode;
			}
			else {
				lastNode = this->pluginNodeList.getUnchecked(index - 1);
			}
			if (index == this->pluginNodeList.size()) {
				nextNode = this->audioOutputNode;
			}
			else {
				nextNode = this->pluginNodeList.getUnchecked(index);
			}

			/** Get Main Bus */
			int mainBusChannels = this->getMainBusNumInputChannels();

			/** Get Plugin Bus */
			int pluginInputChannels = ptrNode->getProcessor()->getMainBusNumInputChannels();
			int pluginOutputChannels = ptrNode->getProcessor()->getMainBusNumOutputChannels();

			/** Check Channels */
			if (pluginInputChannels < mainBusChannels || pluginOutputChannels < mainBusChannels) {
				this->removeNode(ptrNode->nodeID);
				return false;
			}

			/** Remove Connection Between Hot Spot Nodes */
			for (int i = 0; i < mainBusChannels; i++) {
				this->removeConnection(
					{ {lastNode->nodeID, i}, {nextNode->nodeID, i} });
			}

			/** Add Connection To Hot Spot Nodes */
			for (int i = 0; i < mainBusChannels; i++) {
				this->addConnection(
					{ {lastNode->nodeID, i}, {ptrNode->nodeID, i} });
				this->addConnection(
					{ {ptrNode->nodeID, i}, {nextNode->nodeID, i} });
			}
		}

		/** Connect Node To MIDI Input */
		this->addConnection(
			{ {this->midiInputNode->nodeID, this->midiChannelIndex}, {ptrNode->nodeID, this->midiChannelIndex} });

		/** Add Node To The Plugin List */
		this->pluginNodeList.insert(index, ptrNode);

		/** Prepare To Play */
		ptrNode->getProcessor()->prepareToPlay(this->getSampleRate(), this->getBlockSize());

		return true;
	}
	else {
		jassertfalse;
		return false;
	}
}

void PluginDock::removePlugin(int index) {
	/** Limit Index */
	if (index < 0 || index >= this->pluginNodeList.size()) { return; }

	/** Get The Node Ptr Then Remove From The List */
	auto ptrNode = this->pluginNodeList.removeAndReturn(index);

	/** Close The Editor */
	if (auto processor = ptrNode->getProcessor()) {
		if (auto editor = juce::Component::SafePointer(processor->getActiveEditor())) {
			juce::MessageManager::callAsync([editor] {if (editor) { delete editor; }});
		}
	}

	/** Remove Node From Graph */
	this->removeNode(ptrNode->nodeID);

	/** Connect The Last Node With The Next Node */
	{
		/** Find Hot Spot Nodes */
		juce::AudioProcessorGraph::Node::Ptr lastNode, nextNode;
		if (index == 0) {
			lastNode = this->audioInputNode;
		}
		else {
			lastNode = this->pluginNodeList.getUnchecked(index - 1);
		}
		if (index == this->pluginNodeList.size()) {
			nextNode = this->audioOutputNode;
		}
		else {
			nextNode = this->pluginNodeList.getUnchecked(index);
		}

		/** Get Main Bus */
		int mainBusChannels = this->getMainBusNumInputChannels();

		/** Add Connection Between Hot Spot Nodes */
		for (int i = 0; i < mainBusChannels; i++) {
			this->addConnection(
				{ {lastNode->nodeID, i}, {nextNode->nodeID, i} });
		}
	}
}

int PluginDock::getPluginNum() const {
	return this->pluginNodeList.size();
}

juce::AudioPluginInstance* PluginDock::getPluginProcessor(int index) const {
	if (index < 0 || index >= this->pluginNodeList.size()) { return nullptr; }
	return dynamic_cast<juce::AudioPluginInstance*>(
		this->pluginNodeList.getUnchecked(index)->getProcessor());
}

void PluginDock::setPluginBypass(int index, bool bypass) {
	if (index < 0 || index >= this->pluginNodeList.size()) { return; }
	auto node = this->pluginNodeList.getUnchecked(index);
	if (node) {
		node->setBypassed(bypass);
	}
}

void PluginDock::addAdditionalAudioBus() {
	/** Prepare Bus Layout */
	auto layout = this->getBusesLayout();
	layout.inputBuses.add(this->audioChannels);

	/** Set Bus Layout Of Current Graph */
	this->setBusesLayout(layout);

	/** Set Bus Layout Of Input Node */
	juce::AudioProcessorGraph::BusesLayout inputLayout = layout;
	inputLayout.outputBuses = inputLayout.inputBuses;
	this->audioInputNode->getProcessor()->setBusesLayout(inputLayout);
}

void PluginDock::removeAdditionalAudioBus() {
	/** Check Has Additional Bus */
	auto layout = this->getBusesLayout();
	if (layout.inputBuses.size() > 1) {
		/** Prepare Bus Layout */
		layout.inputBuses.removeLast();

		/** Set Bus Layout Of Current Graph */
		this->setBusesLayout(layout);

		/** Set Bus Layout Of Input Node */
		juce::AudioProcessorGraph::BusesLayout inputLayout = layout;
		inputLayout.outputBuses = inputLayout.inputBuses;
		this->audioInputNode->getProcessor()->setBusesLayout(inputLayout);

		/** Auto Remove Connection */
		this->removeIllegalConnections();
	}
}

PluginDock::PluginStateList PluginDock::getPluginList() const {
	PluginDock::PluginStateList result;

	for (auto& plugin : this->pluginNodeList) {
		result.add(std::make_tuple(
			plugin->getProcessor()->getName(), !plugin->isBypassed()));
	}

	return result;
}

void PluginDock::prepareToPlay(double sampleRate, int maximumExpectedSamplesPerBlock) {
	/** PluginDock */
	this->juce::AudioProcessorGraph::prepareToPlay(sampleRate, maximumExpectedSamplesPerBlock);

	/** Plugins */
	for (auto& i : this->pluginNodeList) {
		auto plugin = i->getProcessor();
		plugin->prepareToPlay(sampleRate, maximumExpectedSamplesPerBlock);
	}
}
