﻿#include "crash.h"

#include "audioCore/AC_API.h"
#include "ui/Utils.h"
#include <iostream>
#include <filesystem>

#if JUCE_WINDOWS
#include <Windows.h>
#endif //JUCE_WINDOWS

static std::string hostPath;
static UICrashCallback uiCrashCallback;

std::string getLocalTime() {
	time_t nowTime;
	time(&nowTime);

	std::stringstream timeStr;
	timeStr << std::put_time(std::localtime(&nowTime), "%F %T");
	return timeStr.str();
}

void showWarningBox(const std::string& title, const std::string& message) {
#if JUCE_WINDOWS
	MessageBox(NULL, message.c_str(), title.c_str(), MB_ICONWARNING | MB_OK);

#else
	juce::AlertWindow::showOkCancelBox(juce::MessageBoxIconType::WarningIcon, title, message);

#endif
}

void applicationCrashHandler(void* info) {
	/** Get Time */
	std::string time = getLocalTime();
	std::replace(time.begin(), time.end(), ' ', '_');
	std::replace(time.begin(), time.end(), '-', '_');
	std::replace(time.begin(), time.end(), ':', '_');

	/** Get File Path */
	std::filesystem::path projPath = std::filesystem::path{ ::hostPath };
	std::filesystem::path projHLPath = projPath / (time + ".vshl.dmp");
	std::filesystem::path projMLPath = projPath / (time + ".vsml.dmp");
	std::filesystem::path projLLPath = projPath / (time + ".vsll.dmp");

	/** Results */
	std::array<int, 3> results = { 0 };
	
	/** Low Layer Recovery */
	std::string projLLPathStr = projLLPath.string();
	results[2] = audioCoreLowLayerDataRecovery(projLLPathStr.c_str(), info);

	/** Mid Layer Recovery */
	std::string projMLPathStr = projMLPath.string();
	results[1] = audioCoreMidLayerDataRecovery(projMLPathStr.c_str());

	/** High Layer Recovery */
	std::string projHLPathStr = projHLPath.string();
	results[0] = audioCoreHighLayerDataRecovery(projHLPathStr.c_str());

	/** =======================Unsafe========================= */

	/** Close UI */
	if (::uiCrashCallback) {
		uiCrashCallback();
	}

	/** Result */
	std::string message = "A fatal error has occurred in the application and the program will abort.\n"
		"The error triggered an emergency backup of the project data, and here is the result of that backup:\n"
		+ std::string{ (results[2] == 0) ? "Saved" : "Error" } + " : " + std::string{ projLLPathStr } + "\n"
		+ std::string{ (results[1] == 0) ? "Saved" : "Error" } + " : " + std::string{ projMLPathStr } + "\n"
		+ std::string{ (results[0] == 0) ? "Saved" : "Error" } + " : " + std::string{ projHLPathStr } + "\n";
	showWarningBox("VocalShaper Crash Handler", message);
}

const juce::Array<juce::File> getAllDumpFiles() {
	auto appDir = utils::getAppRootDir();
	return appDir.findChildFiles(juce::File::findFiles, false,
		"*.vsll.dmp;*.vsml.dmp;*.vshl.dmp", juce::File::FollowSymlinks::no);
}

void initCrashHandler(const juce::String& path) {
	::hostPath = path.toStdString();
}

void setUICrashCallback(const UICrashCallback& callback) {
	::uiCrashCallback = callback;
}
