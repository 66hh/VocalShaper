﻿#include <JuceHeader.h>
#include "audioCore/AC_API.h"
#include <FlowUI.h>
#include <google/protobuf/message_lite.h>
#include "ui/misc/ColorMap.h"
#include "ui/misc/InitTaskList.h"
#include "ui/misc/RCManager.h"
#include "ui/misc/MainThreadPool.h"
#include "ui/misc/ConfigManager.h"
#include "ui/misc/SysStatus.h"
#include "ui/misc/CoreCallbacks.h"
#include "ui/misc/PluginEditorHub.h"
#include "ui/misc/LevelMeterHub.h"
#include "ui/debug/AudioDebuggerComponent.h"
#include "ui/debug/MidiDebuggerComponent.h"
#include "ui/lookAndFeel/LookAndFeelFactory.h"
#include "ui/component/toolbar/ToolBar.h"
#include "ui/component/misc/Splash.h"
#include "ui/component/CompManager.h"
#include "ui/component/plugin/PluginView.h"
#include "ui/component/mixer/MixerView.h"
#include "ui/component/sequencer/SeqView.h"
#include "ui/component/editor/SourceEditor.h"
#include "ui/menuAndCommand/CommandManager.h"
#include "ui/menuAndCommand/CoreCommandTarget.h"
#include "ui/menuAndCommand/GUICommandTarget.h"
#include "ui/dataModel/toolbar/MessageModel.h"
#include "ui/Utils.h"
#include "crash.h"

class MainApplication : public juce::JUCEApplication {
private:
	std::unique_ptr<Splash> splash = nullptr;

	void initCrashHandler() {
		InitTaskList::getInstance()->add(
			[splash = Splash::SafePointer<Splash>(this->splash.get())] {
				if (splash) { splash->showMessage("Init Crash Handler..."); }
			}
		);
		InitTaskList::getInstance()->add(
			[] {
				/** Init Path */
				::initCrashHandler(utils::getAppRootDir().getFullPathName());

				/** ShutDown FlowUI And Close All Plugin Editors */
				::setUICrashCallback([] {
					flowUI::FlowWindowHub::shutdown();
					PluginEditorHub::getInstance()->closeAll();
					});
			}
		);
	};

	void loadConfig() {
		InitTaskList::getInstance()->add(
			[splash = Splash::SafePointer<Splash>(this->splash.get())] {
				if (splash) { splash->showMessage("Load Configs..."); }
			}
		);
		InitTaskList::getInstance()->add(
			[] {
				ConfigManager::getInstance()->loadConfigs();
			}
		);
	};

	void setAudioConfig() {
		InitTaskList::getInstance()->add(
			[splash = Splash::SafePointer<Splash>(this->splash.get())] {
				if (splash) { splash->showMessage("Set Audio Configs..."); }
			}
		);
		InitTaskList::getInstance()->add(
			[] {
				/** Audio Device */
				auto kmFile = utils::getAudioConfigFile();
				if (auto kmData = utils::readXml(kmFile)) {
					quickAPI::setAudioDeviceInitState(std::move(kmData));
				}

				/** Plugin Path */
				quickAPI::setPluginSearchPathListFilePath(utils::getPluginSearchPathFile().getFullPathName());
				quickAPI::setPluginListTemporaryFilePath(utils::getPluginListFile().getFullPathName());
				quickAPI::setPluginBlackListFilePath(utils::getPluginBlackListFile().getFullPathName());
				quickAPI::setDeadPluginListPath(utils::getPluginDeadTempDir().getFullPathName());

				/** Functions */
				auto& funcVar = ConfigManager::getInstance()->get("function");
				quickAPI::setReturnToStartOnStop(funcVar["return-on-stop"]);
				quickAPI::setAnonymousMode(funcVar["anonymous-mode"]);
				quickAPI::setSIMDLevel(funcVar["simd-speed-up"]);

				/** Output */
				auto formats = quickAPI::getAudioFormatsSupported(true);
				for (auto& s : formats) {
					juce::String format = s.trimCharactersAtStart("*");
					auto& outputVar =
						ConfigManager::getInstance()->get("output" + format);

					quickAPI::setFormatBitsPerSample(
						format, outputVar["bit-depth"].toString().getIntValue());
					quickAPI::setFormatQualityOptionIndex(
						format, outputVar["quality"]);

					auto& metaVar = outputVar["meta"];
					if (auto metaObject = metaVar.getDynamicObject()) {
						auto& metaSet = metaObject->getProperties();
						juce::StringPairArray metaList;
						for (auto& i : metaSet) {
							metaList.set(i.name.toString(), i.value.toString());
						}
						quickAPI::setFormatMetaData(format, metaList);
					}
				}
			}
		);
	};

