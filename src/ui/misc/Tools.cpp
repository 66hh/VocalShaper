﻿#include "Tools.h"

void Tools::setType(Type type) {
	this->type = type;
}

Tools::Type Tools::getType() const {
	return this->type;
}

void Tools::setAdsorb(double adsorbLevel) {
	this->adsorbLevel = adsorbLevel;
}

double Tools::getAdsorb() const {
	return this->adsorbLevel;
}

void Tools::setFollow(bool follow) {
	this->editorFollow = follow;
}

bool Tools::getFollow() const {
	return this->editorFollow;
}

void Tools::setMIDIChannel(uint8_t channel) {
	if (channel >= 1 && channel <= 16) {
		this->midiChannel = channel;
		this->midiChannelBroadcaster.sendChangeMessage();
	}
}

uint8_t Tools::getMIDIChannel() const {
	return this->midiChannel;
}

void Tools::addMIDIChannelChangedListener(juce::ChangeListener* listener) {
	this->midiChannelBroadcaster.addChangeListener(listener);
}

Tools* Tools::getInstance() {
	return Tools::instance ? Tools::instance
		: (Tools::instance = new Tools{});
}

void Tools::releaseInstance() {
	if (Tools::instance) {
		delete Tools::instance;
		Tools::instance = nullptr;
	}
}

Tools* Tools::instance = nullptr;
