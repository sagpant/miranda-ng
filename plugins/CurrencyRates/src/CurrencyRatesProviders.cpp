#include "StdAfx.h"
#include "CurrencyRatesProviderCurrencyConverter.h"

#define LAST_RUN_VERSION "LastRunVersion"

TCurrencyRatesProviders g_apProviders;

/////////////////////////////////////////////////////////////////////////////////////////

template<class T>void create_provider(TCurrencyRatesProviders& g_apProviders)
{
	ICurrencyRatesProvider *pProvider = new T;
	if (pProvider->Init())
		g_apProviders.push_back(pProvider);
};

void CreateProviders()
{
	create_provider<CCurrencyRatesProviderCurrencyConverter>(g_apProviders);
}

/////////////////////////////////////////////////////////////////////////////////////////

void ClearProviders()
{
	g_apProviders.clear();
}

/////////////////////////////////////////////////////////////////////////////////////////

void convert_contact_settings(MCONTACT hContact)
{
	WORD dwLogMode = db_get_w(hContact, CURRENCYRATES_MODULE_NAME, DB_STR_CURRENCYRATE_LOG, static_cast<WORD>(lmDisabled));
	if ((dwLogMode&lmInternalHistory) || (dwLogMode&lmExternalFile))
		db_set_b(hContact, CURRENCYRATES_MODULE_NAME, DB_STR_CONTACT_SPEC_SETTINGS, 1);
}

void InitProviders()
{
	CreateProviders();

	const WORD nCurrentVersion = 17;
	WORD nVersion = db_get_w(0, CURRENCYRATES_MODULE_NAME, LAST_RUN_VERSION, 1);

	for (auto &hContact : Contacts(CURRENCYRATES_MODULE_NAME)) {
		ICurrencyRatesProvider *pProvider = GetContactProviderPtr(hContact);
		if (pProvider) {
			pProvider->AddContact(hContact);
			if (nVersion < nCurrentVersion)
				convert_contact_settings(hContact);
		}
	}

	db_set_w(0, CURRENCYRATES_MODULE_NAME, LAST_RUN_VERSION, nCurrentVersion);
}

/////////////////////////////////////////////////////////////////////////////////////////

ICurrencyRatesProvider* GetContactProviderPtr(MCONTACT hContact)
{
	char* szProto = GetContactProto(hContact);
	if (nullptr == szProto || 0 != ::_stricmp(szProto, CURRENCYRATES_PROTOCOL_NAME))
		return nullptr;

	tstring sProvider = CurrencyRates_DBGetStringW(hContact, CURRENCYRATES_MODULE_NAME, DB_STR_CURRENCYRATE_PROVIDER);
	if (true == sProvider.empty())
		return nullptr;

	return FindProvider(sProvider);
}

/////////////////////////////////////////////////////////////////////////////////////////

ICurrencyRatesProvider* FindProvider(const tstring& rsName)
{
	for (auto &pProvider : g_apProviders) {
		const ICurrencyRatesProvider::CProviderInfo& rInfo = pProvider->GetInfo();
		if (0 == ::mir_wstrcmpi(rsName.c_str(), rInfo.m_sName.c_str()))
			return pProvider;
	}

	return nullptr;
}