	void loadAudioPlugins() {
		InitTaskList::getInstance()->add(
			[splash = Splash::SafePointer<Splash>(this->splash.get())] {
				if (splash) { splash->showMessage("Load Audio Plugins..."); }
			}
		);
		InitTaskList::getInstance()->add(
			[] {
				[[maybe_unused]] auto result = quickAPI::getPluginList();
			}
		);
	};

	void loadTheme() {
		InitTaskList::getInstance()->add(
			[splash = Splash::SafePointer<Splash>(this->splash.get())] {
				if (splash) { splash->showMessage("Load Theme Colors..."); }
			}
		);
		InitTaskList::getInstance()->add(
			[] {
				/** Get Config */
				auto& conf = ConfigManager::getInstance()->get("startup");
				juce::String themeName = conf["theme"].toString();

				/** Get Color Map */
				juce::File colorMapFile = utils::getThemeColorFile(themeName);
				auto colorMapData = juce::JSON::parse(colorMapFile);
				if (!colorMapData.isObject()) {
					juce::AlertWindow::showMessageBox(
						juce::MessageBoxIconType::WarningIcon, "VocalShaper Fatal Error",
						"Can't load theme!");
					juce::JUCEApplication::quit();
				}

				/** Set Global Color Map */
				if (auto pColorObj = colorMapData.getDynamicObject()) {
					auto& colorSet = pColorObj->getProperties();
					for (auto& i : colorSet) {
						ColorMap::getInstance()->set(
							i.name.toString(), ColorMap::fromString(i.value.toString()));
					}
				}
			}
		);
	};

	void initLookAndFeel() {
		InitTaskList::getInstance()->add(
			[splash = Splash::SafePointer<Splash>(this->splash.get())] {
				if (splash) { splash->showMessage("Init Default LookAndFeel..."); }
			}
		);
		InitTaskList::getInstance()->add(
			[] {
				LookAndFeelFactory::getInstance()->initialise();
			}
		);
	};

	void setFlowUIIcon() {
		InitTaskList::getInstance()->add(
			[splash = Splash::SafePointer<Splash>(this->splash.get())] {
				if (splash) { splash->showMessage("Set FlowUI Button Icons..."); }
			}
		);
		InitTaskList::getInstance()->add(
			[] {
				flowUI::FlowStyle::setButtonLeftIcon(
					utils::getIconFile("Design", "layout-left-2-line").getFullPathName());
				flowUI::FlowStyle::setButtonRightIcon(
					utils::getIconFile("Design", "layout-right-2-line").getFullPathName());
				flowUI::FlowStyle::setButtonTopIcon(
					utils::getIconFile("Design", "layout-top-2-line").getFullPathName());
				flowUI::FlowStyle::setButtonBottomIcon(
					utils::getIconFile("Design", "layout-bottom-2-line").getFullPathName());
				flowUI::FlowStyle::setButtonAdsorbCenterIcon(
					utils::getIconFile("Editor", "link").getFullPathName());
				flowUI::FlowStyle::setButtonAdsorbLeftIcon(
					utils::getIconFile("Design", "layout-left-2-line").getFullPathName());
				flowUI::FlowStyle::setButtonAdsorbRightIcon(
					utils::getIconFile("Design", "layout-right-2-line").getFullPathName());
				flowUI::FlowStyle::setButtonAdsorbTopIcon(
					utils::getIconFile("Design", "layout-top-2-line").getFullPathName());
				flowUI::FlowStyle::setButtonAdsorbBottomIcon(
					utils::getIconFile("Design", "layout-bottom-2-line").getFullPathName());
			}
		);
	};

