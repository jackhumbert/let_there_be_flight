// -----------------------------------------------------------------------------
// LocalizationSystem
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

module BaseLib.Localization

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

	private func OnAttach() -> Void {
		this.m_interfaceTranslationData = new inkHashMap();
		this.m_subtitleTranslationData = new inkHashMap();

		this.m_settingsWatcher = new LanguageSettingsWatcher();
		this.m_settingsWatcher.Initialize(this.GetGameInstance());
		this.m_settingsWatcher.Start();

		this.UpdateLocale();
		this.UpdateTranslations();

		this.QueueRequest(UpdateGenderRequest.Create());
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

	private func UpdateLocale() -> Void {
		let settings: ref<UserSettings> = GameInstance.GetSettingsSystem(this.GetGameInstance());

		this.m_interfaceLanguage = (settings.GetVar(n"/language", n"OnScreen") as ConfigVarListName).GetValue();
		this.m_subtitleLanguage = (settings.GetVar(n"/language", n"Subtitles") as ConfigVarListName).GetValue();
		this.m_voiceLanguage = (settings.GetVar(n"/language", n"VoiceOver") as ConfigVarListName).GetValue();
	}

	private func UpdateGender() -> Void {
		let genderName: CName = GetPlayer(this.GetGameInstance()).GetResolvedGenderName();

		this.m_playerGender = Equals(genderName, n"Male") ? PlayerGender.Male : PlayerGender.Female;
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
			return;
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
		return GameInstance.GetScriptableSystemsContainer(game).Get(n"BaseLib.Localization.LocalizationSystem") as LocalizationSystem;
	}
}
