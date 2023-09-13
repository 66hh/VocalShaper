#include "CloneableAudioSource.h"

#include "../Utils.h"

double CloneableAudioSource::getSourceSampleRate() const {
	return this->sourceSampleRate;
}

void CloneableAudioSource::readData(juce::AudioBuffer<float>& buffer, double bufferOffset,
	double dataOffset, double length) const {
	if (this->checkRecording()) { return; }
	if (buffer.getNumSamples() <= 0 || length <= 0) { return; }

	juce::ScopedTryReadLock locker(this->lock);
	if (locker.isLocked()) {
		if (this->source && this->memorySource) {
			this->memorySource->setNextReadPosition(std::floor(dataOffset * this->sourceSampleRate));
			int startSample = (int)std::floor(bufferOffset * this->getSampleRate());
			this->source->getNextAudioBlock(juce::AudioSourceChannelInfo{
				&buffer, startSample,
					std::min(buffer.getNumSamples() - startSample, (int)std::ceil(length * this->getSampleRate()))});/**< Ceil then min with buffer size to ensure audio data fill the last point in buffer */
		}
	}
}

int CloneableAudioSource::getChannelNum() const {
	return this->buffer.getNumChannels();
}

bool CloneableAudioSource::clone(const CloneableSource* src) {
	/** Check Not Recording */
	if (this->checkRecording()) { return false; }

	/** Lock */
	juce::ScopedWriteLock locker(this->lock);

	/** Check Source Type */
	auto ptrSrc = dynamic_cast<const CloneableAudioSource*>(src);
	if (!ptrSrc) { return false; }

	/** Clear Audio Source */
	this->source = nullptr;
	this->memorySource = nullptr;

	/** Copy Data */
	this->buffer = ptrSrc->buffer;
	this->sourceSampleRate = ptrSrc->sourceSampleRate;

	/** Create Audio Source */
	this->memorySource = std::make_unique<juce::MemoryAudioSource>(this->buffer, false, false);
	auto source = std::make_unique<juce::ResamplingAudioSource>(this->memorySource.get(), false, this->buffer.getNumChannels());

	/** Set Sample Rate */
	source->setResamplingRatio(this->sourceSampleRate / this->getSampleRate());

	this->source = std::move(source);
	return true;
}

bool CloneableAudioSource::load(const juce::File& file) {
	/** Check Not Recording */
	if (this->checkRecording()) { return false; }

	/** Lock */
	juce::ScopedWriteLock locker(this->lock);

	/** Create Audio Reader */
	auto audioReader = utils::createAudioReader(file);
	if (!audioReader) { return false; }

	/** Clear Audio Source */
	this->source = nullptr;
	this->memorySource = nullptr;

	/** Copy Data */
	this->buffer = juce::AudioSampleBuffer{ (int)audioReader->numChannels, (int)audioReader->lengthInSamples };
	audioReader->read(&(this->buffer), 0, audioReader->lengthInSamples, 0, true, true);
	this->sourceSampleRate = audioReader->sampleRate;

	/** Create Audio Source */
	this->memorySource = std::make_unique<juce::MemoryAudioSource>(this->buffer, false, false);
	auto source = std::make_unique<juce::ResamplingAudioSource>(this->memorySource.get(), false, this->buffer.getNumChannels());

	/** Set Sample Rate */
	source->setResamplingRatio(this->sourceSampleRate / this->getSampleRate());
	source->prepareToPlay(this->getBufferSize(), this->getSampleRate());

	this->source = std::move(source);
	return true;
}

bool CloneableAudioSource::save(const juce::File& file) const {
	juce::ScopedReadLock locker(this->lock);

	/** Create Audio Writer */
	auto audioWriter = utils::createAudioWriter(file, this->sourceSampleRate,
		juce::AudioChannelSet::canonicalChannelSet(this->buffer.getNumChannels()));
	if (!audioWriter) { return false; }

	/** Write Data */
	audioWriter->writeFromAudioSampleBuffer(this->buffer, 0, this->buffer.getNumSamples());

	return true;
}

double CloneableAudioSource::getLength() const {
	juce::ScopedReadLock locker(this->lock);
	return this->buffer.getNumSamples() / this->sourceSampleRate;
}

void CloneableAudioSource::sampleRateChanged() {
	juce::ScopedWriteLock locker(this->lock);
	if (this->source) {
		this->source->setResamplingRatio(this->sourceSampleRate / this->getSampleRate());
		this->source->prepareToPlay(this->getBufferSize(), this->getSampleRate());
	}
}

void CloneableAudioSource::prepareToRecord(
	int inputChannels, double sampleRate, int blockSize, bool /*updateOnly*/) {
	juce::ScopedWriteLock locker(this->lock);

	if (this->getSourceSampleRate() != sampleRate) {
		this->buffer.clear();
	}

	this->buffer.setSize(
		inputChannels, this->buffer.getNumSamples(), true, false, true);

	this->prepareToPlay(sampleRate, blockSize);
}

void CloneableAudioSource::recordingFinished() {
	/** Nothing To Do */
}

void CloneableAudioSource::writeData(
	const juce::AudioBuffer<float>& buffer, double offset)	{
	/** Lock */
	juce::ScopedTryWriteLock locker(this->lock);
	if (locker.isLocked()) {
		/** Get Time */
		int startSample = offset * this->getSampleRate();
		int srcStartSample = 0;
		int length = buffer.getNumSamples();
		if (startSample < 0) {
			srcStartSample -= startSample;
			length -= srcStartSample;
			startSample = 0;
		}

		/** Increase BufferLength */
		if (startSample > this->buffer.getNumSamples() - length) {
			int newLength = startSample + length;
			this->buffer.setSize(
				this->buffer.getNumChannels(), newLength, true, false, true);
		}

		/** CopyData */
		for (int i = 0; i < buffer.getNumChannels(); i++) {
			if (auto rptr = buffer.getReadPointer(i)) {
				this->buffer.copyFrom(
					i, startSample, &(rptr)[srcStartSample], length);
			}
		}
	}
}
