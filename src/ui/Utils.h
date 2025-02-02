﻿#pragma once

#include <JuceHeader.h>

namespace utils {
	const juce::File getAppExecFile();
	const juce::File getAppRootDir();
	const juce::File getRegExecFile();

	const juce::File getLayoutDir();
	const juce::File getResourceDir();
	const juce::File getFontDir();
	const juce::File getSplashConfigDir();
	const juce::File getThemeDir();
	const juce::File getTransDir();
	const juce::File getDataDir();
	const juce::File getLicenseDir();
	const juce::File getIconDir();

	const juce::File getConfigDir();
	const juce::File getAudioDir();
	const juce::File getEditorDir();
	const juce::File getThemeRootDir(const juce::String& name);
	const juce::File getTransRootDir(const juce::String& name);
	const juce::File getIconClassDir(const juce::String& name);

	const juce::File getConfigFile(const juce::String& name,
		const juce::String& type = ".json");
	const juce::File getEditorDataFile(const juce::String& name);
	const juce::File getKeyMappingFile(
		const juce::String& file = "keyMapping.xml");
	const juce::File getAudioConfigFile(
		const juce::String& file = "audioConfig.xml");
	const juce::File getThemeColorFile(const juce::String& name,
		const juce::String& file = "colors.json");
	const juce::File getFontFile(const juce::String& name,
		const juce::String& type = ".ttf");
	const juce::File getResourceFile(const juce::String& name);
	const juce::File getSplashConfigFile(const juce::String& name,
		const juce::String& type = ".json");
	const juce::File getTransConfigFile(const juce::String& name,
		const juce::String& file = "config.json");
	const juce::File getThemeConfigFile(const juce::String& name,
		const juce::String& file = "config.json");
	const juce::File getLayoutFile(const juce::String& name,
		const juce::String& type = ".json");
	const juce::File getIconFile(
		const juce::String& className, const juce::String& name,
		const juce::String& type = ".svg");

	const juce::File getPluginBlackListFile(
		const juce::String& file = "blackPlugins.txt");
	const juce::File getPluginListFile(
		const juce::String& file = "plugins.xml");
	const juce::File getPluginSearchPathFile(
		const juce::String& file = "pluginPaths.txt");
	const juce::File getPluginDeadTempDir(
		const juce::String& path = "./deadPlugins/");
	const juce::File getPluginPresetDir();

	const juce::URL getHelpPage(
		const juce::String& version, const juce::String& branch,
		const juce::String& language);
	const juce::URL getUpdatePage(const juce::String& version, const juce::String& branch);
	const juce::URL getBilibiliPage();
	const juce::URL getGithubPage();
	const juce::URL getWebsitePage();

	const juce::Rectangle<int> getScreenSize(const juce::Component* comp);
	double getScreenScale(const juce::Component* comp);

	const juce::StringArray parseCommand(const juce::String& command);

	bool regProjectFileInSystem();
	bool unregProjectFileFromSystem();

	int matchKMP(const juce::String& line, const juce::String& word);
	const juce::StringArray searchKMP(const juce::StringArray& list, const juce::String& word);
	
	bool fuzzyMatch(const juce::String& line, const juce::StringArray& keyWords, bool ignoreCase);
	const juce::StringArray searchFuzzy(const juce::StringArray& list, const juce::String& word, bool ignoreCase);

	bool saveXml(const juce::File& file, juce::XmlElement* xml);
	std::unique_ptr<juce::XmlElement> readXml(const juce::File& file);

	void panic();

	float logRMS(float rms);
	float getLogLevelPercent(float logLevel, float total = 60.f);
	float getLogFromPercent(float percent, float total = 60.f);

	class TextIntegerFilter : public juce::TextEditor::InputFilter {
	public:
		TextIntegerFilter() = delete;
		TextIntegerFilter(int minLimit, int maxLimit);

		juce::String filterNewText(
			juce::TextEditor& editor, const juce::String& newInput) override;

	private:
		const int minLimit, maxLimit;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextIntegerFilter)
	};

	class TextDoubleFilter : public juce::TextEditor::InputFilter {
	public:
		TextDoubleFilter() = delete;
		TextDoubleFilter(double minLimit, double maxLimit, int numberOfDecimalPlaces);

		juce::String filterNewText(
			juce::TextEditor& editor, const juce::String& newInput) override;

	private:
		const double minLimit, maxLimit;
		const int numberOfDecimalPlaces;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextDoubleFilter)
	};

	class TextColorRGBFilter final : public juce::TextEditor::InputFilter {
	public:
		TextColorRGBFilter() = default;

	private:
		juce::String filterNewText(juce::TextEditor& e, const juce::String& newInput) override;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextColorRGBFilter)
	};

	class TextColorHexFilter final : public juce::TextEditor::InputFilter {
	public:
		TextColorHexFilter() = default;

	private:
		juce::String filterNewText(juce::TextEditor& e, const juce::String& newInput) override;

		JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TextColorHexFilter)
	};

	using PluginGroup = std::tuple<juce::String, juce::Array<juce::PluginDescription>>;
	enum class PluginGroupType {
		Format, Manufacturer, Category
	};
	const juce::Array<PluginGroup> groupPlugin(const juce::Array<juce::PluginDescription>& list,
		PluginGroupType groupType, bool search = false, const juce::String& searchText = "");
	juce::PopupMenu createPluginMenu(const juce::Array<PluginGroup>& list,
		const std::function<void(const juce::PluginDescription&)>& callback);

	using TimeInSeconds = std::tuple<int, int, int, int>;
	using TimeInBeats = std::tuple<int, int, int>;
	const TimeInSeconds splitTime(double seconds);
	const TimeInBeats splitBeat(uint64_t measures, double beats);
	const std::array<uint8_t, 8> createTimeStringBase(const TimeInSeconds& time);
	const std::array<uint8_t, 8> createBeatStringBase(const TimeInBeats& time);
	const juce::String createTimeString(const TimeInSeconds& time);
	const juce::String createBeatString(const TimeInBeats& time);

	float colorLuminance(const juce::Colour& color);
	float colorContrastRatio(float luminance1, float luminance2);
	bool isLightColor(const juce::Colour& color,
		const juce::Colour& textColorLight = juce::Colours::black,
		const juce::Colour& textColorDark = juce::Colours::white);
	juce::Colour chooseTextColor(const juce::Colour& backgroundColor,
		const juce::Colour& textColorLight = juce::Colours::black,
		const juce::Colour& textColorDark = juce::Colours::white);
	using IntSection = std::pair<int, int>;
	using IntSectionList = juce::Array<IntSection>;
	const IntSectionList getUnionSections(const IntSectionList& source);

	float bezierInterpolate(float p0, float p1, float p2, float p3, float t);
	juce::Colour bezierInterpolateColor(
		const juce::Colour& color1, const juce::Colour& control1,
		const juce::Colour& control2, const juce::Colour& color2, float t);
	const juce::Array<juce::Colour> generateBezierColorGradient(
		const juce::Colour& baseColor, const juce::Colour& targetColor, int num,
		float control1 = 0.3f, float control2 = 0.7f);
}
