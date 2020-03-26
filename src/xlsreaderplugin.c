/*
** Xls Reader Hollywood plugin
** Copyright (C) 2020 Christophe Gouiran <bechris13250@gmail.com>
**
** Permission is hereby granted, free of charge, to any person obtaining a copy
** of this software and associated documentation files (the "Software"), to deal
** in the Software without restriction, including without limitation the rights
** to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
** copies of the Software, and to permit persons to whom the Software is
** furnished to do so, subject to the following conditions:
**
** The above copyright notice and this permission notice shall be included in
** all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <hollywood/plugin.h>

#include "xlsreaderplugin.h"
#include "version.h"

#include "xls.h"

// pointer to the Hollywood plugin API
static hwPluginAPI *hwcl = NULL;

static hwPluginBase *hwpb = NULL;

// information about our plugin for InitPlugin()
// (NB: we store the version string after the plugin's name; this is not required by Hollywood;
// it is just a trick to prevent the linker from optimizing our version string away)
static const char plugin_name[] = PLUGIN_NAME "\0$VER: " PLUGIN_MODULENAME ".hwp " PLUGIN_VER_STR " (" PLUGIN_DATE ") [" PLUGIN_PLAT "]";
static const char plugin_modulename[] = PLUGIN_MODULENAME;
static const char plugin_author[] = PLUGIN_AUTHOR;
static const char plugin_description[] = PLUGIN_DESCRIPTION;
static const char plugin_copyright[] = PLUGIN_COPYRIGHT;
static const char plugin_url[] = PLUGIN_URL;
static const char plugin_date[] = PLUGIN_DATE;

// all functions will be added to this table
static const char *basetable = "xlsreader";

// custom errors registered by this plugin
static int PERR_LIBXLSERROR;

/*
** WARNING: InitPlugin() will be called by *any* Hollywood version >= 5.0. Thus, you must
** check the Hollywood version that called your InitPlugin() implementation before calling
** functions from the hwPluginAPI pointer or accessing certain structure members. Your
** InitPlugin() implementation must be compatible with *any* Hollywood version >= 5.0. If
** you call Hollywood 6.0 functions here without checking first that Hollywood 6.0 or higher
** has called your InitPlugin() implementation, *all* programs compiled with Hollywood
** versions < 6.0 *will* crash when they try to open your plugin! 
*/
HW_EXPORT int InitPlugin(hwPluginBase *self, hwPluginAPI *cl, STRPTR path)
{
	// open Amiga libraries needed by this plugin		
#ifdef HW_AMIGA
	if(!initamigastuff()) return FALSE;
#endif

	// identify as a file plugin to Hollywood
	self->CapsMask = HWPLUG_CAPS_LIBRARY;
	self->Version = PLUGIN_VER;
	self->Revision = PLUGIN_REV;

	// we want to be compatible with Hollywood 6.0
	// **WARNING**: when compiling with newer SDK versions you have to be very
	// careful which functions you call and which structure members you access
	// because not all of them are present in earlier versions. Thus, if you
	// target versions older than your SDK version you have to check the hollywood.h
	// header file very carefully to check whether the older version you want to
	// target has the respective feature or not
	self->hwVersion = 6;
	self->hwRevision = 0;
	
	// set plugin information; note that these string pointers need to stay
	// valid until Hollywood calls ClosePlugin()		
	self->Name = (STRPTR) plugin_name;
	self->ModuleName = (STRPTR) plugin_modulename;	
	self->Author = (STRPTR) plugin_author;
	self->Description = (STRPTR) plugin_description;
	self->Copyright = (STRPTR) plugin_copyright;
	self->URL = (STRPTR) plugin_url;
	self->Date = (STRPTR) plugin_date;
	self->Settings = NULL;
	self->HelpFile = NULL;

	hwpb = self;

	// NB: "cl" can be NULL in case Hollywood or Designer just wants to obtain information
	// about our plugin
	if(cl) {
			
		hwcl = cl;

		// register a custom error for our plugin
		PERR_LIBXLSERROR = hwcl->SysBase->hw_RegisterError("XlsReader : %s");
	}

	return TRUE;
}

/*
** WARNING: ClosePlugin() will be called by *any* Hollywood version >= 5.0.
** --> see the note above in InitPlugin() for information on how to implement this function
*/
HW_EXPORT void ClosePlugin(void)
{
#ifdef HW_AMIGA
	freeamigastuff();
#endif
}

