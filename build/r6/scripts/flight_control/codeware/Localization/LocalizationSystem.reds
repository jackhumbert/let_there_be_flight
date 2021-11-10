// -----------------------------------------------------------------------------
// Codeware.Localization.LocalizationSystem
// -----------------------------------------------------------------------------
//
// public class LocalizationSystem extends ScriptableSystem {
//   public func GetInterfaceLanguage() -> CName
//   public func GetSubtitleLanguage() -> CName
//   public func GetVoiceLanguage() -> CName
//   public func GetPlayerGender() -> PlayerGender
//   public func GetText(key: String) -> String
//   public func GetSubtitle(key: String) -> String
//   public func RegisterProvider(provider: ref<ModLocalizationProvider>) -> Void
//   public static func GetInstance(game: GameInstance) -> ref<LocalizationSystem>
// }
//

module Codeware.Localization

public class LocalizationSystem extends ScriptableSystem {
	private let m_interfaceLanguage: CName;

	private let m_subtitleLanguage: CName;

	private let m_voiceLanguage: CName;

	private let m_playerGender: PlayerGender;

	private let m_providers: array<ref<ModLocalizationProvider>>;

	private let m_interfaceTranslationLanguage: CName;

	private let m_interfaceTranslationData: ref<inkHashMap>;

	private let m_subtitleTranslationLanguage: CName;

	private let m_subtitleTranslationData: ref<inkHashMap>;

	private let m_settingsWatcher: ref<LanguageSettingsWatcher>;

	private let m_genderWatcher: ref<PlayerGenderWatcher>;

	private let m_localeChanged: Bool;

	private let m_genderChanged: Bool;

	private func OnAttach() -> Void {
		this.m_interfaceTranslationData = new inkHashMap();
		this.m_subtitleTranslationData = new inkHashMap();

		this.m_settingsWatcher = new LanguageSettingsWatcher();
		this.m_settingsWatcher.Initialize(this.GetGameInstance());
		this.m_settingsWatcher.Start();

		this.m_genderWatcher = new PlayerGenderWatcher();
		this.m_genderWatcher.Initialize(this.GetGameInstance());
		this.m_genderWatcher.Start();

		this.UpdateLocale();
		this.UpdateTranslations();

		this.QueueRequest(UpdateGenderRequest.Create());
	}

	private func OnDetach() -> Void {
		this.m_genderWatcher.Stop();
	}

	private func OnRegisterProviderRequest(request: ref<RegisterProviderRequest>) -> Void {
		this.RegisterProvider(request.GetProvider());
	}

	private func OnUpdateLocaleRequest(request: ref<UpdateLocaleRequest>) -> Void {
		this.UpdateLocale();
		this.UpdateTranslations();
	}

	private func OnUpdateGenderRequest(request: ref<UpdateGenderRequest>) -> Void {
		this.UpdateGender();
	}

	private func OnUpdateTranslationsRequest(request: ref<UpdateTranslationsRequest>) -> Void {
		if request.IsForced() {
			this.InvalidateTranslations();
		}

		this.UpdateTranslations();
	}

	private func NotifyProviders() -> Void {
		if this.m_localeChanged {
			for provider in this.m_providers {
				provider.OnLocaleChange();
			}
			this.m_localeChanged = false;
		}

		if this.m_genderChanged {
			for provider in this.m_providers {
				provider.OnGenderChange();
			}
			this.m_genderChanged = false;
		}
	}

	private func UpdateLocale() -> Void {
		let settings: ref<UserSettings> = GameInstance.GetSettingsSystem(this.GetGameInstance());

		let interfaceLanguage: CName = (settings.GetVar(n"/language", n"OnScreen") as ConfigVarListName).GetValue();
		let subtitleLanguage: CName = (settings.GetVar(n"/language", n"Subtitles") as ConfigVarListName).GetValue();
		let voiceLanguage: CName = (settings.GetVar(n"/language", n"VoiceOver") as ConfigVarListName).GetValue();

		if NotEquals(this.m_interfaceLanguage, interfaceLanguage) {
			this.m_interfaceLanguage = interfaceLanguage;
			this.m_localeChanged = true;
		}

		if NotEquals(this.m_subtitleLanguage, subtitleLanguage) {
			this.m_subtitleLanguage = subtitleLanguage;
			this.m_localeChanged = true;
		}

		if NotEquals(this.m_voiceLanguage, voiceLanguage) {
			this.m_voiceLanguage = voiceLanguage;
			this.m_localeChanged = true;
		}

		this.NotifyProviders();
	}

