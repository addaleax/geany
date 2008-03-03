/*
 *      keybindings.c - this file is part of Geany, a fast and lightweight IDE
 *
 *      Copyright 2006-2008 Enrico Tröger <enrico(dot)troeger(at)uvena(dot)de>
 *      Copyright 2006-2008 Nick Treleaven <nick(dot)treleaven(at)btinternet(dot)com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $Id$
 */

/*
 * Configurable keyboard shortcuts.
 */

#include <gdk/gdkkeysyms.h>

#include "geany.h"
#include "keybindings.h"
#include "support.h"
#include "utils.h"
#include "ui_utils.h"
#include "document.h"
#include "filetypes.h"
#include "callbacks.h"
#include "prefs.h"
#include "msgwindow.h"
#include "editor.h"
#include "sciwrappers.h"
#include "build.h"
#include "tools.h"
#include "navqueue.h"
#include "symbols.h"
#include "vte.h"


GPtrArray *keybinding_groups;	/* array of KeyBindingGroup pointers */

static const gchar default_group_name[] = "Bindings";

static const gboolean swap_alt_tab_order = FALSE;


static void cb_func_file_action(guint key_id);
static void cb_func_menu_print(guint key_id);

static void cb_func_menu_undo(guint key_id);
static void cb_func_menu_redo(guint key_id);
static void cb_func_clipboard(guint key_id);
static void cb_func_menu_selectall(guint key_id);
static void cb_func_menu_help(guint key_id);
static void cb_func_menu_preferences(guint key_id);
static void cb_func_menu_insert_date(guint key_id);

static void cb_func_menu_search(guint key_id);

static void cb_func_menu_toggle_all(guint key_id);
static void cb_func_menu_fullscreen(guint key_id);
static void cb_func_menu_messagewindow(guint key_id);
static void cb_func_menu_zoomin(guint key_id);
static void cb_func_menu_zoomout(guint key_id);

static void cb_func_menu_replacetabs(guint key_id);
static void cb_func_menu_foldall(guint key_id);
static void cb_func_menu_unfoldall(guint key_id);
static void cb_func_reloadtaglist(guint key_id);

static void cb_func_menu_opencolorchooser(guint key_id);
static void cb_func_menu_insert_specialchars(guint key_id);

static void cb_func_build_action(guint key_id);

static void cb_func_switch_editor(guint key_id);
static void cb_func_switch_scribble(guint key_id);
static void cb_func_switch_vte(guint key_id);
static void cb_func_switch_search_bar(guint key_id);
static void cb_func_switch_tableft(guint key_id);
static void cb_func_switch_tabright(guint key_id);
static void cb_func_switch_tablastused(guint key_id);
static void cb_func_move_tab(guint key_id);
static void cb_func_nav_back(guint key_id);
static void cb_func_nav_forward(guint key_id);
static void cb_func_toggle_sidebar(guint key_id);

/* common function for global editing keybindings. */
static void cb_func_editing_global(guint key_id);

/* common function for editing keybindings, only valid when scintilla has focus. */
static void cb_func_editing(guint key_id);

/* common function for tags keybindings, only valid when scintilla has focus. */
static void cb_func_tags(guint key_id);

/* common function for keybindings using current word */
static void cb_func_current_word(guint key_id);

static void add_menu_accels(void);


/* simple convenience function to fill a KeyBinding struct item */
static void add_kb(KeyBindingGroup *group, gsize kb_id,
		KBCallback func, guint key, GdkModifierType mod,
		const gchar *name, const gchar *label)
{
	KeyBinding *kb;

	g_assert(kb_id < group->count);

	kb = &group->keys[kb_id];

	kb->name = name;
	kb->label = label;
	kb->key = key;
	kb->mods = mod;
	kb->cb_func = func;
}


static KeyBindingGroup *add_kb_group(KeyBindingGroup *group,
		const gchar *name, const gchar *label, gsize count, KeyBinding *keys)
{
	g_ptr_array_add(keybinding_groups, group);

	group->name = name;
	group->label = label;
	group->count = count;
	group->keys = keys;
	return group;
}


/* Expansion for group_id = FILE:
 * static KeyBinding FILE_keys[GEANY_KEYS_FILE_COUNT]; */
#define DECLARE_KEYS(group_id) \
	static KeyBinding group_id ## _keys[GEANY_KEYS_ ## group_id ## _COUNT]

/* Expansion for group_id = FILE:
 * add_kb_group(&groups[GEANY_KEYGROUP_FILE], NULL, _("File menu"),
 * 	GEANY_KEYS_FILE_COUNT, FILE_keys); */