/* Open an .xls file and:
** 1)If everything went well return its content as a table as described below:
** {
**   ["first tab"] = {
**    (note:first row) {"First column value", "Second column value", "Third column value", ...},
**    (note:second row) {"First column value", "Second column value", "Third column value", ...},
**    (note:third row) {"First column value", "Second column value", "Third column value", ...},
**     ...
**   },
**   ["second tab"] = {
**    (note:first row) {"First column value", "Second column value", "Third column value", ...},
**    (note:second row) {"First column value", "Second column value", "Third column value", ...},
**    (note:third row) {"First column value", "Second column value", "Third column value", ...},
**     ...
**   },
**   ...
** }
**
** 2)If something went wrong returns PERR_LIBXLSERROR error code ()
*/
static SAVEDS int hw_OpenXls(lua_State *L)
{
	const char *file = luaL_checkfilename(L, 1);

	xlsWorkBook* pWB;
	xlsWorkSheet* pWS;
    xls_error_t error = LIBXLS_OK;
	pWB = xls_open_file(hwcl->DOSBase, file, "UTF-8", &error);
	if (pWB) {

		lua_newtable(L);

		for (unsigned int i = 0; i < pWB->sheets.count; ++i) {

			lua_pushstring(L, pWB->sheets.sheet[i].name);

			// open and parse the sheet
			pWS = xls_getWorkSheet(pWB, i);
			xls_parseWorkSheet(pWS);

			lua_newtable(L);

			for (unsigned int j = 0; j <= (unsigned int)pWS->rows.lastrow; ++j) {
				XLS_WORD cellRow = (XLS_WORD)j;

				XLS_WORD cellCol;
				int tabIdx = 0;

				lua_pushnumber(L, j);

				lua_newtable(L);

				for (cellCol = 0; cellCol <= pWS->rows.lastcol; ++cellCol) {
					xlsCell *cell = xls_cell(pWS, cellRow, cellCol);

					if ((!cell) || (cell->isHidden)) {
						continue;
					}

					lua_pushnumber(L, tabIdx);
					++tabIdx;

					// extract the value of the cell (either numeric or string)
					if (cell->id == XLS_RECORD_RK || cell->id == XLS_RECORD_MULRK || cell->id == XLS_RECORD_NUMBER) {
						lua_pushnumber(L, cell->d);
					} else if (cell->id == XLS_RECORD_FORMULA || cell->id == XLS_RECORD_FORMULA_ALT) {
                    	// formula
						if (cell->l == 0) // its a number
						{
							lua_pushnumber(L, cell->d);
						} else {
							if (!strcmp((char *)cell->str, "bool")) // its boolean, and test cell->d
							{
								lua_pushstring(L, (int) cell->d ? "true" : "false");
							} else if (!strcmp((char *)cell->str, "error")) // formula is in error
							{
								lua_pushstring(L, "*error*");
							} else // ... cell->str is valid as the result of a string formula.
							{
								lua_pushstring(L, (char *)cell->str);
							}
						}
					} else if (cell->str != NULL) {
						lua_pushstring(L, (char *)cell->str);
					} else {
						lua_pushstring(L, "");
					}
					lua_rawset(L, -3);
				}
				lua_rawset(L, -3);
			}
			xls_close_WS(pWS);
			lua_rawset(L, -3);
		}
		xls_close(pWB);
		return 1;
	}
	else {
		hw_SetErrorString((STRPTR)xls_getError(error));
		return PERR_LIBXLSERROR;
	}

}

/* table containing all commands to be added by this plugin */
struct hwCmdStruct plug_commands[] = {
	{"OpenXls", hw_OpenXls},
	{NULL, NULL}
};

/* table containing all constants to be added by this plugin */
struct hwCstStruct plug_constants[] = {
	{NULL, NULL, 0}
};

/* return base table's name */
HW_EXPORT STRPTR GetBaseTable(void)
{
	return (STRPTR) basetable;
}

/* return command table */
HW_EXPORT struct hwCmdStruct *GetCommands(void)
{
	return (struct hwCmdStruct *) plug_commands;
}

/* return constant table */
HW_EXPORT struct hwCstStruct *GetConstants(void)
{
	return (struct hwCstStruct *) plug_constants;
}

/* you may do additional initialization here */
HW_EXPORT int InitLibrary(lua_State *L)
{
	return 0;
}

/* you may do additional clean-up here */
#if defined(HW_WIN32) && defined(HW_64BIT)
HW_EXPORT void _FreeLibrary(lua_State *L)
#else
HW_EXPORT void FreeLibrary(lua_State *L)
#endif
{
}
