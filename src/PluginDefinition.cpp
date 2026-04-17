//this file is part of notepad++
//Copyright (C)2022 Don HO <don.h@free.fr>
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <stdio.h>
#include <windows.h>

#include "PluginDefinition.h"
#include "menuCmdID.h"

#include "readtags.h"

//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;

//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE /*hModule*/)
{
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{

	//--------------------------------------------//
	//-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
	//--------------------------------------------//
	// with function :
	// setCommand(int index,                      // zero based number to indicate the order of command
	//            TCHAR *commandName,             // the command name that you want to see in plugin menu
	//            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
	//            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
	//            bool check0nInit                // optional. Make this menu item be checked visually
	//            );
	setCommand(0, TEXT("Go To Definition"), go_to_definition, NULL, false);
	setCommand(1, TEXT("Go Back"), go_back, NULL, false);
}

//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
	// Don't forget to deallocate your shortcut here
}


//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) 
{
	if (index >= nbFunc)
		return false;

	if (!pFunc)
		return false;

	lstrcpy(funcItem[index]._itemName, cmdName);
	funcItem[index]._pFunc = pFunc;
	funcItem[index]._init2Check = check0nInit;
	funcItem[index]._pShKey = sk;

	return true;
}

inline bool streq(const char* s1, const char* s2)
{
	return strcmp(s1, s2) == 0;
}

wchar_t last_filepath[256];
int last_pos = 0;
bool same_file = false;

void go_back()
{
	int which = -1;
	::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
	HWND curScintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;

	SendMessage(nppData._nppHandle, NPPM_DOOPEN, 0, (LPARAM)&last_filepath);
	::SendMessage(curScintilla, SCI_GOTOPOS, last_pos, 0);

	if (same_file)
		::SendMessage(curScintilla, SCI_VERTICALCENTRECARET, 0, 0);
}

bool paths_equal(wchar_t *a, wchar_t *b)
{
	if (wcslen(a) != wcslen(b))
		return false;

	for (int i = 0; i < wcslen(a); i++) {
		if ((a[i] == '\\' || a[i] == '/') && (b[i] == '\\' || b[i] == '/'))
			continue;

		if (a[i] != b[i])
			return false;
	}

	return true;
}

//----------------------------------------------//
//-- STEP 4. DEFINE YOUR ASSOCIATED FUNCTIONS --//
//----------------------------------------------//
void go_to_definition()
{
	// Open a new document
	//::SendMessage(nppData._nppHandle, NPPM_MENUCOMMAND, 0, IDM_FILE_NEW);

	// Get the current scintilla
	//int which = -1;
	//::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
	//if (which == -1)
	//    return;
	//HWND curScintilla = (which == 0)?nppData._scintillaMainHandle:nppData._scintillaSecondHandle;

	// Say hello now :
	// Scintilla control has no Unicode mode, so we use (char *) here
	//::SendMessage(curScintilla, SCI_SETTEXT, 0, (LPARAM)"Hello, Notepad++!");

	// Get the current scintilla
	int which = -1;
	::SendMessage(nppData._nppHandle, NPPM_GETCURRENTSCINTILLA, 0, (LPARAM)&which);
	HWND curScintilla = (which == 0) ? nppData._scintillaMainHandle : nppData._scintillaSecondHandle;


	::SendMessage(nppData._nppHandle, NPPM_GETFULLCURRENTPATH, 256, (LPARAM)&last_filepath); 
	last_pos = ::SendMessage(curScintilla, SCI_GETCURRENTPOS, 0, 0);
	
	wchar_t current_word[256];
	::SendMessage(nppData._nppHandle, NPPM_GETCURRENTWORD, 0, (LPARAM)current_word); // This selects the word for some reason.
	::SendMessage(curScintilla, SCI_GOTOPOS, last_pos, 0); // We do this to clear that selection.

	int len = WideCharToMultiByte(CP_UTF8, 0, current_word, -1, NULL, 0, NULL, NULL);
	char* current_word_utf8 = (char *)malloc(len);
	WideCharToMultiByte(CP_UTF8, 0, current_word, -1, current_word_utf8, len, NULL, NULL);

	tagFile* tag_file = tagsOpen("C:/Users/ntaul/Code/Dissertation/project/tags", NULL);
	tagEntry tag_entry;
	same_file = false;
	while (tagsNext(tag_file, &tag_entry) == TagSuccess) {
		if (streq(tag_entry.name, current_word_utf8)) {
			wchar_t filepath[256];
			MultiByteToWideChar(CP_UTF8, 0, tag_entry.file, -1, filepath, 256);

			int line;
			int col;

			sscanf(tag_entry.address.pattern, "/\\%dl\\%dc", &line, &col) == 2;

			if (paths_equal(last_filepath, filepath))
				same_file = true;

			SendMessage(nppData._nppHandle, NPPM_DOOPEN, 0, (LPARAM)&filepath);
			::SendMessage(curScintilla, SCI_GOTOLINE, line - 1, 0);
			::SendMessage(curScintilla, SCI_VERTICALCENTRECARET, 0, 0);

			break;
		}
	}
	tagsClose(tag_file);
}

void helloDlg()
{
	::MessageBox(NULL, TEXT("Hello, Notepad++!"), TEXT("Notepad++ Plugin Template"), MB_OK);
}