#define ADD_KB_GROUP(group_id, label) \
	add_kb_group(&groups[GEANY_KEYGROUP_ ## group_id], default_group_name, label, \
		GEANY_KEYS_ ## group_id ## _COUNT, group_id ## _keys)

/* init all fields of keys with default values. */
static void init_default_kb(void)
{
	static KeyBindingGroup groups[GEANY_KEYGROUP_COUNT];
	KeyBindingGroup *group;
	DECLARE_KEYS(FILE);
	DECLARE_KEYS(EDIT);
	DECLARE_KEYS(SEARCH);
	DECLARE_KEYS(VIEW);
	DECLARE_KEYS(DOCUMENT);
	DECLARE_KEYS(BUILD);
	DECLARE_KEYS(TOOLS);
	DECLARE_KEYS(HELP);
	DECLARE_KEYS(FOCUS);
	DECLARE_KEYS(NOTEBOOK);
	DECLARE_KEYS(EDITING);
	DECLARE_KEYS(TAGS);
	DECLARE_KEYS(OTHER);

	group = ADD_KB_GROUP(FILE, _("File menu"));

	add_kb(group, GEANY_KEYS_MENU_NEW, cb_func_file_action,
		GDK_n, GDK_CONTROL_MASK, "menu_new", _("New"));
	add_kb(group, GEANY_KEYS_MENU_OPEN, cb_func_file_action,
		GDK_o, GDK_CONTROL_MASK, "menu_open", _("Open"));
	add_kb(group, GEANY_KEYS_MENU_OPENSELECTED, cb_func_file_action,
		GDK_o, GDK_SHIFT_MASK | GDK_CONTROL_MASK, "menu_open_selected", _("Open selected file"));
	add_kb(group, GEANY_KEYS_MENU_SAVE, cb_func_file_action,
		GDK_s, GDK_CONTROL_MASK, "menu_save", _("Save"));
	add_kb(group, GEANY_KEYS_MENU_SAVEAS, cb_func_file_action,
		0, 0, "menu_saveas", _("Save as"));
	add_kb(group, GEANY_KEYS_MENU_SAVEALL, cb_func_file_action,
		GDK_S, GDK_SHIFT_MASK | GDK_CONTROL_MASK, "menu_saveall", _("Save all"));
	add_kb(group, GEANY_KEYS_MENU_PRINT, cb_func_menu_print,
		GDK_p, GDK_CONTROL_MASK, "menu_print", _("Print"));
	add_kb(group, GEANY_KEYS_MENU_CLOSE, cb_func_file_action,
		GDK_w, GDK_CONTROL_MASK, "menu_close", _("Close"));
	add_kb(group, GEANY_KEYS_MENU_CLOSEALL, cb_func_file_action,
		GDK_w, GDK_CONTROL_MASK | GDK_SHIFT_MASK, "menu_closeall", _("Close all"));
	add_kb(group, GEANY_KEYS_MENU_RELOADFILE, cb_func_file_action,
		GDK_r, GDK_CONTROL_MASK, "menu_reloadfile", _("Reload file"));
	add_kb(group, GEANY_KEYS_MENU_PROJECTPROPERTIES, cb_func_file_action,
		0, 0, "project_properties", _("Project properties"));

	group = ADD_KB_GROUP(EDIT, _("Edit menu"));

	add_kb(group, GEANY_KEYS_MENU_UNDO, cb_func_menu_undo,
		GDK_z, GDK_CONTROL_MASK, "menu_undo", _("Undo"));
	add_kb(group, GEANY_KEYS_MENU_REDO, cb_func_menu_redo,
		GDK_y, GDK_CONTROL_MASK, "menu_redo", _("Redo"));
	add_kb(group, GEANY_KEYS_MENU_CUT, cb_func_clipboard,
		GDK_x, GDK_CONTROL_MASK, "menu_cut", _("Cut"));
	add_kb(group, GEANY_KEYS_MENU_COPY, cb_func_clipboard,
		GDK_c, GDK_CONTROL_MASK, "menu_copy", _("Copy"));
	add_kb(group, GEANY_KEYS_MENU_PASTE, cb_func_clipboard,
		GDK_v, GDK_CONTROL_MASK, "menu_paste", _("Paste"));
	add_kb(group, GEANY_KEYS_MENU_SELECTALL, cb_func_menu_selectall,
		GDK_a, GDK_CONTROL_MASK, "menu_selectall", _("Select All"));
	add_kb(group, GEANY_KEYS_MENU_INSERTDATE, cb_func_menu_insert_date,
		GDK_d, GDK_SHIFT_MASK | GDK_MOD1_MASK, "menu_insert_date", _("Insert date"));
	add_kb(group, GEANY_KEYS_MENU_PREFERENCES, cb_func_menu_preferences,
		GDK_p, GDK_CONTROL_MASK | GDK_MOD1_MASK, "menu_preferences", _("Preferences"));

	group = ADD_KB_GROUP(SEARCH, _("Search menu"));

	add_kb(group, GEANY_KEYS_MENU_FIND, cb_func_menu_search,
		GDK_f, GDK_CONTROL_MASK, "menu_find", _("Find"));
	add_kb(group, GEANY_KEYS_MENU_FINDNEXT, cb_func_menu_search,
		GDK_g, GDK_CONTROL_MASK, "menu_findnext", _("Find Next"));
	add_kb(group, GEANY_KEYS_MENU_FINDPREVIOUS, cb_func_menu_search,
		GDK_g, GDK_CONTROL_MASK | GDK_SHIFT_MASK, "menu_findprevious", _("Find Previous"));
	add_kb(group, GEANY_KEYS_MENU_FINDNEXTSEL, cb_func_menu_search,
		0, 0, "menu_findnextsel", _("Find Next Selection"));
	add_kb(group, GEANY_KEYS_MENU_FINDPREVSEL, cb_func_menu_search,
		0, 0, "menu_findprevsel", _("Find Previous Selection"));
	add_kb(group, GEANY_KEYS_MENU_REPLACE, cb_func_menu_search,
		GDK_h, GDK_CONTROL_MASK, "menu_replace", _("Replace"));
	add_kb(group, GEANY_KEYS_MENU_FINDINFILES, cb_func_menu_search, GDK_f,
		GDK_CONTROL_MASK | GDK_SHIFT_MASK, "menu_findinfiles", _("Find in Files"));
	add_kb(group, GEANY_KEYS_MENU_NEXTMESSAGE, cb_func_menu_search,
		0, 0, "menu_nextmessage", _("Next Message"));
	add_kb(group, GEANY_KEYS_MENU_GOTOLINE, cb_func_menu_search,
		GDK_l, GDK_CONTROL_MASK, "menu_gotoline", _("Go to Line"));

	group = ADD_KB_GROUP(VIEW, _("View menu"));

	add_kb(group, GEANY_KEYS_MENU_TOGGLEALL, cb_func_menu_toggle_all,
		0, 0, "menu_toggleall", _("Toggle All Additional Widgets"));
	add_kb(group, GEANY_KEYS_MENU_FULLSCREEN, cb_func_menu_fullscreen,
		GDK_F11, 0, "menu_fullscreen", _("Fullscreen"));
	add_kb(group, GEANY_KEYS_MENU_MESSAGEWINDOW, cb_func_menu_messagewindow,
		0, 0, "menu_messagewindow", _("Toggle Messages Window"));
	add_kb(group, GEANY_KEYS_MENU_SIDEBAR, cb_func_toggle_sidebar,
		0, 0, "toggle_sidebar", _("Toggle Sidebar"));
	add_kb(group, GEANY_KEYS_MENU_ZOOMIN, cb_func_menu_zoomin,
		GDK_plus, GDK_CONTROL_MASK, "menu_zoomin", _("Zoom In"));
	add_kb(group, GEANY_KEYS_MENU_ZOOMOUT, cb_func_menu_zoomout,
		GDK_minus, GDK_CONTROL_MASK, "menu_zoomout", _("Zoom Out"));

	group = ADD_KB_GROUP(DOCUMENT, _("Document menu"));

	add_kb(group, GEANY_KEYS_MENU_REPLACETABS, cb_func_menu_replacetabs,
		0, 0, "menu_replacetabs", _("Replace tabs by space"));
	add_kb(group, GEANY_KEYS_MENU_FOLDALL, cb_func_menu_foldall,
		0, 0, "menu_foldall", _("Fold all"));
	add_kb(group, GEANY_KEYS_MENU_UNFOLDALL, cb_func_menu_unfoldall,
		0, 0, "menu_unfoldall", _("Unfold all"));
	add_kb(group, GEANY_KEYS_RELOADTAGLIST, cb_func_reloadtaglist,
		GDK_r, GDK_SHIFT_MASK | GDK_CONTROL_MASK, "reloadtaglist", _("Reload symbol list"));

	group = ADD_KB_GROUP(BUILD, _("Build menu"));

	add_kb(group, GEANY_KEYS_BUILD_COMPILE, cb_func_build_action,
		GDK_F8, 0, "build_compile", _("Compile"));
	add_kb(group, GEANY_KEYS_BUILD_LINK, cb_func_build_action,
		GDK_F9, 0, "build_link", _("Build"));
	add_kb(group, GEANY_KEYS_BUILD_MAKE, cb_func_build_action,
		GDK_F9, GDK_SHIFT_MASK, "build_make", _("Make all"));
	add_kb(group, GEANY_KEYS_BUILD_MAKEOWNTARGET, cb_func_build_action,
		GDK_F9, GDK_SHIFT_MASK | GDK_CONTROL_MASK, "build_makeowntarget",
		_("Make custom target"));
	add_kb(group, GEANY_KEYS_BUILD_MAKEOBJECT, cb_func_build_action,
		0, 0, "build_makeobject", _("Make object"));
	add_kb(group, GEANY_KEYS_BUILD_NEXTERROR, cb_func_build_action,
		0, 0, "build_nexterror", _("Next error"));
	add_kb(group, GEANY_KEYS_BUILD_RUN, cb_func_build_action,
		GDK_F5, 0, "build_run", _("Run"));
	add_kb(group, GEANY_KEYS_BUILD_RUN2, cb_func_build_action,
		0, 0, "build_run2", _("Run (alternative command)"));
	add_kb(group, GEANY_KEYS_BUILD_OPTIONS, cb_func_build_action,
		0, 0, "build_options", _("Build options"));

	group = ADD_KB_GROUP(TOOLS, _("Tools menu"));

	add_kb(group, GEANY_KEYS_MENU_OPENCOLORCHOOSER, cb_func_menu_opencolorchooser,
		0, 0, "menu_opencolorchooser", _("Show Color Chooser"));
	add_kb(group, GEANY_KEYS_MENU_INSERTSPECIALCHARS, cb_func_menu_insert_specialchars,
		0, 0, "menu_insert_specialchars", _("Insert Special HTML Characters"));

	group = ADD_KB_GROUP(HELP, _("Help menu"));

	add_kb(group, GEANY_KEYS_MENU_HELP, cb_func_menu_help,
		GDK_F1, 0, "menu_help", _("Help"));

	group = ADD_KB_GROUP(FOCUS, _("Focus commands"));

	add_kb(group, GEANY_KEYS_SWITCH_EDITOR, cb_func_switch_editor,
		GDK_F2, 0, "switch_editor", _("Switch to Editor"));
	add_kb(group, GEANY_KEYS_SWITCH_SCRIBBLE, cb_func_switch_scribble,
		GDK_F6, 0, "switch_scribble", _("Switch to Scribble"));
	add_kb(group, GEANY_KEYS_SWITCH_VTE, cb_func_switch_vte,
		GDK_F4, 0, "switch_vte", _("Switch to VTE"));
	add_kb(group, GEANY_KEYS_SWITCH_SEARCH_BAR, cb_func_switch_search_bar,
		GDK_F7, 0, "switch_search_bar", _("Switch to Search Bar"));

	group = ADD_KB_GROUP(NOTEBOOK, _("Notebook tab commands"));

	add_kb(group, GEANY_KEYS_SWITCH_TABLEFT, cb_func_switch_tableft,
		GDK_Page_Up, GDK_CONTROL_MASK, "switch_tableft", _("Switch to left document"));
	add_kb(group, GEANY_KEYS_SWITCH_TABRIGHT, cb_func_switch_tabright,
		GDK_Page_Down, GDK_CONTROL_MASK, "switch_tabright", _("Switch to right document"));
	add_kb(group, GEANY_KEYS_SWITCH_TABLASTUSED, cb_func_switch_tablastused,
		GDK_Tab, GDK_CONTROL_MASK, "switch_tablastused", _("Switch to last used document"));
	add_kb(group, GEANY_KEYS_MOVE_TABLEFT, cb_func_move_tab,
		GDK_Page_Up, GDK_MOD1_MASK, "move_tableft", _("Move document left"));
	add_kb(group, GEANY_KEYS_MOVE_TABRIGHT, cb_func_move_tab,
		GDK_Page_Down, GDK_MOD1_MASK, "move_tabright", _("Move document right"));
	add_kb(group, GEANY_KEYS_MOVE_TABFIRST, cb_func_move_tab,
		0, 0, "move_tabfirst", _("Move document first"));
	add_kb(group, GEANY_KEYS_MOVE_TABLAST, cb_func_move_tab,
		0, 0, "move_tablast", _("Move document last"));

	group = ADD_KB_GROUP(EDITING, _("Editing commands"));

	add_kb(group, GEANY_KEYS_EDIT_DUPLICATELINE, cb_func_editing,
		GDK_d, GDK_CONTROL_MASK, "edit_duplicateline", _("Duplicate line or selection"));
	add_kb(group, GEANY_KEYS_EDIT_DELETELINE, cb_func_editing,
		GDK_k, GDK_CONTROL_MASK, "edit_deleteline", _("Delete current line(s)"));
	add_kb(group, GEANY_KEYS_EDIT_COPYLINE, cb_func_editing,
		GDK_c, GDK_CONTROL_MASK | GDK_SHIFT_MASK, "edit_copyline", _("Copy current line(s)"));
	add_kb(group, GEANY_KEYS_EDIT_CUTLINE, cb_func_editing,
		GDK_x, GDK_CONTROL_MASK | GDK_SHIFT_MASK, "edit_cutline", _("Cut current line(s)"));
	add_kb(group, GEANY_KEYS_EDIT_TRANSPOSELINE, cb_func_editing,
		GDK_t, GDK_CONTROL_MASK, "edit_transposeline", _("Transpose current line"));
	add_kb(group, GEANY_KEYS_EDIT_TOGGLECASE, cb_func_editing,
		GDK_u, GDK_CONTROL_MASK | GDK_MOD1_MASK, "edit_togglecase", _("Toggle Case of Selection"));
	add_kb(group, GEANY_KEYS_EDIT_COMMENTLINETOGGLE, cb_func_editing,
		GDK_e, GDK_CONTROL_MASK, "edit_commentlinetoggle", _("Toggle line commentation"));
	add_kb(group, GEANY_KEYS_EDIT_COMMENTLINE, cb_func_editing,
		0, 0, "edit_commentline", _("Comment line(s)"));
	add_kb(group, GEANY_KEYS_EDIT_UNCOMMENTLINE, cb_func_editing,
		0, 0, "edit_uncommentline", _("Uncomment line(s)"));
	add_kb(group, GEANY_KEYS_EDIT_INCREASEINDENT, cb_func_editing,
		GDK_i, GDK_CONTROL_MASK, "edit_increaseindent", _("Increase indent"));
	add_kb(group, GEANY_KEYS_EDIT_DECREASEINDENT, cb_func_editing,
		GDK_u, GDK_CONTROL_MASK, "edit_decreaseindent", _("Decrease indent"));
	add_kb(group, GEANY_KEYS_EDIT_INCREASEINDENTBYSPACE, cb_func_editing,
		0, 0, "edit_increaseindentbyspace", _("Increase indent by one space"));
	add_kb(group, GEANY_KEYS_EDIT_DECREASEINDENTBYSPACE, cb_func_editing,
		0, 0, "edit_decreaseindentbyspace", _("Decrease indent by one space"));
	add_kb(group, GEANY_KEYS_EDIT_AUTOINDENT, cb_func_editing,
		0, 0, "edit_autoindent", _("Smart line indent"));
	add_kb(group, GEANY_KEYS_EDIT_SENDTOCMD1, cb_func_editing,
		GDK_1, GDK_CONTROL_MASK, "edit_sendtocmd1", _("Send to Custom Command 1"));
	add_kb(group, GEANY_KEYS_EDIT_SENDTOCMD2, cb_func_editing,
		GDK_2, GDK_CONTROL_MASK, "edit_sendtocmd2", _("Send to Custom Command 2"));
	add_kb(group, GEANY_KEYS_EDIT_SENDTOCMD3, cb_func_editing,
		GDK_3, GDK_CONTROL_MASK, "edit_sendtocmd3", _("Send to Custom Command 3"));
	add_kb(group, GEANY_KEYS_EDIT_GOTOMATCHINGBRACE, cb_func_editing_global,
		GDK_b, GDK_CONTROL_MASK, "edit_gotomatchingbrace",
		_("Go to matching brace"));
	add_kb(group, GEANY_KEYS_EDIT_TOGGLEMARKER, cb_func_editing_global,
		GDK_m, GDK_CONTROL_MASK, "edit_togglemarker",
		_("Toggle marker"));
	add_kb(group, GEANY_KEYS_EDIT_GOTONEXTMARKER, cb_func_editing_global,
		GDK_period, GDK_CONTROL_MASK, "edit_gotonextmarker",
		_("Go to next marker"));
	add_kb(group, GEANY_KEYS_EDIT_GOTOPREVIOUSMARKER, cb_func_editing_global,
		GDK_comma, GDK_CONTROL_MASK, "edit_gotopreviousmarker",
		_("Go to previous marker"));
	add_kb(group, GEANY_KEYS_EDIT_SELECTWORD, cb_func_editing,
		GDK_w, GDK_SHIFT_MASK | GDK_MOD1_MASK, "edit_selectword", _("Select current word"));
	add_kb(group, GEANY_KEYS_EDIT_SELECTLINE, cb_func_editing,
		GDK_l, GDK_SHIFT_MASK | GDK_MOD1_MASK, "edit_selectline", _("Select current line(s)"));
	add_kb(group, GEANY_KEYS_EDIT_SELECTPARAGRAPH, cb_func_editing,
		GDK_p, GDK_SHIFT_MASK | GDK_MOD1_MASK, "edit_selectparagraph", _("Select current paragraph"));
	add_kb(group, GEANY_KEYS_EDIT_SCROLLTOLINE, cb_func_editing,
		GDK_l, GDK_SHIFT_MASK | GDK_CONTROL_MASK, "edit_scrolltoline", _("Scroll to current line"));
	add_kb(group, GEANY_KEYS_EDIT_SCROLLLINEUP, cb_func_editing,
		GDK_Up, GDK_MOD1_MASK, "edit_scrolllineup", _("Scroll up the view by one line"));
	add_kb(group, GEANY_KEYS_EDIT_SCROLLLINEDOWN, cb_func_editing,
		GDK_Down, GDK_MOD1_MASK, "edit_scrolllinedown", _("Scroll down the view by one line"));
	add_kb(group, GEANY_KEYS_EDIT_INSERTALTWHITESPACE, cb_func_editing,
		0, 0, "edit_insertwhitespace", _("Insert alternative whitespace"));

	/* popup menu editing commands */
	add_kb(group, GEANY_KEYS_POPUP_FINDUSAGE, cb_func_current_word,
		0, 0, "popup_findusage", _("Find Usage"));
	add_kb(group, GEANY_KEYS_POPUP_CONTEXTACTION, cb_func_current_word,
		0, 0, "popup_contextaction", _("Context Action"));

	group = ADD_KB_GROUP(TAGS, _("Tag commands"));

	add_kb(group, GEANY_KEYS_EDIT_AUTOCOMPLETE, cb_func_tags,
		GDK_space, GDK_CONTROL_MASK, "edit_autocomplete", _("Complete word"));
	add_kb(group, GEANY_KEYS_EDIT_CALLTIP, cb_func_tags,
		GDK_space, GDK_CONTROL_MASK | GDK_SHIFT_MASK, "edit_calltip", _("Show calltip"));
	add_kb(group, GEANY_KEYS_EDIT_MACROLIST, cb_func_tags,
		GDK_Return, GDK_CONTROL_MASK, "edit_macrolist", _("Show macro list"));
	add_kb(group, GEANY_KEYS_EDIT_COMPLETESNIPPET, NULL,	/* has special callback */
		GDK_Tab, 0, "edit_completesnippet", _("Complete snippet"));
	add_kb(group, GEANY_KEYS_EDIT_SUPPRESSSNIPPETCOMPLETION, cb_func_tags,
		0, 0, "edit_suppresssnippetcompletion", _("Suppress snippet completion"));
	add_kb(group, GEANY_KEYS_POPUP_GOTOTAGDEFINITION, cb_func_current_word,
		0, 0, "popup_gototagdefinition", _("Go to Tag Definition"));
	add_kb(group, GEANY_KEYS_POPUP_GOTOTAGDECLARATION, cb_func_current_word,
		0, 0, "popup_gototagdeclaration", _("Go to Tag Declaration"));

	group = ADD_KB_GROUP(OTHER, _("Other commands"));

	add_kb(group, GEANY_KEYS_NAV_BACK, cb_func_nav_back,
		0, 0, "nav_back", _("Navigate back a location"));
	add_kb(group, GEANY_KEYS_NAV_FORWARD, cb_func_nav_forward,
		0, 0, "nav_forward", _("Navigate forward a location"));
}


typedef void (*KBItemCallback) (KeyBindingGroup *group, KeyBinding *kb, gpointer user_data);

static void keybindings_foreach(KBItemCallback cb, gpointer user_data)
{
	gsize g, i;

	for (g = 0; g < keybinding_groups->len; g++)
	{
		KeyBindingGroup *group = g_ptr_array_index(keybinding_groups, g);

		for (i = 0; i < group->count; i++)
		{
			KeyBinding *kb = &group->keys[i];

			cb(group, kb, user_data);
		}
	}
}


static void get_keyfile_kb(KeyBindingGroup *group, KeyBinding *kb, gpointer user_data)
{
	GKeyFile *config = user_data;
	gchar *val;
	guint key;
	GdkModifierType mods;

	val = g_key_file_get_string(config, group->name, kb->name, NULL);
	if (val != NULL)
	{
		gtk_accelerator_parse(val, &key, &mods);
		kb->key = key;
		kb->mods = mods;
	}
	g_free(val);
}


static void load_user_kb(void)
{
	gchar *configfile = g_strconcat(app->configdir, G_DIR_SEPARATOR_S, "keybindings.conf", NULL);
	GKeyFile *config = g_key_file_new();

	/* now load user defined keys */
	if (g_key_file_load_from_file(config, configfile, G_KEY_FILE_KEEP_COMMENTS, NULL))
	{
		keybindings_foreach(get_keyfile_kb, config);
	}
	g_free(configfile);
	g_key_file_free(config);
}


void keybindings_init(void)
{
	keybinding_groups = g_ptr_array_sized_new(GEANY_KEYGROUP_COUNT);

	init_default_kb();
	load_user_kb();
	add_menu_accels();
}


#define GEANY_ADD_ACCEL(gkey, wid) \
	if (keys[(gkey)]->key != 0) \
		gtk_widget_add_accelerator( \
			lookup_widget(app->window, G_STRINGIFY(wid)), \
			"activate", accel_group, keys[(gkey)]->key, keys[(gkey)]->mods, \
			GTK_ACCEL_VISIBLE)

#define GEANY_ADD_POPUP_ACCEL(gkey, wid) \
	if (keys[(gkey)]->key != 0) \
		gtk_widget_add_accelerator( \
			lookup_widget(app->popup_menu, G_STRINGIFY(wid)), \
			"activate", accel_group, keys[(gkey)]->key, keys[(gkey)]->mods, \
			GTK_ACCEL_VISIBLE)

static void add_menu_accels()
{
#if 0 /* tmp */
	GtkAccelGroup *accel_group = gtk_accel_group_new();

	/* apply the settings */
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_OPENSELECTED, menu_open_selected_file1);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_SAVEALL, menu_save_all1);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_SAVEAS, menu_save_as1);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_PRINT, print1);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_CLOSE, menu_close1);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_CLOSEALL, menu_close_all1);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_RELOADFILE, menu_reload1);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_UNDO, menu_undo2);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_REDO, menu_redo2);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_SELECTALL, menu_select_all1);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_INSERTDATE, insert_date_custom1);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_PREFERENCES, preferences1);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_HELP, help1);

	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_FIND, find1);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_FINDNEXT, find_next1);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_FINDPREVIOUS, find_previous1);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_FINDNEXTSEL, find_nextsel1);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_FINDPREVSEL, find_prevsel1);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_REPLACE, replace1);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_FINDINFILES, find_in_files1);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_NEXTMESSAGE, next_message1);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_GOTOLINE, go_to_line1);

	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_TOGGLEALL, menu_toggle_all_additional_widgets1);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_FULLSCREEN, menu_fullscreen1);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_MESSAGEWINDOW, menu_show_messages_window1);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_SIDEBAR, menu_show_sidebar1);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_ZOOMIN, menu_zoom_in1);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_ZOOMOUT, menu_zoom_out1);

	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_REPLACETABS, menu_replace_tabs);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_FOLDALL, menu_fold_all1);
	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_UNFOLDALL, menu_unfold_all1);

	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_PROJECTPROPERTIES, project_properties1);

	GEANY_ADD_ACCEL(GEANY_KEYS_MENU_OPENCOLORCHOOSER, menu_choose_color1);
	/*GEANY_ADD_ACCEL(GEANY_KEYS_MENU_INSERTSPECIALCHARS, menu_insert_special_chars1);*/

	GEANY_ADD_ACCEL(GEANY_KEYS_EDIT_TOGGLECASE, menu_toggle_case2);
	GEANY_ADD_ACCEL(GEANY_KEYS_EDIT_COMMENTLINE, menu_comment_line1);
	GEANY_ADD_ACCEL(GEANY_KEYS_EDIT_UNCOMMENTLINE, menu_uncomment_line1);
	GEANY_ADD_ACCEL(GEANY_KEYS_EDIT_COMMENTLINETOGGLE, menu_toggle_line_commentation1);
	GEANY_ADD_ACCEL(GEANY_KEYS_EDIT_DUPLICATELINE, menu_duplicate_line1);
	GEANY_ADD_ACCEL(GEANY_KEYS_EDIT_INCREASEINDENT, menu_increase_indent1);
	GEANY_ADD_ACCEL(GEANY_KEYS_EDIT_DECREASEINDENT, menu_decrease_indent1);

	GEANY_ADD_POPUP_ACCEL(GEANY_KEYS_MENU_UNDO, undo1);
	GEANY_ADD_POPUP_ACCEL(GEANY_KEYS_MENU_REDO, redo1);
	GEANY_ADD_POPUP_ACCEL(GEANY_KEYS_MENU_SELECTALL, menu_select_all2);
	GEANY_ADD_POPUP_ACCEL(GEANY_KEYS_MENU_INSERTDATE, insert_date_custom2);
	GEANY_ADD_POPUP_ACCEL(GEANY_KEYS_MENU_OPENSELECTED, menu_open_selected_file2);
	GEANY_ADD_POPUP_ACCEL(GEANY_KEYS_POPUP_FINDUSAGE, find_usage1);
	GEANY_ADD_POPUP_ACCEL(GEANY_KEYS_POPUP_GOTOTAGDEFINITION, goto_tag_definition1);
	GEANY_ADD_POPUP_ACCEL(GEANY_KEYS_POPUP_GOTOTAGDECLARATION, goto_tag_declaration1);
	GEANY_ADD_POPUP_ACCEL(GEANY_KEYS_POPUP_CONTEXTACTION, context_action1);
	GEANY_ADD_POPUP_ACCEL(GEANY_KEYS_MENU_GOTOLINE, go_to_line);
	GEANY_ADD_POPUP_ACCEL(GEANY_KEYS_EDIT_TOGGLECASE, toggle_case1);
	GEANY_ADD_POPUP_ACCEL(GEANY_KEYS_EDIT_COMMENTLINE, menu_comment_line2);
	GEANY_ADD_POPUP_ACCEL(GEANY_KEYS_EDIT_UNCOMMENTLINE, menu_uncomment_line2);
	GEANY_ADD_POPUP_ACCEL(GEANY_KEYS_EDIT_COMMENTLINETOGGLE, menu_toggle_line_commentation2);
	GEANY_ADD_POPUP_ACCEL(GEANY_KEYS_EDIT_DUPLICATELINE, menu_duplicate_line2);
	GEANY_ADD_POPUP_ACCEL(GEANY_KEYS_EDIT_INCREASEINDENT, menu_increase_indent2);
	GEANY_ADD_POPUP_ACCEL(GEANY_KEYS_EDIT_DECREASEINDENT, menu_decrease_indent2);

	/* the build menu items are set if the build menus are created */

	gtk_window_add_accel_group(GTK_WINDOW(app->window), accel_group);
