/*
WhenWasIt (birthday reminder) plugin for Miranda IM

Copyright © 2006 Cristian Libotean

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#include "stdafx.h"

#define COMMENT_CHAR '#'

int bShouldCheckBirthdays = 0;
int bBirthdayFound = 0;

int InitServices()
{
	Log("%s", "Entering function " __FUNCTION__);

	commonData.foreground = db_get_dw(NULL, ModuleName, "Foreground", FOREGROUND_COLOR);
	commonData.background = db_get_dw(NULL, ModuleName, "Background", BACKGROUND_COLOR);
	commonData.checkInterval = db_get_w(NULL, ModuleName, "CheckInterval", CHECK_INTERVAL);
	commonData.daysInAdvance = db_get_w(NULL, ModuleName, "DaysInAdvance", DAYS_TO_NOTIFY);
	commonData.daysAfter = db_get_w(NULL, ModuleName, "DaysAfter", DAYS_TO_NOTIFY_AFTER);
	commonData.popupTimeout = db_get_w(NULL, ModuleName, "PopupTimeout", POPUP_TIMEOUT);
	commonData.popupTimeoutToday = db_get_w(NULL, ModuleName, "PopupTimeoutToday", commonData.popupTimeout);
	commonData.bUsePopups = db_get_b(NULL, ModuleName, "UsePopups", TRUE);
	commonData.bUseDialog = db_get_b(NULL, ModuleName, "UseDialog", TRUE);
	commonData.bIgnoreSubcontacts = db_get_b(NULL, ModuleName, "IgnoreSubcontacts", FALSE);
	commonData.cShowAgeMode = db_get_b(NULL, ModuleName, "ShowCurrentAge", FALSE);
	commonData.bNoBirthdaysPopup = db_get_b(NULL, ModuleName, "NoBirthdaysPopup", FALSE);
	commonData.bOpenInBackground = db_get_b(NULL, ModuleName, "OpenInBackground", FALSE);
	commonData.cSoundNearDays = db_get_b(NULL, ModuleName, "SoundNearDays", BIRTHDAY_NEAR_DEFAULT_DAYS);
	commonData.cDefaultModule = db_get_b(NULL, ModuleName, "DefaultModule", 0);
	commonData.lPopupClick = db_get_b(NULL, ModuleName, "PopupLeftClick", 2);
	commonData.rPopupClick = db_get_b(NULL, ModuleName, "PopupRightClick", 1);
	commonData.bOncePerDay = db_get_b(NULL, ModuleName, "OncePerDay", 0);
	commonData.cDlgTimeout = db_get_w(NULL, ModuleName, "DlgTimeout", POPUP_TIMEOUT);
	commonData.notifyFor = db_get_b(NULL, ModuleName, "NotifyFor", 0);

	CreateServiceFunction(MS_WWI_CHECK_BIRTHDAYS, CheckBirthdaysService);
	CreateServiceFunction(MS_WWI_LIST_SHOW, ShowListService);
	CreateServiceFunction(MS_WWI_ADD_BIRTHDAY, AddBirthdayService);
	CreateServiceFunction(MS_WWI_REFRESH_USERDETAILS, RefreshUserDetailsService);
	CreateServiceFunction(MS_WWI_IMPORT_BIRTHDAYS, ImportBirthdaysService);
	CreateServiceFunction(MS_WWI_EXPORT_BIRTHDAYS, ExportBirthdaysService);

	Log("%s", "Leaving function " __FUNCTION__);
	return 0;
}

/*
returns -1 if notify is not necesarry
returns daysToBirthday if it should notify
*/
int NotifyContactBirthday(MCONTACT hContact, time_t now, int daysInAdvance)
{
	int year, month, day;
	GetContactDOB(hContact, year, month, day);
	int daysToBirthday = DaysToBirthday(now, year, month, day);
	if (daysInAdvance >= daysToBirthday)
		return daysToBirthday;

	return -1;
}

// returns -1 if notify is not necessary
// returns daysAfterBirthday if it should notify
int NotifyMissedContactBirthday(MCONTACT hContact, time_t now, int daysAfter)
{
	if (daysAfter > 0)
	{
		int year, month, day;
		GetContactDOB(hContact, year, month, day);
		int daysAfterBirthday = DaysAfterBirthday(now, year, month, day);
		if ((daysAfterBirthday >= 0) && (daysAfter >= daysAfterBirthday))
		{
			return daysAfterBirthday;
		}
	}

	return -1;
}

//the timer functions call this service with lParam = 1
//lParam = 0 - force check, lParam - 1 do not force it.