	void configFlowUI() {
		InitTaskList::getInstance()->add(
			[splash = Splash::SafePointer<Splash>(this->splash.get())] {
				if (splash) { splash->showMessage("Config FlowUI Window..."); }
			}
		);
		InitTaskList::getInstance()->add(
			[] {
				auto& funcVar = ConfigManager::getInstance()->get("function");

				flowUI::FlowWindowHub::setTitle(utils::getAudioPlatformName());
				flowUI::FlowWindowHub::setIcon(utils::getResourceFile("logo.png").getFullPathName());
				flowUI::FlowWindowHub::setOpenGL(!((bool)(funcVar["cpu-painting"])));
			}
		);
	};

	void configPluginEditor() {
		InitTaskList::getInstance()->add(
			[splash = Splash::SafePointer<Splash>(this->splash.get())] {
				if (splash) { splash->showMessage("Config Plugin Editor..."); }
			}
		);
		InitTaskList::getInstance()->add(
			[] {
				auto& funcVar = ConfigManager::getInstance()->get("function");

				PluginEditorHub::getInstance()->setIcon(utils::getResourceFile("logo.png").getFullPathName());
				PluginEditorHub::getInstance()->setOpenGL(!((bool)(funcVar["cpu-painting"])));
			}
		);
	};

	void loadUITranslate() {
		InitTaskList::getInstance()->add(
			[splash = Splash::SafePointer<Splash>(this->splash.get())] {
				if (splash) { splash->showMessage("Load UI Translations..."); }
			}
		);
		InitTaskList::getInstance()->add(
			[] {
				/** Get Config */
				auto& conf = ConfigManager::getInstance()->get("startup");
				juce::String transName = conf["language"].toString();

				/** Load Translates */
				juce::File transDir = utils::getTransRootDir(transName);
				auto transList = transDir.findChildFiles(
					juce::File::findFiles, true, "*.txt", juce::File::FollowSymlinks::noCycles);

				if (transList.size() > 0) {
					auto trans = new juce::LocalisedStrings(transList.getReference(0), true);

					for (int i = 1; i < transList.size(); i++) {
						trans->addStrings(juce::LocalisedStrings(transList.getReference(i), true));
					}

					juce::LocalisedStrings::setCurrentMappings(trans);
				}
			}
		);
	};

	void setUIFont() {
		InitTaskList::getInstance()->add(
			[splash = Splash::SafePointer<Splash>(this->splash.get())] {
				if (splash) { splash->showMessage("Set UI Fonts..."); }
			}
		);
		InitTaskList::getInstance()->add(
			[] {
				/** Get Config */
				auto& conf = ConfigManager::getInstance()->get("startup");
				juce::String fontName = conf["font"].toString();

				/** Load Font */
				juce::File fontFile = utils::getFontFile(fontName);

				auto fontSize = fontFile.getSize();
				auto ptrFontData = std::unique_ptr<char[]>(new char[fontSize]);

				auto fontStream = fontFile.createInputStream();
				fontStream->read(ptrFontData.get(), fontSize);

				auto ptrTypeface = juce::Typeface::createSystemTypefaceFor(ptrFontData.get(), fontSize);
				LookAndFeelFactory::getInstance()->setDefaultSansSerifTypeface(ptrTypeface);
			}
		);
	};