	private func UpdateGender() -> Void {
		let playerGenderName: CName = GetPlayer(this.GetGameInstance()).GetResolvedGenderName();
		let playerGender: PlayerGender = Equals(playerGenderName, n"Male") ? PlayerGender.Male : PlayerGender.Female;

		if NotEquals(this.m_playerGender, playerGender) {
			this.m_playerGender = playerGender;
			this.m_genderChanged = true;
		}

		this.NotifyProviders();
	}

	private func UpdateTranslations() -> Void {
		if NotEquals(this.m_interfaceTranslationLanguage, this.m_interfaceLanguage) {
			this.CollectTranslationData(this.m_interfaceTranslationData, EntryType.Interface, this.m_interfaceLanguage);
			this.m_interfaceTranslationLanguage = this.m_interfaceLanguage;
		}

		if NotEquals(this.m_subtitleTranslationLanguage, this.m_subtitleLanguage) {
			this.CollectTranslationData(this.m_subtitleTranslationData, EntryType.Subtitle, this.m_subtitleLanguage);
			this.m_subtitleTranslationLanguage = this.m_subtitleLanguage;
		}
	}

	private func MergeTranslations(provider: ref<ModLocalizationProvider>) -> Void {
		if NotEquals(this.m_interfaceTranslationLanguage, n"") {
			this.FillTranslationData(this.m_interfaceTranslationData, provider, EntryType.Interface, this.m_interfaceTranslationLanguage);
		}

		if NotEquals(this.m_subtitleTranslationLanguage, n"") {
			this.FillTranslationData(this.m_subtitleTranslationData, provider, EntryType.Subtitle, this.m_subtitleTranslationLanguage);
		}
	}

	private func InvalidateTranslations() -> Void {
		this.m_interfaceTranslationLanguage = n"";
		this.m_subtitleTranslationLanguage = n"";
	}

	private func CollectTranslationData(translations: ref<inkHashMap>, type: EntryType, language: CName) -> Void {
		translations.Clear();

		for provider in this.m_providers {
			this.FillTranslationData(translations, provider, type, language);
		}
	}

	private func FillTranslationData(translations: ref<inkHashMap>, provider: ref<ModLocalizationProvider>, type: EntryType, language: CName) -> Void {
		let package: ref<ModLocalizationPackage> = provider.GetPackage(language);

		if !IsDefined(package) {
			let fallback: CName = provider.GetFallback();

			if Equals(fallback, n"") {
				return;
			}

			package = provider.GetPackage(fallback);

			if !IsDefined(package) {
				return;
			}
		}

		let values: array<wref<IScriptable>>;

		package.GetEntries(type).GetValues(values);

		for value in values {
			let entry: wref<LocalizationEntry> = value as LocalizationEntry;
			let hash: Uint64 = LocalizationEntry.Hash(entry.GetKey());

			if !translations.KeyExist(hash) {
				translations.Insert(hash, entry);
			} else {
				translations.Set(hash, entry);
			}
		}
	}

	private func GetTranslationFrom(translations: ref<inkHashMap>, key: String) -> String {
		let hash: Uint64 = LocalizationEntry.Hash(key);

		if translations.KeyExist(hash) {
			return (translations.Get(hash) as LocalizationEntry).GetVariant(this.m_playerGender);
		}

		let fallback: String = GetLocalizedText(key);

		if StrLen(fallback) > 0 {
			return fallback;
		}

		return key;
	}

	public func GetText(key: String) -> String {
		return this.GetTranslationFrom(this.m_interfaceTranslationData, key);
	}

	public func GetSubtitle(key: String) -> String {
		return this.GetTranslationFrom(this.m_subtitleTranslationData, key);
	}

	public func GetInterfaceLanguage() -> CName {
		return this.m_interfaceLanguage;
	}

	public func GetSubtitleLanguage() -> CName {
		return this.m_interfaceLanguage;
	}

	public func GetVoiceLanguage() -> CName {
		return this.m_interfaceLanguage;
	}

	public func GetPlayerGender() -> PlayerGender {
		return this.m_playerGender;
	}

	public func RegisterProvider(provider: ref<ModLocalizationProvider>) -> Void {
		ArrayPush(this.m_providers, provider);

		this.MergeTranslations(provider);
	}

	public static func GetInstance(game: GameInstance) -> ref<LocalizationSystem> {
		return GameInstance.GetScriptableSystemsContainer(game).Get(n"Codeware.Localization.LocalizationSystem") as LocalizationSystem;
	}
}
