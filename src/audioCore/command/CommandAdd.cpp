﻿#include "CommandUtils.h"
#include "../Utils.h"

AUDIOCORE_FUNC(addPluginBlackList) {
	auto action = std::unique_ptr<ActionBase>(new ActionAddPluginBlackList{
		juce::String::fromUTF8(luaL_checkstring(L, 1)) });
	ActionDispatcher::getInstance()->dispatch(std::move(action));
	return CommandFuncResult{ true, "" };
}

AUDIOCORE_FUNC(addPluginSearchPath) {
	auto action = std::unique_ptr<ActionBase>(new ActionAddPluginSearchPath{
		juce::String::fromUTF8(luaL_checkstring(L, 1)) });
	ActionDispatcher::getInstance()->dispatch(std::move(action));
	return CommandFuncResult{ true, "" };
}

AUDIOCORE_FUNC(addMixerTrack) {
	auto action = std::unique_ptr<ActionBase>(new ActionAddMixerTrack{
		(int)luaL_checkinteger(L, 1), (int)luaL_checkinteger(L, 2) });
	ActionDispatcher::getInstance()->dispatch(std::move(action));
	return CommandFuncResult{ true, "" };
}

AUDIOCORE_FUNC(addMixerTrackSend) {
	auto action = std::unique_ptr<ActionBase>(new ActionAddMixerTrackSend{
		(int)luaL_checkinteger(L, 1), (int)luaL_checkinteger(L, 2),
		(int)luaL_checkinteger(L, 3), (int)luaL_checkinteger(L, 4) });
	ActionDispatcher::getInstance()->dispatch(std::move(action));
	return CommandFuncResult{ true, "" };
}

AUDIOCORE_FUNC(addMixerTrackInputFromDevice) {
	auto action = std::unique_ptr<ActionBase>(new ActionAddMixerTrackInputFromDevice{
		(int)luaL_checkinteger(L, 1), (int)luaL_checkinteger(L, 2),
		(int)luaL_checkinteger(L, 3) });
	ActionDispatcher::getInstance()->dispatch(std::move(action));
	return CommandFuncResult{ true, "" };
}

AUDIOCORE_FUNC(addMixerTrackOutput) {
	auto action = std::unique_ptr<ActionBase>(new ActionAddMixerTrackOutput{
		(int)luaL_checkinteger(L, 1), (int)luaL_checkinteger(L, 2),
		(int)luaL_checkinteger(L, 3) });
	ActionDispatcher::getInstance()->dispatch(std::move(action));
	return CommandFuncResult{ true, "" };
}

AUDIOCORE_FUNC(addEffect) {
	auto action = std::unique_ptr<ActionBase>(new ActionAddEffect{
		(int)luaL_checkinteger(L, 1), (int)luaL_checkinteger(L, 2),
		juce::String::fromUTF8(luaL_checkstring(L, 3)) });
	ActionDispatcher::getInstance()->dispatch(std::move(action));
	return CommandFuncResult{ true, "" };
}

AUDIOCORE_FUNC(addEffectAdditionalInput) {
	auto action = std::unique_ptr<ActionBase>(new ActionAddEffectAdditionalInput{
		(int)luaL_checkinteger(L, 1), (int)luaL_checkinteger(L, 2),
		(int)luaL_checkinteger(L, 3), (int)luaL_checkinteger(L, 4) });
	ActionDispatcher::getInstance()->dispatch(std::move(action));
	return CommandFuncResult{ true, "" };
}

AUDIOCORE_FUNC(addInstr) {
	auto action = std::unique_ptr<ActionBase>(new ActionAddInstr{
		(int)luaL_checkinteger(L, 1), (int)luaL_checkinteger(L, 2),
		juce::String::fromUTF8(luaL_checkstring(L, 3)) });
	ActionDispatcher::getInstance()->dispatch(std::move(action));
	return CommandFuncResult{ true, "" };
}

AUDIOCORE_FUNC(addInstrOutput) {
	juce::String result;

	if (auto graph = audioCore->getGraph()) {
		int src = luaL_checkinteger(L, 1);
		int srcc = luaL_checkinteger(L, 2);
		int dst = luaL_checkinteger(L, 3);
		int dstc = luaL_checkinteger(L, 4);
		graph->setAudioInstr2TrkConnection(src, dst, srcc, dstc);

		result += juce::String(src) + ", " + juce::String(srcc) + " - " + juce::String(dst) + ", " + juce::String(dstc) + "\n";
	}

	return CommandFuncResult{ true, result };
}

AUDIOCORE_FUNC(addInstrMidiInput) {
	juce::String result;

	if (auto graph = audioCore->getGraph()) {
		int dst = luaL_checkinteger(L, 1);
		graph->setMIDII2InstrConnection(dst);

		result += juce::String("[MIDI Input]") + " - " + juce::String(dst) + "\n";
	}

	return CommandFuncResult{ true, result };
}

