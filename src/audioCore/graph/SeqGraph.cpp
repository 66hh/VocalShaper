﻿#include "MainGraph.h"

void MainGraph::insertSource(int index, const juce::AudioChannelSet& type) {
	/** Add To The Graph */
	if (auto ptrNode = this->addNode(std::make_unique<SeqSourceProcessor>(type))) {
		/** Limit Index */
		if (index < 0 || index > this->audioSourceNodeList.size()) {
			index = this->audioSourceNodeList.size();
		}

		/** Add Node To The Source List */
		this->audioSourceNodeList.insert(index, ptrNode);

		/** Prepare To Play */
		ptrNode->getProcessor()->setPlayHead(this->getPlayHead());
		ptrNode->getProcessor()->prepareToPlay(this->getSampleRate(), this->getBlockSize());
	}
	else {
		jassertfalse;
	}
}

void MainGraph::removeSource(int index) {
	/** Limit Index */
	if (index < 0 || index >= this->audioSourceNodeList.size()) { return; }

	/** Get The Node Ptr Then Remove From The List */
	auto ptrNode = this->audioSourceNodeList.removeAndReturn(index);

	/** Remove MIDI Instrument Connection */
	this->midiSrc2InstrConnectionList.removeIf(
		[this, nodeID = ptrNode->nodeID](const juce::AudioProcessorGraph::Connection& element) {
			if (element.source.nodeID == nodeID) {
				this->removeConnection(element);
				return true;
			}
			return false;
		});

	/** Remove MIDI Send Connection */
	this->midiSrc2TrkConnectionList.removeIf(
		[this, nodeID = ptrNode->nodeID](const juce::AudioProcessorGraph::Connection& element) {
			if (element.source.nodeID == nodeID) {
				this->removeConnection(element);
				return true;
			}
			return false;
		});

	/** Remove Audio Send Connection */
	this->audioSrc2TrkConnectionList.removeIf(
		[this, nodeID = ptrNode->nodeID](const juce::AudioProcessorGraph::Connection& element) {
			if (element.source.nodeID == nodeID) {
				this->removeConnection(element);
				return true;
			}
			return false;
		});

	/** Remove Node From Graph */
	this->removeNode(ptrNode->nodeID);
}

PluginDecorator::SafePointer MainGraph::insertInstrument(std::unique_ptr<juce::AudioPluginInstance> processor,
	const juce::String& identifier, int index, const juce::AudioChannelSet& type) {
	/** Add To The Graph */
	if (auto ptr = this->insertInstrument(index, type)) {
		ptr->setPlugin(std::move(processor), identifier);
		return ptr;
	}
	else {
		jassertfalse;
		return nullptr;
	}
}

PluginDecorator::SafePointer MainGraph::insertInstrument(int index,
	const juce::AudioChannelSet& type) {
	/** Add To The Graph */
	if (auto ptrNode = this->addNode(std::make_unique<PluginDecorator>(true, type))) {
		/** Limit Index */
		if (index < 0 || index > this->instrumentNodeList.size()) {
			index = this->instrumentNodeList.size();
		}

		/** Add Node To The Source List */
		this->instrumentNodeList.insert(index, ptrNode);

		/** Prepare To Play */
		ptrNode->getProcessor()->setPlayHead(this->getPlayHead());
		ptrNode->getProcessor()->prepareToPlay(this->getSampleRate(), this->getBlockSize());

		return PluginDecorator::SafePointer{ dynamic_cast<PluginDecorator*>(ptrNode->getProcessor()) };
	}
	else {
		jassertfalse;
		return nullptr;
	}
}

