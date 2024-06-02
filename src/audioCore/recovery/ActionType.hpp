﻿#pragma once

enum class ActionType : unsigned int {
	ActionAddMixerTrack = 0x0001,
	ActionAddMixerTrackSend,
	ActionAddMixerTrackInputFromDevice,
	ActionAddMixerTrackOutput,
	ActionAddEffect,
	ActionAddInstr,
	ActionAddMixerTrackMidiInput,
	ActionAddMixerTrackMidiOutput,
	ActionAddSequencerTrack,
	ActionAddSequencerTrackMidiOutputToMixer,
	ActionAddSequencerTrackOutput,
	ActionAddMixerTrackSideChainBus,
	ActionAddSequencerBlock,
	ActionAddTempoTempo,
	ActionAddTempoBeat,

	ActionSave = 0x0101,
	ActionSplitSequencerBlock,

	ActionRemoveMixerTrack = 0x0201,
	ActionRemoveMixerTrackSend,
	ActionRemoveMixerTrackInputFromDevice,
	ActionRemoveMixerTrackOutput,
	ActionRemoveEffect,
	ActionRemoveInstr,
	ActionRemoveInstrParamCCConnection,
	ActionRemoveEffectParamCCConnection,
	ActionRemoveMixerTrackMidiInput,
	ActionRemoveMixerTrackMidiOutput,
	ActionRemoveSequencerTrack,
	ActionRemoveSequencerTrackMidiOutputToMixer,
	ActionRemoveSequencerTrackOutput,
	ActionRemoveMixerTrackSideChainBus,
	ActionRemoveSequencerBlock,
	ActionRemoveTempo,

	ActionSetMixerTrackGain = 0x0301,
	ActionSetMixerTrackPan,
	ActionSetMixerTrackSlider,
	ActionSetEffectBypass,
	ActionSetInstrBypass,
	ActionSetInstrMidiChannel,
	ActionSetEffectMidiChannel,
	ActionSetInstrParamValue,
	ActionSetEffectParamValue,
	ActionSetInstrParamConnectToCC,
	ActionSetEffectParamConnectToCC,
	ActionSetInstrMidiCCIntercept,
	ActionSetEffectMidiCCIntercept,
	ActionSetSequencerTrackBypass,
	ActionSetSourceSynthesizer,
	ActionSetSourceName,
	ActionSetSourceSynthDst,
	ActionSetEffectBypassByPtr,
	ActionSetInstrBypassByPtr,
	ActionSetInstrMidiChannelByPtr,
	ActionSetEffectMidiChannelByPtr,
	ActionSetInstrMidiCCInterceptByPtr,
	ActionSetEffectMidiCCInterceptByPtr,
	ActionSetInstrMidiOutputByPtr,
	ActionSetEffectMidiOutputByPtr,
	ActionSetInstrParamConnectToCCByPtr,
	ActionSetEffectParamConnectToCCByPtr,
	ActionSetMixerTrackName,
	ActionSetMixerTrackColor,
	ActionSetMixerTrackMute,
	ActionSetEffectIndex,
	ActionSetSequencerTrackRecording,
	ActionSetInstrOffline,
	ActionSetTempoTime,
	ActionSetTempoTempo,
	ActionSetTempoBeat,
	ActionSetSequencerTrackName,
	ActionSetSequencerTrackColor,
	ActionSetSequencerTrackMute,
	ActionSetEffect,
	ActionSetSequencerMIDITrack
};