AUDIOCORE_FUNC(addMixerTrackMidiInput) {
	juce::String result;

	if (auto graph = audioCore->getGraph()) {
		int dst = luaL_checkinteger(L, 1);
		graph->setMIDII2TrkConnection(dst);

		result += juce::String("[MIDI Input]") + " - " + juce::String(dst) + "\n";
	}

	return CommandFuncResult{ true, result };
}

AUDIOCORE_FUNC(addMixerTrackMidiOutput) {
	juce::String result;

	if (auto graph = audioCore->getGraph()) {
		int src = luaL_checkinteger(L, 1);
		graph->setMIDITrk2OConnection(src);

		result += juce::String(src) + " - " + "[MIDI Output]" + "\n";
	}

	return CommandFuncResult{ true, result };
}

#include "../source/CloneableSourceManagerTemplates.h"

AUDIOCORE_FUNC(addAudioSource) {
	juce::String result;

	if (lua_isstring(L, 1) && !lua_isnumber(L, 1)) {
		juce::String sourcePath = juce::String::fromUTF8(lua_tostring(L, 1));
		bool copyFile = lua_toboolean(L, 2);

		CloneableSourceManager::getInstance()
			->createNewSourceThenLoadAsync<CloneableAudioSource>(sourcePath, copyFile);
	}
	else {
		double sampleRate = luaL_checknumber(L, 1);
		int channelNum = luaL_checkinteger(L, 2);
		double length = luaL_checknumber(L, 3);

		CloneableSourceManager::getInstance()
			->createNewSource<CloneableAudioSource>(
				sampleRate, channelNum, length);
	}

	result += "Total Source Num: " + juce::String(CloneableSourceManager::getInstance()->getSourceNum()) + "\n";

	return CommandFuncResult{ true, result };
}

AUDIOCORE_FUNC(addMIDISource) {
	juce::String result;

	if (lua_isstring(L, 1) && !lua_isnumber(L, 1)) {
		juce::String sourcePath = juce::String::fromUTF8(lua_tostring(L, 1));
		bool copyFile = lua_toboolean(L, 2);

		CloneableSourceManager::getInstance()
			->createNewSourceThenLoadAsync<CloneableMIDISource>(sourcePath, copyFile);
	}
	else {
		CloneableSourceManager::getInstance()
			->createNewSource<CloneableMIDISource>();
	}

	result += "Total Source Num: " + juce::String(CloneableSourceManager::getInstance()->getSourceNum()) + "\n";

	return CommandFuncResult{ true, result };
}

AUDIOCORE_FUNC(addSynthSource) {
	juce::String result;

	if (lua_isstring(L, 1) && !lua_isnumber(L, 1)) {
		juce::String sourcePath = juce::String::fromUTF8(lua_tostring(L, 1));
		bool copyFile = lua_toboolean(L, 2);

		CloneableSourceManager::getInstance()
			->createNewSourceThenLoadAsync<CloneableSynthSource>(sourcePath, copyFile);
	}
	else {
		CloneableSourceManager::getInstance()
			->createNewSource<CloneableSynthSource>();
	}

	result += "Total Source Num: " + juce::String(CloneableSourceManager::getInstance()->getSourceNum()) + "\n";

	return CommandFuncResult{ true, result };
}

AUDIOCORE_FUNC(addSequencerTrack) {
	juce::String result;

	if (auto graph = audioCore->getGraph()) {
		int trackTypeArg = luaL_checkinteger(L, 2);
		juce::AudioChannelSet trackType = utils::getChannelSet(static_cast<utils::TrackType>(trackTypeArg));

		graph->insertSource(luaL_checkinteger(L, 1), trackType);

		result += "Add Sequencer Track: [" + juce::String(trackTypeArg) + "]" + trackType.getDescription() + "\n";
		result += "Total Sequencer Track Num: " + juce::String(graph->getSourceNum()) + "\n";
	}

	return CommandFuncResult{ true, result };
}

AUDIOCORE_FUNC(addSequencerTrackMidiOutputToMixer) {
	juce::String result;

	if (auto graph = audioCore->getGraph()) {
		int src = luaL_checkinteger(L, 1);
		int dst = luaL_checkinteger(L, 2);
		graph->setMIDISrc2TrkConnection(src, dst);

		result += juce::String(src) + " - " + juce::String(dst) + "\n";
	}

	return CommandFuncResult{ true, result };
}

AUDIOCORE_FUNC(addSequencerTrackMidiOutputToInstr) {
	juce::String result;

	if (auto graph = audioCore->getGraph()) {
		int src = luaL_checkinteger(L, 1);
		int dst = luaL_checkinteger(L, 2);
		graph->setMIDISrc2InstrConnection(src, dst);

		result += juce::String(src) + " - " + juce::String(dst) + "\n";
	}

	return CommandFuncResult{ true, result };
}