#endif
}


static void set_keyfile_kb(KeyBindingGroup *group, KeyBinding *kb, gpointer user_data)
{
	GKeyFile *config = user_data;
	gchar *val;

	val = gtk_accelerator_name(kb->key, kb->mods);
	g_key_file_set_string(config, group->name, kb->name, val);
	g_free(val);
}


/* just write the content of the keys array to the config file */
void keybindings_write_to_file(void)
{
	gchar *configfile = g_strconcat(app->configdir, G_DIR_SEPARATOR_S, "keybindings.conf", NULL);
	gchar *data;
	GKeyFile *config = g_key_file_new();

 	/* add comment if the file is newly created */
	if (! g_key_file_load_from_file(config, configfile, G_KEY_FILE_KEEP_COMMENTS, NULL))
	{
		g_key_file_set_comment(config, NULL, NULL, "Keybindings for Geany\nThe format looks like \"<Control>a\" or \"<Shift><Alt>F1\".\nBut you can also change the keys in Geany's preferences dialog.", NULL);
	}

	keybindings_foreach(set_keyfile_kb, config);

	/* write the file */
	data = g_key_file_to_data(config, NULL, NULL);
	utils_write_file(configfile, data);

	g_free(data);
	g_free(configfile);
	g_key_file_free(config);
}