void MainGraph::removeInstrument(int index) {
	/** Limit Index */
	if (index < 0 || index >= this->instrumentNodeList.size()) { return; }

	/** Get The Node Ptr Then Remove From The List */
	auto ptrNode = this->instrumentNodeList.removeAndReturn(index);

	/** Close The Editor */
	if (auto processor = ptrNode->getProcessor()) {
		if (auto editor = juce::Component::SafePointer(processor->getActiveEditor())) {
			juce::MessageManager::callAsync([editor] {if (editor) { delete editor; }});
		}
	}

	/** Remove MIDI Instrument Connection */
	this->midiSrc2InstrConnectionList.removeIf(
		[this, nodeID = ptrNode->nodeID](const juce::AudioProcessorGraph::Connection& element) {
			if (element.destination.nodeID == nodeID) {
				this->removeConnection(element);
				return true;
			}
			return false;
		});

	/** Remove Audio Output Connection */
	this->audioInstr2TrkConnectionList.removeIf(
		[this, nodeID = ptrNode->nodeID](const juce::AudioProcessorGraph::Connection& element) {
			if (element.source.nodeID == nodeID) {
				this->removeConnection(element);
				return true;
			}
			return false;
		});

	/** Remove Node From Graph */
	this->removeNode(ptrNode->nodeID);
}

int MainGraph::getSourceNum() const {
	return this->audioSourceNodeList.size();
}

SeqSourceProcessor* MainGraph::getSourceProcessor(int index) const {
	if (index < 0 || index >= this->audioSourceNodeList.size()) { return nullptr; }
	return dynamic_cast<SeqSourceProcessor*>(
		this->audioSourceNodeList.getUnchecked(index)->getProcessor());
}

int MainGraph::getInstrumentNum() const {
	return this->instrumentNodeList.size();
}

PluginDecorator* MainGraph::getInstrumentProcessor(int index) const {
	if (index < 0 || index >= this->instrumentNodeList.size()) { return nullptr; }
	return dynamic_cast<PluginDecorator*>(
		this->instrumentNodeList.getUnchecked(index)->getProcessor());
}

void MainGraph::setSourceBypass(int index, bool bypass) {
	if (index < 0 || index >= this->audioSourceNodeList.size()) { return; }
	if (auto node = this->audioSourceNodeList.getUnchecked(index)) {
		node->setBypassed(bypass);
	}
}

bool MainGraph::getSourceBypass(int index) const {
	if (index < 0 || index >= this->audioSourceNodeList.size()) { return false; }
	if (auto node = this->audioSourceNodeList.getUnchecked(index)) {
		return node->isBypassed();
	}
	return false;
}

void MainGraph::setInstrumentBypass(int index, bool bypass) {
	if (index < 0 || index >= this->instrumentNodeList.size()) { return; }
	if (auto node = this->instrumentNodeList.getUnchecked(index)) {
		node->setBypassed(bypass);
	}
}

bool MainGraph::getInstrumentBypass(int index) const {
	if (index < 0 || index >= this->instrumentNodeList.size()) { return false; }
	if (auto node = this->instrumentNodeList.getUnchecked(index)) {
		return node->isBypassed();
	}
	return false;
}

void MainGraph::setMIDII2InstrConnection(int instrIndex) {
	/** Limit Index */
	if (instrIndex < 0 || instrIndex >= this->instrumentNodeList.size()) { return; }

	/** Remove Current Connection */
	this->removeMIDII2InstrConnection(instrIndex);

	/** Get Node ID */
	auto nodeID = this->instrumentNodeList.getUnchecked(instrIndex)->nodeID;

	/** Add Connection */
	juce::AudioProcessorGraph::Connection connection =
	{ {this->midiInputNode->nodeID, this->midiChannelIndex}, {nodeID, this->midiChannelIndex} };
	this->addConnection(connection);
	this->midiI2InstrConnectionList.add(connection);
}

void MainGraph::removeMIDII2InstrConnection(int instrIndex) {
	/** Limit Index */
	if (instrIndex < 0 || instrIndex >= this->instrumentNodeList.size()) { return; }

	/** Get Node ID */
	auto nodeID = this->instrumentNodeList.getUnchecked(instrIndex)->nodeID;

	/** Remove Connection */
	this->midiI2InstrConnectionList.removeIf(
		[this, nodeID](const juce::AudioProcessorGraph::Connection& element) {
			if (element.destination.nodeID == nodeID) {
				this->removeConnection(element);
				return true;
			}
			return false;
		});
}