	void createComponents() {
		InitTaskList::getInstance()->add(
			[splash = Splash::SafePointer<Splash>(this->splash.get())] {
				if (splash) { splash->showMessage("Create Components..."); }
			}
		);
		InitTaskList::getInstance()->add(
			[] {
				CompManager::getInstance()->set(CompManager::CompType::StartMenu,
					std::make_unique<flowUI::FlowComponent>(TRANS("Start Menu")));
				CompManager::getInstance()->set(CompManager::CompType::ToolBar,
					std::make_unique<ToolBar>());
				CompManager::getInstance()->set(CompManager::CompType::PluginView,
					std::make_unique<PluginView>());
				CompManager::getInstance()->set(CompManager::CompType::TrackView,
					std::make_unique<SeqView>());
				CompManager::getInstance()->set(CompManager::CompType::MixerView,
					std::make_unique<MixerView>());
				CompManager::getInstance()->set(CompManager::CompType::ResourceEditView,
					std::make_unique<SourceEditor>());
				CompManager::getInstance()->set(CompManager::CompType::AudioDebugger,
					std::make_unique<AudioDebuggerComponent>());
				CompManager::getInstance()->set(CompManager::CompType::MidiDebugger,
					std::make_unique<MidiDebuggerComponent>());
			}
		);
	};

	void initCommands() {
		InitTaskList::getInstance()->add(
			[splash = Splash::SafePointer<Splash>(this->splash.get())] {
				if (splash) { splash->showMessage("Init Command Manager..."); }
			}
		);
		InitTaskList::getInstance()->add(
			[] {
				CommandManager::getInstance()->init();
				flowUI::FlowWindowHub::addKeyListener(CommandManager::getInstance()->getKeyMappings());
			}
		);
	};

	void loadKeyMapping() {
		InitTaskList::getInstance()->add(
			[splash = Splash::SafePointer<Splash>(this->splash.get())] {
				if (splash) { splash->showMessage("Load Key Mapping..."); }
			}
		);
		InitTaskList::getInstance()->add(
			[] {
				if (auto keyMapping = CommandManager::getInstance()->getKeyMappings()) {
					auto kmFile = utils::getKeyMappingFile();
					if (auto kmData = utils::readXml(kmFile)) {
						keyMapping->restoreFromXml(*(kmData.get()));
					}
				}
				CommandManager::getInstance()->startListening();
			}
		);
	};

	void initCoreHooks() {
		InitTaskList::getInstance()->add(
			[splash = Splash::SafePointer<Splash>(this->splash.get())] {
				if (splash) { splash->showMessage("Init Core Hooks..."); }
			}
		);
		InitTaskList::getInstance()->add(
			[] {
				flowUI::FlowWindowHub::setAppExitHook([]() -> bool {
					if (quickAPI::checkProjectSaved() && quickAPI::checkSourcesSaved()) {
						return true;
					}

					return juce::AlertWindow::showOkCancelBox(
						juce::MessageBoxIconType::QuestionIcon, TRANS("Close Editor"),
						TRANS("Discard unsaved changes and exit?"));
					});
			}
		);
	};

	void addCoreCallback() {
		InitTaskList::getInstance()->add(
			[splash = Splash::SafePointer<Splash>(this->splash.get())] {
				if (splash) { splash->showMessage("Register Core Callbacks..."); }
			}
		);
		InitTaskList::getInstance()->add(
			[] {
				CoreCallbacks::getInstance()->addError(
					[](const juce::String& title, const juce::String& mes) {
						MessageModel::getInstance()->addNow("AudioCore: " + mes, MessageModel::Callback{});
						juce::AlertWindow::showMessageBox(
							juce::MessageBoxIconType::WarningIcon, title, mes);
					}
				);
				CoreCallbacks::getInstance()->addErrorMes(
					[](const juce::String& mes) {
						MessageModel::getInstance()->addNow("AudioCore: " + mes, MessageModel::Callback{});
					}
				);
			}
		);
	};