void keybindings_free(void)
{
	g_ptr_array_free(keybinding_groups, TRUE);
}


static void get_shortcut_labels_text(GString **text_names_str, GString **text_keys_str)
{
#if 0
	guint i;
	GString *text_names = g_string_sized_new(600);
	GString *text_keys = g_string_sized_new(600);

	*text_names_str = text_names;
	*text_keys_str = text_keys;

	for (i = 0; i < GEANY_MAX_KEYS; i++)
	{
		gchar *shortcut;

		if (keys[i]->section != NULL)
		{
			if (i == GEANY_KEYS_MENU_NEW)
			{
				g_string_append_printf(text_names, "<b>%s</b>\n", keys[i]->section);
				g_string_append(text_keys, "\n");
			}
			else
			{
				g_string_append_printf(text_names, "\n<b>%s</b>\n", keys[i]->section);
				g_string_append(text_keys, "\n\n");
			}
		}

		shortcut = gtk_accelerator_get_label(keys[i]->key, keys[i]->mods);
		g_string_append(text_names, keys[i]->label);
		g_string_append(text_names, "\n");
		g_string_append(text_keys, shortcut);
		g_string_append(text_keys, "\n");
		g_free(shortcut);
	}
#endif
}


void keybindings_show_shortcuts()
{
	GtkWidget *dialog, *hbox, *label1, *label2, *label3, *swin, *vbox;
	GString *text_names;
	GString *text_keys;
	gint height, response;
return;/* tmp */
	dialog = gtk_dialog_new_with_buttons(_("Keyboard Shortcuts"), GTK_WINDOW(app->window),
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_STOCK_EDIT, GTK_RESPONSE_APPLY,
				GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL, NULL);
	vbox = ui_dialog_vbox_new(GTK_DIALOG(dialog));
	gtk_box_set_spacing(GTK_BOX(vbox), 6);
	gtk_widget_set_name(dialog, "GeanyDialog");

	height = GEANY_WINDOW_MINIMAL_HEIGHT;
	gtk_window_set_default_size(GTK_WINDOW(dialog), height * 8 / 10, height);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);

	label3 = gtk_label_new(_("The following keyboard shortcuts are configurable:"));
	gtk_misc_set_alignment(GTK_MISC(label3), 0, 0.5);

	hbox = gtk_hbox_new(FALSE, 6);

	label1 = gtk_label_new(NULL);

	label2 = gtk_label_new(NULL);

	get_shortcut_labels_text(&text_names, &text_keys);

	gtk_label_set_markup(GTK_LABEL(label1), text_names->str);
	gtk_label_set_text(GTK_LABEL(label2), text_keys->str);

	g_string_free(text_names, TRUE);
	g_string_free(text_keys, TRUE);

	gtk_container_add(GTK_CONTAINER(hbox), label1);
	gtk_container_add(GTK_CONTAINER(hbox), label2);

	swin = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC,
		GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swin), hbox);

	gtk_box_pack_start(GTK_BOX(vbox), label3, FALSE, FALSE, 6);
	gtk_box_pack_start(GTK_BOX(vbox), swin, TRUE, TRUE, 0);

	gtk_widget_show_all(dialog);
	response = gtk_dialog_run(GTK_DIALOG(dialog));
	if (response == GTK_RESPONSE_APPLY)
	{
		GtkWidget *wid;

		prefs_show_dialog();
		/* select the KB page */
		wid = lookup_widget(ui_widgets.prefs_dialog, "frame22");
		if (wid != NULL)
		{
			GtkNotebook *nb = GTK_NOTEBOOK(lookup_widget(ui_widgets.prefs_dialog, "notebook2"));

			if (nb != NULL)
				gtk_notebook_set_current_page(nb, gtk_notebook_page_num(nb, wid));
		}
	}

	gtk_widget_destroy(dialog);
}