void MainGraph::setMIDISrc2InstrConnection(int sourceIndex, int instrIndex) {
	/** Limit Index */
	if (sourceIndex < 0 || sourceIndex >= this->audioSourceNodeList.size()) { return; }
	if (instrIndex < 0 || instrIndex >= this->instrumentNodeList.size()) { return; }

	/** Remove Current Connection */
	this->removeMIDISrc2InstrConnection(sourceIndex, instrIndex);

	/** Get Node ID */
	auto nodeID = this->audioSourceNodeList.getUnchecked(sourceIndex)->nodeID;
	auto dstNodeID = this->instrumentNodeList.getUnchecked(instrIndex)->nodeID;

	/** Add Connection */
	juce::AudioProcessorGraph::Connection connection =
	{ {nodeID, this->midiChannelIndex}, {dstNodeID, this->midiChannelIndex} };
	this->addConnection(connection);
	this->midiSrc2InstrConnectionList.add(connection);
}

void MainGraph::removeMIDISrc2InstrConnection(int sourceIndex, int instrIndex) {
	/** Limit Index */
	if (sourceIndex < 0 || sourceIndex >= this->audioSourceNodeList.size()) { return; }
	if (instrIndex < 0 || instrIndex >= this->instrumentNodeList.size()) { return; }

	/** Get Node ID */
	auto nodeID = this->audioSourceNodeList.getUnchecked(sourceIndex)->nodeID;
	auto dstNodeID = this->instrumentNodeList.getUnchecked(instrIndex)->nodeID;

	/** Remove Connection */
	this->midiSrc2InstrConnectionList.removeIf(
		[this, nodeID, dstNodeID](const juce::AudioProcessorGraph::Connection& element) {
			if (element.source.nodeID == nodeID &&
				element.destination.nodeID == dstNodeID) {
				this->removeConnection(element);
				return true;
			}
			return false;
		});
}

bool MainGraph::isMIDII2InstrConnected(int instrIndex) const {
	/** Limit Index */
	if (instrIndex < 0 || instrIndex >= this->instrumentNodeList.size()) { return false; }

	/** Get Node ID */
	auto nodeID = this->instrumentNodeList.getUnchecked(instrIndex)->nodeID;

	/** Find Connection */
	juce::AudioProcessorGraph::Connection connection =
	{ {this->midiInputNode->nodeID, this->midiChannelIndex}, {nodeID, this->midiChannelIndex} };
	return this->midiI2InstrConnectionList.contains(connection);
}

bool MainGraph::isMIDISrc2InstrConnected(int sourceIndex, int instrIndex) const {
	/** Limit Index */
	if (sourceIndex < 0 || sourceIndex >= this->audioSourceNodeList.size()) { return false; }
	if (instrIndex < 0 || instrIndex >= this->instrumentNodeList.size()) { return false; }

	/** Get Node ID */
	auto nodeID = this->audioSourceNodeList.getUnchecked(sourceIndex)->nodeID;
	auto dstNodeID = this->instrumentNodeList.getUnchecked(instrIndex)->nodeID;

	/** Find Connection */
	juce::AudioProcessorGraph::Connection connection =
	{ {nodeID, this->midiChannelIndex}, {dstNodeID, this->midiChannelIndex} };
	return this->midiSrc2InstrConnectionList.contains(connection);
}