	void addAudioDeviceCallback() {
		InitTaskList::getInstance()->add(
			[splash = Splash::SafePointer<Splash>(this->splash.get())] {
				if (splash) { splash->showMessage("Register Audio Device Callbacks..."); }
			}
		);
		InitTaskList::getInstance()->add(
			[] {
				auto audioDeviceCallback = [](juce::XmlElement* data) {
					auto file = utils::getAudioConfigFile();
					utils::saveXml(file, data);
					};
				quickAPI::setAudioDeviceCallback(audioDeviceCallback);
			}
		);
	};

	void autoLayout() {
		InitTaskList::getInstance()->add(
			[splash = Splash::SafePointer<Splash>(this->splash.get())] {
				if (splash) { splash->showMessage("Auto Layout Components..."); }
			}
		);
		InitTaskList::getInstance()->add(
			[] {
				/** Get Config */
				auto& conf = ConfigManager::getInstance()->get("startup");
				juce::String layoutName = conf["layout"].toString();

				/** Layout */
				CompManager::getInstance()->autoLayout(
					utils::getLayoutFile(layoutName).getFullPathName());
			}
		);
	};

	void prepareMainWindow() {
		InitTaskList::getInstance()->add(
			[splash = Splash::SafePointer<Splash>(this->splash.get())] {
				if (splash) { splash->showMessage("Set Main Window Size..."); }
			}
		);
		InitTaskList::getInstance()->add(
			[] {
				CompManager::getInstance()->maxMainWindow();
			}
		);
	};

	void clearCrashDump() {
		InitTaskList::getInstance()->add(
			[splash = Splash::SafePointer<Splash>(this->splash.get())] {
				if (splash) { splash->showMessage("Find Crash Dump..."); }
			}
		);
		InitTaskList::getInstance()->add(
			[] {
				auto dumpList = ::getAllDumpFiles();
				if (!dumpList.isEmpty()) {
					juce::String mes = TRANS("Found the following crash dump files. These files take up {DMPSIZE} of storage space, should they be deleted to save disk space?") + "\n";
					size_t dumpSize = 0;
					for (auto& i : dumpList) {
						mes += (i.getFullPathName() + "\n");
						dumpSize += i.getSize();
					}
					mes = mes.replace("{DMPSIZE}", juce::File::descriptionOfSizeInBytes(dumpSize));

					if (juce::AlertWindow::showOkCancelBox(
						juce::MessageBoxIconType::QuestionIcon, TRANS("Crash Dump File"), mes)) {
						for (auto& i : dumpList) {
							i.deleteFile();
						}
					}
				}
			}
		);
	};

	void hideSplash() {
		InitTaskList::getInstance()->add(
			[splash = Splash::SafePointer<Splash>(this->splash.get())] {
				if (splash) {
					splash->showMessage("Ready. Special thanks to Warsic Music Club.");
					splash->ready();
				}
			}
		);
	};

	void loadProject(const juce::String& commandLineParameters) {
		InitTaskList::getInstance()->add(
			[commandLineParameters] {
				if (commandLineParameters.isNotEmpty()) {
					auto params = utils::parseCommand(commandLineParameters);

					/** Remove Quote */
					for (auto& s : params) {
						s = s.removeCharacters("\"");
					}
					/** Check First Arg And Remove Execute Path */
					{
						juce::File firstArgFile(params[0]);
						juce::File execFile = utils::getAppExecFile();
						if (firstArgFile == execFile) {
							params.remove(0);
						}
					}

					/** Load Project */
					if (params.size() >= 1) {
						juce::File projFile = utils::getAppRootDir().getChildFile(params[0]);
						juce::StringArray extensions;
						auto formatsSupport = quickAPI::getProjectFormatsSupported(false);
						for (auto& s : formatsSupport) {
							extensions.add(s.trimCharactersAtStart("*"));
						}

						if (projFile.existsAsFile() && extensions.contains(projFile.getFileExtension())) {
							CoreCommandTarget::getInstance()->systemRequestOpen(params[0]);
						}
					}
				}
			}
		);
	};