static gboolean check_fixed_kb(guint keyval, guint state)
{
	/* check alt-0 to alt-9 for setting current notebook page */
	if (state & GDK_MOD1_MASK && keyval >= GDK_0 && keyval <= GDK_9)
	{
		gint page = keyval - GDK_0 - 1;
		gint npages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(app->notebook));

		/* alt-0 is for the rightmost tab */
		if (keyval == GDK_0)
			page = npages - 1;
		/* invert the order if tabs are added on the other side */
		if (swap_alt_tab_order && ! prefs.tab_order_ltr)
			page = (npages - 1) - page;

		gtk_notebook_set_current_page(GTK_NOTEBOOK(app->notebook), page);
		return TRUE;
	}
	if (keyval == GDK_Page_Up || keyval == GDK_Page_Down)
	{
		/* switch to first or last document */
		if (state == (GDK_CONTROL_MASK | GDK_SHIFT_MASK))
		{
			if (keyval == GDK_Page_Up)
				gtk_notebook_set_current_page(GTK_NOTEBOOK(app->notebook), 0);
			if (keyval == GDK_Page_Down)
				gtk_notebook_set_current_page(GTK_NOTEBOOK(app->notebook),
					gtk_notebook_get_n_pages(GTK_NOTEBOOK(app->notebook)) - 1);
			return TRUE;
		}
	}
	return FALSE;
}


/* We have a special case for GEANY_KEYS_EDIT_COMPLETESNIPPET, because we need to
 * return FALSE if no completion occurs, so the tab or space is handled normally. */
static gboolean check_snippet_completion(guint keyval, guint state)
{
return FALSE; /* tmp */
#if 0
	const guint i = GEANY_KEYS_EDIT_COMPLETESNIPPET;
	if (keys[i]->key == keyval && keys[i]->mods == state)
	{
		gint idx = document_get_cur_idx();
		GtkWidget *focusw = gtk_window_get_focus(GTK_WINDOW(app->window));

		/* keybinding only valid when scintilla widget has focus */
		if (DOC_IDX_VALID(idx) && focusw == GTK_WIDGET(doc_list[idx].sci))
		{
			ScintillaObject *sci = doc_list[idx].sci;
			gint pos = sci_get_current_position(sci);

			if (editor_prefs.complete_snippets)
				return editor_complete_snippet(idx, pos);
		}
	}
	return FALSE;
#endif
}


#ifdef HAVE_VTE
static gboolean set_sensitive(gpointer widget)
{
	gtk_widget_set_sensitive(GTK_WIDGET(widget), TRUE);
	return FALSE;
}


