#pragma once

#include <JuceHeader.h>

class MovablePlayHead : public juce::AudioPlayHead {
public:
	MovablePlayHead() = default;
	~MovablePlayHead() override = default;

	juce::Optional<juce::AudioPlayHead::PositionInfo> getPosition() const override;
	const juce::ReadWriteLock& getLock() const;

	bool canControlTransport() override;
	void transportPlay(bool shouldStartPlaying) override;
	void transportRecord(bool shouldStartRecording) override;
	void transportRewind() override;

	void setTimeFormat(short ticksPerQuarter);
	void setSampleRate(double sampleRate);
	void setLooping(bool looping);
	void setLoopPointsInSeconds(const std::tuple<double, double>& points);
	void setLoopPointsInQuarter(const std::tuple<double, double>& points);
	void setPositionInSeconds(double time);
	void setPositionInQuarter(double time);
	void setPositionInSamples(int64_t sampleNum);
	void next(int blockSize);

	double toSecond(double timeTick) const;
	double toTick(double timeSecond) const;
	std::tuple<int, double> toBar(double timeSecond) const;
	double toSecond(double timeTick, short timeFormat) const;
	double toTick(double timeSecond, short timeFormat) const;
	std::tuple<int, double> toBar(double timeSecond, short timeFormat) const;

	juce::MidiMessageSequence& getTempoSequence();

protected:
	mutable juce::AudioPlayHead::PositionInfo position;
	juce::MidiMessageSequence tempos;
	std::atomic_short timeFormat = 480;
	std::atomic<double> sampleRate = 48000;
	juce::ReadWriteLock lock;

	void updatePositionByTimeInSecond();
	void updatePositionByTimeInSample();

private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MovablePlayHead)
};

class PlayPosition final : public MovablePlayHead,
	private juce::DeletedAtShutdown {
public:
	PlayPosition() = default;
	~PlayPosition() override = default;

public:
	static PlayPosition* getInstance();
	static void releaseInstance();

private:
	static PlayPosition* instance;
	static juce::CriticalSection instanceGuard;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PlayPosition)
};