	void setCrashHandler() {
		InitTaskList::getInstance()->add(
			[] {
				juce::SystemStats::setApplicationCrashHandler(::applicationCrashHandler);
			}
		);
	};

public:
	const juce::String getApplicationName() override {
		return utils::getAudioPlatformName(); };
	const juce::String getApplicationVersion() override {
		return utils::getAudioPlatformVersionString(); };
	bool moreThanOneInstanceAllowed() override { return false; };

	void initialise(const juce::String& commandLineParameters) override {
		/** Show Splash */
		this->splash = std::make_unique<Splash>();
		this->splash->setVisible(true);

		/** Init Crash Handler */
		this->initCrashHandler();

		/** Load Config */
		this->loadConfig();

		/** Set Audio Config */
		this->setAudioConfig();

		/** Load Plugin List */
		this->loadAudioPlugins();
		
		/** Load Theme Colors */
		this->loadTheme();

		/** Init Default LookAndFeel */
		this->initLookAndFeel();

		/** Set FlowUI Button Icon */
		this->setFlowUIIcon();

		/** Config Flow Window */
		this->configFlowUI();

		/** Config Plugin Editor */
		this->configPluginEditor();

		/** Load UI Translate */
		this->loadUITranslate();

		/** Set UI Font */
		this->setUIFont();

		/** Create Components */
		this->createComponents();

		/** Init Commands */
		this->initCommands();

		/** Load Key Mapping */
		this->loadKeyMapping();

		/** Init Core Hooks */
		this->initCoreHooks();

		/** Add Core Callback */
		this->addCoreCallback();

		/** Set Audio Device Callback */
		this->addAudioDeviceCallback();

		/** Auto Layout */
		this->autoLayout();

		/** Set Main Window Size */
		this->prepareMainWindow();

		/** Clear Dump File */
		this->clearCrashDump();

		/** Hide Splash */
		this->hideSplash();

		/** Load Project */
		this->loadProject(commandLineParameters);

		/** Set Crash Handler */
		this->setCrashHandler();

		/** Run Init */
		InitTaskList::getInstance()->runNow();
	};

	void shutdown() override {
		/** ShutDown FlowUI */
		flowUI::FlowWindowHub::shutdown();

		/** Release Components */
		LevelMeterHub::releaseInstance();
		PluginEditorHub::releaseInstance();
		CompManager::releaseInstance();
		LookAndFeelFactory::releaseInstance();

		/** Release Commands */
		CommandManager::releaseInstance();
		CoreCommandTarget::releaseInstance();
		GUICommandTarget::releaseInstance();

		/** ShutDown Threads */
		MainThreadPool::releaseInstance();

		/** ShutDown Backend */
		shutdownAudioCore();
		google::protobuf::ShutdownProtobufLibrary();

		/** ShutDown Sigar */
		SysStatus::releaseInstance();

		/** Release Resources */
		RCManager::releaseInstance();
	};

	void anotherInstanceStarted(const juce::String& commandLine) override {
		if (commandLine.isNotEmpty()) {
			auto params = utils::parseCommand(commandLine);

			/** Remove Quote */
			for (auto& s : params) {
				s = s.removeCharacters("\"");
			}
			/** Check First Arg And Remove Execute Path */
			{
				juce::File firstArgFile(params[0]);
				juce::File execFile = utils::getAppExecFile();
				if (firstArgFile == execFile) {
					params.remove(0);
				}
			}

			/** Load Project */
			if (params.size() >= 1) {
				juce::File projFile = utils::getAppRootDir().getChildFile(params[0]);
				juce::StringArray extensions;
				auto formatsSupport = quickAPI::getProjectFormatsSupported(false);
				for (auto& s : formatsSupport) {
					extensions.add(s.trimCharactersAtStart("*"));
				}

				if (projFile.existsAsFile() && extensions.contains(projFile.getFileExtension())) {
					CoreCommandTarget::getInstance()->systemRequestOpen(params[0]);
				}
			}
		}
	}
};

START_JUCE_APPLICATION(MainApplication)