AUDIOCORE_FUNC(addSequencerTrackOutput) {
	juce::String result;

	if (auto graph = audioCore->getGraph()) {
		int src = luaL_checkinteger(L, 1);
		int srcc = luaL_checkinteger(L, 2);
		int dst = luaL_checkinteger(L, 3);
		int dstc = luaL_checkinteger(L, 4);
		graph->setAudioSrc2TrkConnection(src, dst, srcc, dstc);

		result += juce::String(src) + ", " + juce::String(srcc) + " - " + juce::String(dst) + ", " + juce::String(dstc) + "\n";
	}

	return CommandFuncResult{ true, result };
}

AUDIOCORE_FUNC(addSequencerSourceInstance) {
	juce::String result;

	int trackIndex = luaL_checkinteger(L, 1);
	int srcIndex = luaL_checkinteger(L, 2);
	double startTime = luaL_checknumber(L, 3);
	double endTime = luaL_checknumber(L, 4);
	double offset = luaL_checknumber(L, 5);

	AudioCore::getInstance()->addSequencerSourceInstance(trackIndex, srcIndex,
		startTime, endTime, offset);

	result += "Add Sequencer Source Instance [" + juce::String(trackIndex) + "] : [" + juce::String(srcIndex) + "]\n";
	result += "Total Sequencer Source Instance: " + juce::String(AudioCore::getInstance()->getSequencerSourceInstanceNum(trackIndex)) + "\n";

	return CommandFuncResult{ true, result };
}

AUDIOCORE_FUNC(addRecorderSourceInstance) {
	juce::String result;

	int srcIndex = luaL_checkinteger(L, 1);
	double offset = luaL_checknumber(L, 2);

	AudioCore::getInstance()->addRecorderSourceInstance(srcIndex, offset);

	result += "Add Recorder Source Instance [" + juce::String(srcIndex) + "]\n";
	result += "Total Recorder Source Instance: " + juce::String(AudioCore::getInstance()->getRecorderSourceInstanceNum()) + "\n";

	return CommandFuncResult{ true, result };
}

void regCommandAdd(lua_State* L) {
	LUA_ADD_AUDIOCORE_FUNC_DEFAULT_NAME(L, addPluginBlackList);
	LUA_ADD_AUDIOCORE_FUNC_DEFAULT_NAME(L, addPluginSearchPath);
	LUA_ADD_AUDIOCORE_FUNC_DEFAULT_NAME(L, addMixerTrack);
	LUA_ADD_AUDIOCORE_FUNC_DEFAULT_NAME(L, addMixerTrackSend);
	LUA_ADD_AUDIOCORE_FUNC_DEFAULT_NAME(L, addMixerTrackInputFromDevice);
	LUA_ADD_AUDIOCORE_FUNC_DEFAULT_NAME(L, addMixerTrackOutput);
	LUA_ADD_AUDIOCORE_FUNC_DEFAULT_NAME(L, addEffect);
	LUA_ADD_AUDIOCORE_FUNC_DEFAULT_NAME(L, addEffectAdditionalInput);
	LUA_ADD_AUDIOCORE_FUNC_DEFAULT_NAME(L, addInstr);
	LUA_ADD_AUDIOCORE_FUNC_DEFAULT_NAME(L, addInstrOutput);
	LUA_ADD_AUDIOCORE_FUNC_DEFAULT_NAME(L, addInstrMidiInput);
	LUA_ADD_AUDIOCORE_FUNC_DEFAULT_NAME(L, addMixerTrackMidiInput);
	LUA_ADD_AUDIOCORE_FUNC_DEFAULT_NAME(L, addMixerTrackMidiOutput);
	LUA_ADD_AUDIOCORE_FUNC_DEFAULT_NAME(L, addAudioSource);
	LUA_ADD_AUDIOCORE_FUNC_DEFAULT_NAME(L, addMIDISource);
	LUA_ADD_AUDIOCORE_FUNC_DEFAULT_NAME(L, addSynthSource);
	LUA_ADD_AUDIOCORE_FUNC_DEFAULT_NAME(L, addSequencerTrack);
	LUA_ADD_AUDIOCORE_FUNC_DEFAULT_NAME(L, addSequencerTrackMidiOutputToMixer);
	LUA_ADD_AUDIOCORE_FUNC_DEFAULT_NAME(L, addSequencerTrackMidiOutputToInstr);
	LUA_ADD_AUDIOCORE_FUNC_DEFAULT_NAME(L, addSequencerTrackOutput);
	LUA_ADD_AUDIOCORE_FUNC_DEFAULT_NAME(L, addSequencerSourceInstance);
	LUA_ADD_AUDIOCORE_FUNC_DEFAULT_NAME(L, addRecorderSourceInstance);
}
