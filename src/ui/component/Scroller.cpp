﻿#include "Scroller.h"
#include "../lookAndFeel/LookAndFeelFactory.h"

Scroller::Scroller(bool vertical,
	const ViewSizeFunc& viewSizeCallback,
	const ItemNumFunc& itemNumCallback,
	const ItemSizeLimitFunc& itemSizeLimitCallback,
	const UpdatePosFunc& updatePosCallback,
	const PaintPreviewFunc& paintPreviewCallback,
	const PaintItemPreviewFunc& paintItemPreviewCallback)
	: ScrollerBase(vertical),
	viewSizeCallback(viewSizeCallback), itemNumCallback(itemNumCallback),
	itemSizeLimitCallback(itemSizeLimitCallback), updatePosCallback(updatePosCallback),
	paintPreviewCallback(paintPreviewCallback), paintItemPreviewCallback(paintItemPreviewCallback) {
	/** Look And Feel */
	this->setLookAndFeel(
		LookAndFeelFactory::getInstance()->forScroller());
}

int Scroller::createViewSize() {
	return this->viewSizeCallback();
}

int Scroller::createItemNum() {
	return this->itemNumCallback();
}

std::tuple<int, int> Scroller::createItemSizeLimit() {
	return this->itemSizeLimitCallback();
}

void Scroller::updatePos(int pos, int itemSize) {
	this->updatePosCallback(pos, itemSize);
}

void Scroller::paintPreview(juce::Graphics& g, bool vertical) {
	if (this->paintPreviewCallback) {
		this->paintPreviewCallback(g, vertical);
	}
}

void Scroller::paintItemPreview(juce::Graphics& g, int itemIndex,
	int width, int height, bool vertical) {
	if (this->paintItemPreviewCallback) {
		this->paintItemPreviewCallback(
			g, itemIndex, width, height, vertical);
	}
}