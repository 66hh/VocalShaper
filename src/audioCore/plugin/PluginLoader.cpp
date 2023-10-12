#include "PluginLoader.h"
#include "../AudioCore.h"
#include "../Utils.h"

PluginLoader::PluginLoader() {
	this->loadThread = std::make_unique<PluginLoadThread>();
}

void PluginLoader::loadPlugin(
	const juce::PluginDescription& pluginInfo,
	PluginDecorator::SafePointer ptr, const Callback& callback) {
	/** Get Audio Config */
	auto mainGraph = AudioCore::getInstance()->getGraph();
	if (!mainGraph) {
		jassertfalse;
		return;
	}
	double sampleRate = mainGraph->getSampleRate();
	int bufferSize = mainGraph->getBlockSize();

	/** Create Task */
	this->loadThread->load(pluginInfo,
		{ PluginLoadThread::DstPointer::Type::Plugin, ptr, nullptr }, callback, sampleRate, bufferSize);
}

void PluginLoader::loadPlugin(const juce::PluginDescription& pluginInfo,
	CloneableSource::SafePointer<CloneableSynthSource> ptr, const Callback& callback) {
	/** Get Audio Config */
	auto mainGraph = AudioCore::getInstance()->getGraph();
	if (!mainGraph) {
		jassertfalse;
		return;
	}
	double sampleRate = mainGraph->getSampleRate();
	int bufferSize = mainGraph->getBlockSize();

	/** Create Task */
	this->loadThread->load(pluginInfo,
		{ PluginLoadThread::DstPointer::Type::Synth, nullptr, ptr }, callback, sampleRate, bufferSize);
}

bool PluginLoader::isRunning() const {
	return this->loadThread->isThreadRunning();
}

PluginLoader* PluginLoader::getInstance() {
	return PluginLoader::instance ? PluginLoader::instance : (PluginLoader::instance = new PluginLoader());
}

PluginLoader* PluginLoader::instance = nullptr;