INT_PTR CheckBirthdaysService(WPARAM, LPARAM lParam)
{
	bBirthdayFound = 0; //no birthdays have been found in the given interval

	SYSTEMTIME today;
	GetLocalTime(&today);

	DWORD lastChecked = db_get_dw(NULL, ModuleName, "LastChecked", 0); //get last checked date
	int lcDay = LOBYTE(LOWORD(lastChecked));
	int lcMonth = HIBYTE(LOWORD(lastChecked));
	int lcYear = HIWORD(lastChecked);

	int daysAfter = DaysAfterBirthday(Today(), lcYear, lcMonth, lcDay); //get difference between last checked date and today
	int savedDaysAfter = 0;

	savedDaysAfter = commonData.daysAfter; //save value

	if ((daysAfter > commonData.daysAfter) && (commonData.daysAfter > 0))//check for passed birthdays
		commonData.daysAfter = daysAfter; //bigger values of the two

	if ((lParam) && (commonData.bOncePerDay)) //if force check then we don't take OncePerDay into account
		if (lcDay == today.wDay && lcMonth == today.wMonth && lcYear == today.wYear)
			return 0; //already checked today

	bShouldCheckBirthdays = 1;
	RefreshAllContactListIcons();
	if ((!bBirthdayFound) && (commonData.bNoBirthdaysPopup))
		PopupNotifyNoBirthdays();

	bShouldCheckBirthdays = 0;

	commonData.daysAfter = savedDaysAfter; //restore previous value

	if (lParam) //if not forced - i.e. timer check
		db_set_dw(NULL, ModuleName, "LastChecked", MAKELONG(MAKEWORD(today.wDay, today.wMonth), today.wYear)); //write the value in DB so we don't check again today

	return 0;
}

INT_PTR ShowListService(WPARAM, LPARAM)
{
	if (!hBirthdaysDlg)
		hBirthdaysDlg = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_BIRTHDAYS), nullptr, DlgProcBirthdays);

	ShowWindow(hBirthdaysDlg, SW_SHOW);
	return 0;
}

INT_PTR AddBirthdayService(WPARAM hContact, LPARAM)
{
	HWND hWnd = WindowList_Find(hAddBirthdayWndsList, hContact);
	if (!hWnd)
		hWnd = CreateDialogParam(hInstance, MAKEINTRESOURCE(IDD_ADD_BIRTHDAY), nullptr, DlgProcAddBirthday, hContact);

	return ShowWindow(hWnd, SW_SHOW);
}

void ShowPopupMessage(const wchar_t *title, const wchar_t *message, HANDLE icon)
{
	POPUPDATAT pd = { 0 };
	pd.lchIcon = IcoLib_GetIconByHandle(icon);
	wcsncpy_s(pd.lptzContactName, title, _TRUNCATE);
	wcsncpy_s(pd.lptzText, message, _TRUNCATE);
	pd.colorText = commonData.foreground;
	pd.colorBack = commonData.background;
	PUAddPopupT(&pd);
}

void __cdecl RefreshUserDetailsWorkerThread(void*)
{
	Thread_SetName("WhenWasIt: RefreshUserDetailsWorkerThread");

	ShowPopupMessage(TranslateT("WhenWasIt"), TranslateT("Starting to refresh user details"), hRefreshUserDetails);
	int delay = db_get_w(NULL, ModuleName, "UpdateDelay", REFRESH_DETAILS_DELAY);

	MCONTACT hContact = db_find_first();
	while (hContact != NULL) {
		ProtoChainSend(hContact, PSS_GETINFO, 0, 0);
		hContact = db_find_next(hContact);
		if (hContact)
			Sleep(delay); //sleep for a few seconds between requests
	}
	ShowPopupMessage(TranslateT("WhenWasIt"), TranslateT("Done refreshing user details"), hRefreshUserDetails);
}

INT_PTR RefreshUserDetailsService(WPARAM, LPARAM)
{
	mir_forkthread(RefreshUserDetailsWorkerThread, nullptr);
	return 0;
}

INT_PTR ImportBirthdaysService(WPARAM, LPARAM)
{
	wchar_t fileName[1024] = { 0 };
	OPENFILENAME of = { 0 };
	of.lStructSize = sizeof(OPENFILENAME);
	//of.hInstance = hInstance;
	wchar_t filter[MAX_PATH];
	mir_snwprintf(filter, L"%s (*" BIRTHDAY_EXTENSION L")%c*" BIRTHDAY_EXTENSION L"%c", TranslateT("Birthdays files"), 0, 0);
	of.lpstrFilter = filter;
	of.lpstrFile = fileName;
	of.nMaxFile = _countof(fileName);
	of.lpstrTitle = TranslateT("Please select a file to import birthdays from...");
	of.Flags = OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&of)) {
		wchar_t buffer[2048];
		mir_snwprintf(buffer, TranslateT("Importing birthdays from file: %s"), fileName);
		ShowPopupMessage(TranslateT("WhenWasIt"), buffer, hImportBirthdays);
		DoImport(fileName);
		ShowPopupMessage(TranslateT("WhenWasIt"), TranslateT("Done importing birthdays"), hImportBirthdays);
	}

	return 0;
}