static gboolean check_vte(GdkModifierType state, guint keyval)
{
	guint i;
	GtkWidget *widget;

	if (! vc->enable_bash_keys)
		return FALSE;
	if (gtk_window_get_focus(GTK_WINDOW(app->window)) != vc->vte)
		return FALSE;
	/* prevent menubar flickering: */
	if (state == GDK_SHIFT_MASK && (keyval >= GDK_a && keyval <= GDK_z))
		return FALSE;
	if (state == 0 && (keyval < GDK_F1 || keyval > GDK_F35))	/* e.g. backspace */
		return FALSE;

	/* make focus commands override any bash commands */
	for (i = 0; i < GEANY_KEYS_FOCUS_COUNT; i++)
	{
		KeyBinding *kb = keybindings_lookup_item(GEANY_KEYGROUP_FOCUS, i);

		if (state == kb->mods && keyval == kb->key)
			return FALSE;
	}

	/* Temporarily disable the menus to prevent conflicting menu accelerators
	 * from overriding the VTE bash shortcuts.
	 * Ideally we would just somehow disable the menubar without redrawing it,
	 * but maybe that's not possible. */
	widget = lookup_widget(app->window, "menubar1");
	gtk_widget_set_sensitive(widget, FALSE);
	g_idle_add(&set_sensitive, (gpointer) widget);
	widget = app->popup_menu;
	gtk_widget_set_sensitive(widget, FALSE);
	g_idle_add(&set_sensitive, (gpointer) widget);
	return TRUE;
}
#endif


/* central keypress event handler, almost all keypress events go to this function */
gboolean keybindings_got_event(GtkWidget *widget, GdkEventKey *ev, gpointer user_data)
{
	guint state, keyval;
	gsize g, i;

	if (ev->keyval == 0)
		return FALSE;

	keyval = ev->keyval;
    state = ev->state & GEANY_KEYS_MODIFIER_MASK;

	/* hack to get around that CTRL+Shift+r results in GDK_R not GDK_r */
	if ((ev->state & GDK_SHIFT_MASK) || (ev->state & GDK_LOCK_MASK))
		if (keyval >= GDK_A && keyval <= GDK_Z)
			keyval += GDK_a - GDK_A;

	/*geany_debug("%d (%d) %d (%d)", keyval, ev->keyval, state, ev->state);*/

	/* special cases */
#ifdef HAVE_VTE
	if (vte_info.have_vte && check_vte(state, keyval))
		return FALSE;
#endif
	if (check_snippet_completion(keyval, state))
		return TRUE;

	for (g = 0; g < keybinding_groups->len; g++)
	{
		KeyBindingGroup *group = g_ptr_array_index(keybinding_groups, g);

		for (i = 0; i < group->count; i++)
		{
			KeyBinding *kb = &group->keys[i];

			if (keyval == kb->key && state == kb->mods)
			{
				if (kb->cb_func == NULL)
					return FALSE;	/* ignore the keybinding */

				/* call the corresponding callback function for this shortcut */
				kb->cb_func(i);
				return TRUE;
			}
		}
	}
	/* fixed keybindings can be overridden by user bindings, so check them last */
	if (check_fixed_kb(keyval, state))
		return TRUE;
	return FALSE;
}


KeyBinding *keybindings_lookup_item(guint group_id, guint cmd_id)
{
	KeyBindingGroup *group;

	g_return_val_if_fail(group_id < GEANY_KEYGROUP_COUNT, NULL);	/* plugins can't use this */

	group = g_ptr_array_index(keybinding_groups, group_id);

	g_return_val_if_fail(group && cmd_id < group->count, NULL);

	return &group->keys[cmd_id];
}


/* Mimic a (built-in only) keybinding action */
void keybindings_send_command(gint group_id, gint cmd_id)
{
	KeyBinding *kb = keybindings_lookup_item(group_id, cmd_id);

	if (kb)
		kb->cb_func(cmd_id);
}


/* These are the callback functions, either each group or each shortcut has it's
 * own function. */


static void cb_func_file_action(guint key_id)
{
	switch (key_id)
	{
		case GEANY_KEYS_MENU_NEW:
			document_new_file(NULL, NULL, NULL);
			break;
		case GEANY_KEYS_MENU_OPEN:
			on_open1_activate(NULL, NULL);
			break;
		case GEANY_KEYS_MENU_OPENSELECTED:
			on_menu_open_selected_file1_activate(NULL, NULL);
			break;
		case GEANY_KEYS_MENU_SAVE:
			on_save1_activate(NULL, NULL);
			break;
		case GEANY_KEYS_MENU_SAVEAS:
			on_save_as1_activate(NULL, NULL);
			break;
		case GEANY_KEYS_MENU_SAVEALL:
			on_save_all1_activate(NULL, NULL);
			break;
		case GEANY_KEYS_MENU_CLOSE:
			on_close1_activate(NULL, NULL);
			break;
		case GEANY_KEYS_MENU_CLOSEALL:
			on_close_all1_activate(NULL, NULL);
			break;
		case GEANY_KEYS_MENU_RELOADFILE:
			on_toolbutton23_clicked(NULL, NULL);
			break;
		case GEANY_KEYS_MENU_PROJECTPROPERTIES:
			if (app->project)
				on_project_properties1_activate(NULL, NULL);
			break;
	}
}


static void cb_func_menu_print(G_GNUC_UNUSED guint key_id)
{
	on_print1_activate(NULL, NULL);
}

static void cb_func_menu_undo(G_GNUC_UNUSED guint key_id)
{
	on_undo1_activate(NULL, NULL);
}

static void cb_func_menu_redo(G_GNUC_UNUSED guint key_id)
{
	on_redo1_activate(NULL, NULL);
}

static void cb_func_menu_selectall(G_GNUC_UNUSED guint key_id)
{
	on_menu_select_all1_activate(NULL, NULL);
}

static void cb_func_menu_preferences(G_GNUC_UNUSED guint key_id)
{
	on_preferences1_activate(NULL, NULL);
}

static void cb_func_menu_help(G_GNUC_UNUSED guint key_id)
{
	on_help1_activate(NULL, NULL);
}

static void cb_func_menu_search(guint key_id)
{
	switch (key_id)
	{
		case GEANY_KEYS_MENU_FIND:
			on_find1_activate(NULL, NULL); break;
		case GEANY_KEYS_MENU_FINDNEXT:
			on_find_next1_activate(NULL, NULL); break;
		case GEANY_KEYS_MENU_FINDPREVIOUS:
			on_find_previous1_activate(NULL, NULL); break;
		case GEANY_KEYS_MENU_FINDPREVSEL:
			on_find_prevsel1_activate(NULL, NULL); break;
		case GEANY_KEYS_MENU_FINDNEXTSEL:
			on_find_nextsel1_activate(NULL, NULL); break;
		case GEANY_KEYS_MENU_REPLACE:
			on_replace1_activate(NULL, NULL); break;
		case GEANY_KEYS_MENU_FINDINFILES:
			on_find_in_files1_activate(NULL, NULL); break;
		case GEANY_KEYS_MENU_NEXTMESSAGE:
			on_next_message1_activate(NULL, NULL); break;
		case GEANY_KEYS_MENU_GOTOLINE:
			on_go_to_line1_activate(NULL, NULL); break;
	}
}

static void cb_func_menu_opencolorchooser(G_GNUC_UNUSED guint key_id)
{
	on_show_color_chooser1_activate(NULL, NULL);
}

static void cb_func_menu_fullscreen(G_GNUC_UNUSED guint key_id)
{
	GtkCheckMenuItem *c = GTK_CHECK_MENU_ITEM(lookup_widget(app->window, "menu_fullscreen1"));

	gtk_check_menu_item_set_active(c, ! gtk_check_menu_item_get_active(c));
}

static void cb_func_menu_messagewindow(G_GNUC_UNUSED guint key_id)
{
	GtkCheckMenuItem *c = GTK_CHECK_MENU_ITEM(lookup_widget(app->window, "menu_show_messages_window1"));

	gtk_check_menu_item_set_active(c, ! gtk_check_menu_item_get_active(c));
}

static void cb_func_menu_zoomin(G_GNUC_UNUSED guint key_id)
{
	on_zoom_in1_activate(NULL, NULL);
}

