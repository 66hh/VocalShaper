#include "CloneableAudioSource.h"

double CloneableAudioSource::getSourceSampleRate() const {
	return this->sourceSampleRate;
}

void CloneableAudioSource::readData(juce::AudioBuffer<float>& buffer, double bufferDeviation,
	double dataDeviation, double length) const {
	if (this->source && this->memorySource) {
		this->memorySource->setNextReadPosition(dataDeviation * this->sourceSampleRate);
		this->source->getNextAudioBlock(juce::AudioSourceChannelInfo{
			&buffer, (int)std::floor(bufferDeviation * this->getSampleRate()),
				(int)std::floor(length * this->getSampleRate())});
	}
}

int CloneableAudioSource::getChannelNum() const {
	return this->buffer.getNumChannels();
}

bool CloneableAudioSource::clone(const CloneableSource* src) {
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
	this->source = std::make_unique<juce::ResamplingAudioSource>(this->memorySource.get(), false, this->buffer.getNumChannels());

	/** Set Sample Rate */
	this->source->setResamplingRatio(this->sourceSampleRate / this->getSampleRate());

	return true;
}

bool CloneableAudioSource::load(const juce::File& file) {
	/** Create Audio Reader */
	auto audioReader = CloneableAudioSource::createAudioReader(file);
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
	this->source = std::make_unique<juce::ResamplingAudioSource>(this->memorySource.get(), false, this->buffer.getNumChannels());

	/** Set Sample Rate */
	this->source->setResamplingRatio(this->sourceSampleRate / this->getSampleRate());

	return true;
}

bool CloneableAudioSource::save(const juce::File& file) const {
	/** Create Audio Writer */
	auto audioWriter = CloneableAudioSource::createAudioWriter(file,
		this->sourceSampleRate, juce::AudioChannelSet::canonicalChannelSet(this->buffer.getNumChannels()),
		this->bitsPerSample, this->metadataValues, this->qualityOptionIndex);
	if (!audioWriter) { return false; }

	/** Write Data */
	audioWriter->writeFromAudioSampleBuffer(this->buffer, 0, this->buffer.getNumSamples());

	return true;
}

double CloneableAudioSource::getLength() const {
	return this->buffer.getNumSamples() / this->sourceSampleRate;
}

void CloneableAudioSource::sampleRateChanged() {
	if (this->source) {
		this->source->setResamplingRatio(this->sourceSampleRate / this->getSampleRate());
	}
}

class SingletonAudioFormatManager : public juce::AudioFormatManager,
	private juce::DeletedAtShutdown {
public:
	SingletonAudioFormatManager();

public:
	static SingletonAudioFormatManager* getInstance();

private:
	static SingletonAudioFormatManager* instance;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SingletonAudioFormatManager)
};

SingletonAudioFormatManager::SingletonAudioFormatManager()
	: AudioFormatManager() {
	this->registerBasicFormats();
}

SingletonAudioFormatManager* SingletonAudioFormatManager::instance = nullptr;

SingletonAudioFormatManager* SingletonAudioFormatManager::getInstance() {
	return SingletonAudioFormatManager::instance ? SingletonAudioFormatManager::instance : SingletonAudioFormatManager::instance = new SingletonAudioFormatManager();
}

juce::AudioFormat* CloneableAudioSource::findAudioFormat(const juce::File& file) {
	return SingletonAudioFormatManager::getInstance()->findFormatForFileExtension(file.getFileExtension());
}

std::unique_ptr<juce::AudioFormatReader> CloneableAudioSource::createAudioReader(const juce::File& file) {
	auto format = CloneableAudioSource::findAudioFormat(file);
	if (!format) { return nullptr; }

	return std::unique_ptr<juce::AudioFormatReader>(format->createReaderFor(new juce::FileInputStream(file), true));
}

std::unique_ptr<juce::AudioFormatWriter> CloneableAudioSource::createAudioWriter(const juce::File& file,
	double sampleRateToUse, const juce::AudioChannelSet& channelLayout,
	int bitsPerSample, const juce::StringPairArray& metadataValues, int qualityOptionIndex) {
	auto format = CloneableAudioSource::findAudioFormat(file);
	if (!format) { return nullptr; }

	auto outStream = new juce::FileOutputStream(file);
	if (outStream->openedOk()) {
		outStream->setPosition(0);
		outStream->truncate();
	}

	auto writer = format->createWriterFor(outStream,
		sampleRateToUse, channelLayout, bitsPerSample, metadataValues, qualityOptionIndex);
	if (!writer) {
		delete outStream;
		return nullptr;
	}

	return std::unique_ptr<juce::AudioFormatWriter>(writer);
}