INT_PTR ExportBirthdaysService(WPARAM, LPARAM)
{
	wchar_t fileName[1024] = { 0 };
	OPENFILENAME of = { 0 };
	of.lStructSize = sizeof(OPENFILENAME);
	//of.hInstance = hInstance;
	wchar_t filter[MAX_PATH];
	mir_snwprintf(filter, L"%s (*" BIRTHDAY_EXTENSION L")%c*" BIRTHDAY_EXTENSION L"%c%s (*.*)%c*.*%c", TranslateT("Birthdays files"), 0, 0, TranslateT("All Files"), 0, 0);
	of.lpstrFilter = filter;
	of.lpstrFile = fileName;
	of.nMaxFile = _countof(fileName);
	of.lpstrTitle = TranslateT("Please select a file to export birthdays to...");

	if (GetSaveFileName(&of)) {
		wchar_t buffer[2048];
		wchar_t *fn = wcsrchr(fileName, '\\') + 1;
		if (!wcschr(fn, '.'))
			mir_wstrcat(fileName, BIRTHDAY_EXTENSION);

		mir_snwprintf(buffer, TranslateT("Exporting birthdays to file: %s"), fileName);
		ShowPopupMessage(TranslateT("WhenWasIt"), buffer, hExportBirthdays);
		DoExport(fileName);
		ShowPopupMessage(TranslateT("WhenWasIt"), TranslateT("Done exporting birthdays"), hExportBirthdays);
	}

	return 0;
}

int DoImport(wchar_t *fileName)
{
	FILE *fin = _wfopen(fileName, L"rt");
	if (!fin) {
		MessageBox(nullptr, TranslateT("Could not open file to import birthdays"), TranslateT("Error"), MB_OK | MB_ICONERROR);
		return 1;
	}

	int mode = commonData.cDefaultModule;

	while (!feof(fin)) {
		wchar_t buffer[4096];
		fgetws(buffer, _countof(buffer), fin);
		if (buffer[0] == COMMENT_CHAR)
			continue;

		wchar_t *delAccount = wcsstr(buffer, L" : ");
		if (delAccount) {
			int tmp = delAccount[0];
			delAccount[0] = '\0';
			wchar_t *delProto = wcsrchr(buffer, '@');
			if (delProto) {
				delProto[0] = '\0';

				wchar_t *szHandle = buffer;
				wchar_t *szProto = delProto + 1;
				MCONTACT hContact = GetContactFromID(szHandle, szProto);
				if (hContact) {
					delProto[0] = tmp;
					delAccount[0] = tmp;

					int year, month, day;
					swscanf(delAccount, L" : %02d/%02d/%04d", &day, &month, &year);
					SaveBirthday(hContact, year, month, day, mode);
				}
				else {
					CMStringW msg(FORMAT, TranslateT("Could not find UID '%s [%S]' in current database, skipping"), szHandle, szProto);
					ShowPopupMessage(TranslateT("Warning"), msg, hImportBirthdays);
				}
			}
		}
	}

	fclose(fin);
	return 0;
}

int DoExport(wchar_t *fileName)
{
	FILE *fout = _wfopen(fileName, L"wt");
	if (!fout) {
		MessageBox(nullptr, TranslateT("Could not open file to export birthdays"), TranslateT("Error"), MB_OK | MB_ICONERROR);
		return 1;
	}
	fwprintf(fout, L"%c%s", COMMENT_CHAR, TranslateT("Please do not edit this file by hand. Use the export function of WhenWasIt plugin.\n"));
	fwprintf(fout, L"%c%s", COMMENT_CHAR, TranslateT("Warning! Please do not mix Unicode and Ansi exported birthday files. You should use the same version (Ansi/Unicode) of WhenWasIt that was used to export the info.\n"));
	fwprintf(fout, L"%c%s", COMMENT_CHAR, TranslateT("This file was exported with a Unicode version of WhenWasIt. Please only use a Unicode version of the plugin to import the birthdays.\n"));

	for (MCONTACT hContact = db_find_first(); hContact; hContact = db_find_next(hContact)) {
		int year, month, day;
		GetContactDOB(hContact, year, month, day);
		if (IsDOBValid(year, month, day)) {
			char *szProto = GetContactProto(hContact);
			wchar_t *szHandle = GetContactID(hContact, szProto);

			if ((szHandle) && (mir_strlen(szProto) > 0))
				fwprintf(fout, L"%s@%S : %02d/%02d/%04d\n", szHandle, szProto, day, month, year);

			if (szHandle)
				free(szHandle);
		}
	}

	fclose(fout);
	return 0;
}