utils::AudioConnectionList MainGraph::getInstrOutputToTrackConnections(int index) const {
	/** Check Index */
	if (index < 0 || index >= this->instrumentNodeList.size()) {
		return utils::AudioConnectionList{};
	}

	/** Get Current Instr ID */
	juce::AudioProcessorGraph::NodeID currentID
		= this->instrumentNodeList.getUnchecked(index)->nodeID;
	utils::AudioConnectionList resultList;

	for (auto& i : this->audioInstr2TrkConnectionList) {
		if (i.source.nodeID == currentID) {
			/** Get Destination Track Index */
			int destIndex = this->trackNodeList.indexOf(
				this->getNodeForId(i.destination.nodeID));
			if (destIndex < 0 || destIndex >= this->trackNodeList.size()) {
				continue;
			}

			/** Add To Result */
			resultList.add(std::make_tuple(
				index, i.source.channelIndex, destIndex, i.destination.channelIndex));
		}
	}

	/** Sort Result */
	class SortComparator {
	public:
		int compareElements(utils::AudioConnection& first, utils::AudioConnection& second) {
			if (std::get<1>(first) == std::get<1>(second)) {
				if (std::get<2>(first) == std::get<2>(second)) {
					return std::get<3>(first) - std::get<3>(second);
				}
				return std::get<2>(first) - std::get<2>(second);
			}
			return std::get<1>(first) - std::get<1>(second);
		}
	} comparator;
	resultList.sort(comparator, true);

	return resultList;
}

utils::MidiConnectionList MainGraph::getInstrMidiInputFromSrcConnections(int index) const {
	/** Check Index */
	if (index < 0 || index >= this->instrumentNodeList.size()) {
		return utils::MidiConnectionList{};
	}

	/** Get Current Instr ID */
	juce::AudioProcessorGraph::NodeID currentID
		= this->instrumentNodeList.getUnchecked(index)->nodeID;
	utils::MidiConnectionList resultList;

	for (auto& i : this->midiSrc2InstrConnectionList) {
		if (i.destination.nodeID == currentID) {
			/** Get Source Track Index */
			int sourceIndex = this->audioSourceNodeList.indexOf(
				this->getNodeForId(i.source.nodeID));
			if (sourceIndex < 0 || sourceIndex >= this->audioSourceNodeList.size()) {
				continue;
			}

			/** Add To Result */
			resultList.add(std::make_tuple(sourceIndex, index));
		}
	}

	/** Sort Result */
	class SortComparator {
	public:
		int compareElements(utils::MidiConnection& first, utils::MidiConnection& second) {
			return std::get<0>(first) - std::get<0>(second);
		}
	} comparator;
	resultList.sort(comparator, true);

	return resultList;
}
utils::MidiConnectionList MainGraph::getInstrMidiInputFromDeviceConnections(int index) const {
	/** Check Index */
	if (index < 0 || index >= this->instrumentNodeList.size()) {
		return utils::MidiConnectionList{};
	}

	/** Get Current Instr ID */
	juce::AudioProcessorGraph::NodeID currentID
		= this->instrumentNodeList.getUnchecked(index)->nodeID;
	utils::MidiConnectionList resultList;

	for (auto& i : this->midiI2InstrConnectionList) {
		if (i.destination.nodeID == currentID) {
			/** Add To Result */
			resultList.add(std::make_tuple(-1, index));
		}
	}

	/** Sort Result */
	class SortComparator {
	public:
		int compareElements(utils::MidiConnection& first, utils::MidiConnection& second) {
			return std::get<0>(first) - std::get<0>(second);
		}
	} comparator;
	resultList.sort(comparator, true);

	return resultList;
}

void MainGraph::closeAllNote() {
	for (auto& i : this->audioSourceNodeList) {
		auto seqTrack = dynamic_cast<SeqSourceProcessor*>(i->getProcessor());
		if (seqTrack) {
			seqTrack->closeAllNote();
		}
	}
}

int MainGraph::findSource(const SeqSourceProcessor* ptr) const {
	for (int i = 0; i < this->audioSourceNodeList.size(); i++) {
		if (this->audioSourceNodeList.getUnchecked(i)->getProcessor() == ptr) {
			return i;
		}
	}
	return -1;
}

int MainGraph::findInstr(const PluginDecorator* ptr) const {
	for (int i = 0; i < this->instrumentNodeList.size(); i++) {
		if (this->instrumentNodeList.getUnchecked(i)->getProcessor() == ptr) {
			return i;
		}
	}
	return -1;
}