static void cb_func_menu_zoomout(G_GNUC_UNUSED guint key_id)
{
	on_zoom_out1_activate(NULL, NULL);
}

static void cb_func_menu_foldall(G_GNUC_UNUSED guint key_id)
{
	gint idx = document_get_cur_idx();
	if (idx == -1 || ! doc_list[idx].is_valid) return;
	document_fold_all(idx);
}

static void cb_func_menu_unfoldall(G_GNUC_UNUSED guint key_id)
{
	gint idx = document_get_cur_idx();
	if (idx == -1 || ! doc_list[idx].is_valid) return;
	document_unfold_all(idx);
}

static void cb_func_build_action(guint key_id)
{
	gint idx = document_get_cur_idx();
	GtkWidget *item;
	filetype *ft;
	BuildMenuItems *menu_items;

	if (! DOC_IDX_VALID(idx)) return;

	ft = doc_list[idx].file_type;
	if (! ft) return;
	menu_items = build_get_menu_items(ft->id);

	switch (key_id)
	{
		case GEANY_KEYS_BUILD_COMPILE:
			item = menu_items->item_compile;
			break;
		case GEANY_KEYS_BUILD_LINK:
			item = menu_items->item_link;
			break;
		case GEANY_KEYS_BUILD_MAKE:
			item = menu_items->item_make_all;
			break;
		case GEANY_KEYS_BUILD_MAKEOWNTARGET:
			item = menu_items->item_make_custom;
			break;
		case GEANY_KEYS_BUILD_MAKEOBJECT:
			item = menu_items->item_make_object;
			break;
		case GEANY_KEYS_BUILD_NEXTERROR:
			item = menu_items->item_next_error;
			break;
		case GEANY_KEYS_BUILD_RUN:
			item = menu_items->item_exec;
			break;
		case GEANY_KEYS_BUILD_RUN2:
			item = menu_items->item_exec2;
			break;
		case GEANY_KEYS_BUILD_OPTIONS:
			item = menu_items->item_set_args;
			break;
		default:
			item = NULL;
	}
	/* Note: For Build menu items it's OK (at the moment) to assume they are in the correct
	 * sensitive state, but some other menus don't update the sensitive status until
	 * they are redrawn. */
	if (item && GTK_WIDGET_IS_SENSITIVE(item))
		gtk_menu_item_activate(GTK_MENU_ITEM(item));
}

static void cb_func_reloadtaglist(G_GNUC_UNUSED guint key_id)
{
	gint idx = document_get_cur_idx();
	if (idx == -1 || ! doc_list[idx].is_valid) return;
	document_update_tag_list(idx, TRUE);
}


/* common function for keybindings using current word */
static void cb_func_current_word(guint key_id)
{
	gint idx = document_get_cur_idx();
	gint pos;

	if (idx == -1 || ! doc_list[idx].is_valid) return;

	pos = sci_get_current_position(doc_list[idx].sci);

	editor_find_current_word(doc_list[idx].sci, pos,
		editor_info.current_word, GEANY_MAX_WORD_LENGTH, NULL);

	if (*editor_info.current_word == 0)
		utils_beep();
	else
		switch (key_id)
		{
			case GEANY_KEYS_POPUP_FINDUSAGE:
				on_find_usage1_activate(NULL, NULL);
				break;
			case GEANY_KEYS_POPUP_GOTOTAGDEFINITION:
				symbols_goto_tag(editor_info.current_word, TRUE);
				break;
			case GEANY_KEYS_POPUP_GOTOTAGDECLARATION:
				symbols_goto_tag(editor_info.current_word, FALSE);
				break;
			case GEANY_KEYS_POPUP_CONTEXTACTION:
				on_context_action1_activate(GTK_MENU_ITEM(lookup_widget(app->popup_menu,
					"context_action1")), NULL);
				break;
		}
}


static void cb_func_switch_editor(G_GNUC_UNUSED guint key_id)
{
	gint idx = document_get_cur_idx();
	if (idx == -1 || ! doc_list[idx].is_valid) return;
	gtk_widget_grab_focus(GTK_WIDGET(doc_list[idx].sci));
}

static void cb_func_switch_scribble(G_GNUC_UNUSED guint key_id)
{
	msgwin_switch_tab(MSG_SCRATCH, TRUE);
}

static void cb_func_switch_search_bar(G_GNUC_UNUSED guint key_id)
{
	if (prefs.toolbar_visible && prefs.toolbar_show_search)
		gtk_widget_grab_focus(lookup_widget(app->window, "entry1"));
}

static void cb_func_switch_vte(G_GNUC_UNUSED guint key_id)
{
	msgwin_switch_tab(MSG_VTE, TRUE);
}

static void cb_func_switch_tableft(G_GNUC_UNUSED guint key_id)
{
	utils_switch_document(LEFT);
}

static void cb_func_switch_tabright(G_GNUC_UNUSED guint key_id)
{
	utils_switch_document(RIGHT);
}

static void cb_func_switch_tablastused(G_GNUC_UNUSED guint key_id)
{
	gint last_doc_idx = callbacks_data.last_doc_idx;

	if (DOC_IDX_VALID(last_doc_idx))
		gtk_notebook_set_current_page(GTK_NOTEBOOK(app->notebook),
			document_get_notebook_page(last_doc_idx));
}

/* move document left/right/first/last */
static void cb_func_move_tab(guint key_id)
{
	gint idx = document_get_cur_idx();
	GtkWidget *sci = GTK_WIDGET(doc_list[idx].sci);
	GtkNotebook *nb = GTK_NOTEBOOK(app->notebook);
	gint cur_page = gtk_notebook_get_current_page(nb);

	if (! DOC_IDX_VALID(idx))
		return;

	if (key_id == GEANY_KEYS_MOVE_TABLEFT)
	{
		gtk_notebook_reorder_child(nb, sci, cur_page - 1);	/* notebook wraps around by default */
	}
	else if (key_id == GEANY_KEYS_MOVE_TABRIGHT)
	{
		gint npage = cur_page + 1;

		if (npage == gtk_notebook_get_n_pages(nb))
			npage = 0;	/* wraparound */
		gtk_notebook_reorder_child(nb, sci, npage);
	}
	else if (key_id == GEANY_KEYS_MOVE_TABFIRST)
	{
		gtk_notebook_reorder_child(nb, sci, (prefs.tab_order_ltr) ? 0 : -1);
	}
	else if (key_id == GEANY_KEYS_MOVE_TABLAST)
	{
		gtk_notebook_reorder_child(nb, sci, (prefs.tab_order_ltr) ? -1 : 0);
	}
	return;
}

static void cb_func_toggle_sidebar(G_GNUC_UNUSED guint key_id)
{
	on_menu_show_sidebar1_toggled(NULL, NULL);
}


static void cb_func_menu_toggle_all(G_GNUC_UNUSED guint key_id)
{
	on_menu_toggle_all_additional_widgets1_activate(NULL, NULL);
}


static void goto_matching_brace(gint idx)
{
	gint pos, new_pos;

	if (! DOC_IDX_VALID(idx)) return;

	pos = sci_get_current_position(doc_list[idx].sci);
	if (! utils_isbrace(sci_get_char_at(doc_list[idx].sci, pos), TRUE))
		pos--; /* set pos to the brace */

	new_pos = sci_find_bracematch(doc_list[idx].sci, pos);
	if (new_pos != -1)
	{	/* set the cursor at the brace */
		sci_set_current_position(doc_list[idx].sci, new_pos, FALSE);
		editor_display_current_line(idx, 0.5F);
	}
}


static void cb_func_clipboard(guint key_id)
{
	switch (key_id)
	{
		case GEANY_KEYS_MENU_CUT:
			on_cut1_activate(NULL, NULL);
			break;
		case GEANY_KEYS_MENU_COPY:
			on_copy1_activate(NULL, NULL);
			break;
		case GEANY_KEYS_MENU_PASTE:
			on_paste1_activate(NULL, NULL);
			break;
	}
}


/* Common function for editing keybindings that don't change any text, and are
 * useful even when sci doesn't have focus. */
