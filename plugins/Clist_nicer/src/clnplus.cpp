/*
 * test for gdi+
 */

#include "stdafx.h"
#include <shobjidl.h>
#include "msapi/comptr.h"

#define _CLN_GDIP

void RemoveFromTaskBar(HWND hWnd)
{
	CComPtr<ITaskbarList> pTaskbarList;
	if (SUCCEEDED(pTaskbarList.CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER)))
		if (SUCCEEDED(pTaskbarList->HrInit()))
			pTaskbarList->DeleteTab(hWnd);
}

void AddToTaskBar(HWND hWnd)
{
	CComPtr<ITaskbarList> pTaskbarList;
	if (SUCCEEDED(pTaskbarList.CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER)))
		if (SUCCEEDED(pTaskbarList->HrInit()))
			pTaskbarList->AddTab(hWnd);
}