static void cb_func_editing_global(guint key_id)
{
	gint idx = document_get_cur_idx();
	gint cur_line;

	if (! DOC_IDX_VALID(idx)) return;

	cur_line = sci_get_current_line(doc_list[idx].sci);

	switch (key_id)
	{
		case GEANY_KEYS_EDIT_GOTOMATCHINGBRACE:
			goto_matching_brace(idx);
			break;
		case GEANY_KEYS_EDIT_TOGGLEMARKER:
		{
			gboolean set = sci_is_marker_set_at_line(doc_list[idx].sci, cur_line, 1);

			sci_set_marker_at_line(doc_list[idx].sci, cur_line, ! set, 1);
			break;
		}
		case GEANY_KEYS_EDIT_GOTONEXTMARKER:
		{
			gint mline = sci_marker_next(doc_list[idx].sci, cur_line + 1, 1 << 1, TRUE);

			if (mline != -1)
			{
				sci_set_current_line(doc_list[idx].sci, mline);
				editor_display_current_line(idx, 0.5F);
			}
			break;
		}
		case GEANY_KEYS_EDIT_GOTOPREVIOUSMARKER:
		{
			gint mline = sci_marker_previous(doc_list[idx].sci, cur_line - 1, 1 << 1, TRUE);

			if (mline != -1)
			{
				sci_set_current_line(doc_list[idx].sci, mline);
				editor_display_current_line(idx, 0.5F);
			}
			break;
		}
	}
}


static void duplicate_lines(ScintillaObject *sci)
{
	if (sci_get_lines_selected(sci) > 1)
	{	/* ignore extra_line because of selecting lines from the line number column */
		editor_select_lines(sci, FALSE);
		sci_selection_duplicate(sci);
	}
	else if (sci_can_copy(sci))
		sci_selection_duplicate(sci);
	else
		sci_line_duplicate(sci);
}


static void delete_lines(ScintillaObject *sci)
{
	editor_select_lines(sci, TRUE); /* include last line (like cut lines, copy lines do) */
	sci_clear(sci);	/* (SCI_LINEDELETE only does 1 line) */
}


/* common function for editing keybindings, only valid when scintilla has focus. */
static void cb_func_editing(guint key_id)
{
	gint idx = document_get_cur_idx();
	GtkWidget *focusw = gtk_window_get_focus(GTK_WINDOW(app->window));

	/* edit keybindings only valid when scintilla widget has focus */
	if (! DOC_IDX_VALID(idx) || focusw != GTK_WIDGET(doc_list[idx].sci)) return;

	switch (key_id)
	{
		case GEANY_KEYS_EDIT_SCROLLTOLINE:
			editor_scroll_to_line(doc_list[idx].sci, -1, 0.5F);
			break;
		case GEANY_KEYS_EDIT_SCROLLLINEUP:
			sci_cmd(doc_list[idx].sci, SCI_LINESCROLLUP);
			break;
		case GEANY_KEYS_EDIT_SCROLLLINEDOWN:
			sci_cmd(doc_list[idx].sci, SCI_LINESCROLLDOWN);
			break;
		case GEANY_KEYS_EDIT_DUPLICATELINE:
			duplicate_lines(doc_list[idx].sci);
			break;
		case GEANY_KEYS_EDIT_DELETELINE:
			delete_lines(doc_list[idx].sci);
			break;
		case GEANY_KEYS_EDIT_COPYLINE:
			sci_cmd(doc_list[idx].sci, SCI_LINECOPY);
			break;
		case GEANY_KEYS_EDIT_CUTLINE:
			sci_cmd(doc_list[idx].sci, SCI_LINECUT);
			break;
		case GEANY_KEYS_EDIT_TRANSPOSELINE:
			sci_cmd(doc_list[idx].sci, SCI_LINETRANSPOSE);
			break;
		case GEANY_KEYS_EDIT_COMMENTLINETOGGLE:
			on_menu_toggle_line_commentation1_activate(NULL, NULL);
			break;
		case GEANY_KEYS_EDIT_COMMENTLINE:
			on_menu_comment_line1_activate(NULL, NULL);
			break;
		case GEANY_KEYS_EDIT_UNCOMMENTLINE:
			on_menu_uncomment_line1_activate(NULL, NULL);
			break;
		case GEANY_KEYS_EDIT_SELECTWORD:
			editor_select_word(doc_list[idx].sci);
			break;
		case GEANY_KEYS_EDIT_SELECTLINE:
			editor_select_lines(doc_list[idx].sci, FALSE);
			break;
		case GEANY_KEYS_EDIT_SELECTPARAGRAPH:
			editor_select_paragraph(doc_list[idx].sci);
			break;
		case GEANY_KEYS_EDIT_INSERTALTWHITESPACE:
			editor_insert_alternative_whitespace(idx);
			break;
		case GEANY_KEYS_EDIT_INCREASEINDENT:
			on_menu_increase_indent1_activate(NULL, NULL);
			break;
		case GEANY_KEYS_EDIT_DECREASEINDENT:
			on_menu_decrease_indent1_activate(NULL, NULL);
			break;
		case GEANY_KEYS_EDIT_INCREASEINDENTBYSPACE:
			editor_indentation_by_one_space(idx, -1, FALSE);
			break;
		case GEANY_KEYS_EDIT_DECREASEINDENTBYSPACE:
			editor_indentation_by_one_space(idx, -1, TRUE);
			break;
		case GEANY_KEYS_EDIT_AUTOINDENT:
			editor_auto_line_indentation(idx, -1);
			break;
		case GEANY_KEYS_EDIT_TOGGLECASE:
			on_toggle_case1_activate(NULL, NULL);
			break;
		case GEANY_KEYS_EDIT_SENDTOCMD1:
			if (ui_prefs.custom_commands && g_strv_length(ui_prefs.custom_commands) > 0)
				tools_execute_custom_command(idx, ui_prefs.custom_commands[0]);
			break;
		case GEANY_KEYS_EDIT_SENDTOCMD2:
			if (ui_prefs.custom_commands && g_strv_length(ui_prefs.custom_commands) > 1)
				tools_execute_custom_command(idx, ui_prefs.custom_commands[1]);
			break;
		case GEANY_KEYS_EDIT_SENDTOCMD3:
			if (ui_prefs.custom_commands && g_strv_length(ui_prefs.custom_commands) > 2)
				tools_execute_custom_command(idx, ui_prefs.custom_commands[2]);
			break;
	}
}


/* common function for tags keybindings, only valid when scintilla has focus. */
static void cb_func_tags(guint key_id)
{
	gint idx = document_get_cur_idx();
	GtkWidget *focusw = gtk_window_get_focus(GTK_WINDOW(app->window));

	/* tags keybindings only valid when scintilla widget has focus */
	if (! DOC_IDX_VALID(idx) || focusw != GTK_WIDGET(doc_list[idx].sci)) return;

	switch (key_id)
	{
		case GEANY_KEYS_EDIT_AUTOCOMPLETE:
			editor_start_auto_complete(idx, sci_get_current_position(doc_list[idx].sci), TRUE);
			break;
		case GEANY_KEYS_EDIT_CALLTIP:
			editor_show_calltip(idx, -1);
			break;
		case GEANY_KEYS_EDIT_MACROLIST:
			editor_show_macro_list(doc_list[idx].sci);
			break;
		case GEANY_KEYS_EDIT_SUPPRESSSNIPPETCOMPLETION:
		{
			KeyBinding *kb = keybindings_lookup_item(GEANY_KEYGROUP_TAGS,
				GEANY_KEYS_EDIT_COMPLETESNIPPET);

			switch (kb->key)
			{
				case GDK_space:
					sci_add_text(doc_list[idx].sci, " ");
					break;
				case GDK_Tab:
					sci_cmd(doc_list[idx].sci, SCI_TAB);
					break;
				default:
					break;
			}
			break;
		}
	}
}


static void cb_func_menu_replacetabs(G_GNUC_UNUSED guint key_id)
{
	on_replace_tabs_activate(NULL, NULL);
}

static void cb_func_menu_insert_date(G_GNUC_UNUSED guint key_id)
{
	gtk_menu_item_activate(GTK_MENU_ITEM(lookup_widget(app->window, "insert_date_custom1")));
}

static void cb_func_menu_insert_specialchars(G_GNUC_UNUSED guint key_id)
{
	/** TODO: add plugin keybinding support */
	/*on_menu_insert_special_chars1_activate(NULL, NULL);*/
}

static void cb_func_nav_back(G_GNUC_UNUSED guint key_id)
{
	navqueue_go_back();
}

static void cb_func_nav_forward(G_GNUC_UNUSED guint key_id)
{
	navqueue_go_forward();
}

