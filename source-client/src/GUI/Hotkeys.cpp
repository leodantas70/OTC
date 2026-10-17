/*
  The Forgotten Client
  Copyright (C) 2020 Saiyans King

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include "GUI_UTIL.h"
#include "../engine.h"
#include "../game.h"
#include "../GUI_Elements/GUI_Window.h"
#include "../GUI_Elements/GUI_Button.h"
#include "../GUI_Elements/GUI_Separator.h"
#include "../GUI_Elements/GUI_CheckBox.h"
#include "../GUI_Elements/GUI_Grouper.h"
#include "../GUI_Elements/GUI_Label.h"
#include "../GUI_Elements/GUI_ListBox.h"
#include "../GUI_Elements/GUI_TextBox.h"
#include "itemUI.h"

#include <memory>
#include <utility>

#define HOTKEYS_TITLE "Hotkey Options"
#define HOTKEYS_WIDTH 285
#define HOTKEYS_HEIGHT 438

#define HOTKEYS_CANCEL_EVENTID 1000
#define HOTKEYS_OK_EVENTID 1001
#define HOTKEYS_LISTBOX_EVENTID 1002
#define HOTKEYS_TEXTBOX_EVENTID 1003
#define HOTKEYS_SEND_AUTO_EVENTID 1004
#define HOTKEYS_SELECT_OBJECT_EVENTID 1005
#define HOTKEYS_CLEAR_OBJECT_EVENTID 1006
#define HOTKEYS_USE_YOURSELF_EVENTID 1007
#define HOTKEYS_USE_TARGET_EVENTID 1008
#define HOTKEYS_CROSSHAIRS_EVENTID 1009
#define HOTKEYS_OBJECT_DISPLAY_EVENTID 1010
#define HOTKEYS_ADD_KEY_BUTTON_EVENTID 1012
#define HOTKEYS_REMOVE_KEY_BUTTON_EVENTID 1013

#define HOTKEY_CAPTURE_CANCEL_EVENTID 1050

#define HOTKEY_CAPTURE_WIDTH 400
#define HOTKEY_CAPTURE_HEIGHT 148

#define HOTKEYS_LABEL_X 18
#define HOTKEYS_LABEL_COLOR 223

#define HOTKEYS_AVAILABLE_LABEL_Y 30
#define HOTKEYS_LIST_X 18
#define HOTKEYS_LIST_Y 45
#define HOTKEYS_LIST_W 252
#define HOTKEYS_LIST_H 113

#define HOTKEYS_MANAGE_BUTTON_Y 166

#define HOTKEYS_TEXT_LABEL_Y 203
#define HOTKEYS_TEXTBOX_X 18
#define HOTKEYS_TEXTBOX_Y 219
#define HOTKEYS_TEXTBOX_W 252
#define HOTKEYS_TEXTBOX_H 16

#define HOTKEYS_SEND_AUTO_X 18
#define HOTKEYS_SEND_AUTO_Y 249
#define HOTKEYS_SEND_AUTO_W 252
#define HOTKEYS_SEND_AUTO_H 22

#define HOTKEYS_OBJECT_LABEL_Y 283
#define HOTKEYS_OBJECT_BOX_X 18
#define HOTKEYS_OBJECT_BOX_Y 299
#define HOTKEYS_OBJECT_BOX_W 52
#define HOTKEYS_OBJECT_BOX_H 52

#define HOTKEYS_OBJECT_BUTTON_X 85
#define HOTKEYS_OBJECT_BUTTON_W GUI_UI_BUTTON_86PX_GRAY_UP_W
#define HOTKEYS_OBJECT_BUTTON_H GUI_UI_BUTTON_86PX_GRAY_UP_H
#define HOTKEYS_OBJECT_BUTTON_GAP 33

#define HOTKEYS_USE_BUTTON_X 184
#define HOTKEYS_USE_BUTTON_W GUI_UI_BUTTON_86PX_GRAY_UP_W
#define HOTKEYS_USE_BUTTON_H GUI_UI_BUTTON_86PX_GRAY_UP_H

extern Engine g_engine;
extern Game g_game;

struct ClassicHotkeySlot
{
	std::string name;
	SDL_Keycode keycode;
	Uint16 modifiers;
};

struct ClassicHotkeyEditState
{
	std::string text;
	bool sendAutomatically = false;
	Uint16 itemId = 0;
	Uint8 itemSubtype = 0;
	Uint8 itemUsageType = CLIENT_HOTKEY_ACTION_WITHCROSSHAIRS;
};

static const size_t CLASSIC_HOTKEY_MAX_SLOTS = 64;
static ClassicHotkeySlot classicHotkeySlots[CLASSIC_HOTKEY_MAX_SLOTS] =
{
	{"F1:", SDLK_F1, KMOD_NONE},
	{"F2:", SDLK_F2, KMOD_NONE},
	{"F3:", SDLK_F3, KMOD_NONE},
	{"F4:", SDLK_F4, KMOD_NONE},
	{"F5:", SDLK_F5, KMOD_NONE},
	{"F6:", SDLK_F6, KMOD_NONE},
	{"F7:", SDLK_F7, KMOD_NONE},
	{"F8:", SDLK_F8, KMOD_NONE},
	{"F9:", SDLK_F9, KMOD_NONE},
	{"F10:", SDLK_F10, KMOD_NONE},
	{"F11:", SDLK_F11, KMOD_NONE},
	{"F12:", SDLK_F12, KMOD_NONE}
};

static const size_t g_defaultClassicHotkeySlotCount = 12;
static size_t g_classicHotkeySlotCount = g_defaultClassicHotkeySlotCount;
static ClassicHotkeyEditState g_classicHotkeyEditStates[CLASSIC_HOTKEY_MAX_SLOTS];
static Sint32 g_selectedClassicHotkey = 0;
static std::vector<std::pair<SDL_Keycode, Uint16>> g_removedClassicHotkeys;
static bool g_waitingHotkeyCapture = false;
static SDL_Keycode g_preferredHotkeyKeycode = SDLK_UNKNOWN;
static Uint16 g_preferredHotkeyModifiers = KMOD_NONE;

void hotkey_Events(Uint32 event, Sint32 status);

static Uint16 normalizeHotkeyModifiers(Uint16 modifiers)
{
	Uint16 normalized = KMOD_NONE;
	if((modifiers & KMOD_CTRL) != 0)
		normalized |= KMOD_CTRL;
	if((modifiers & KMOD_SHIFT) != 0)
		normalized |= KMOD_SHIFT;
	if((modifiers & KMOD_ALT) != 0)
		normalized |= KMOD_ALT;
	return normalized;
}

static bool isModifierKey(SDL_Keycode keycode)
{
	return keycode == SDLK_LCTRL || keycode == SDLK_RCTRL ||
		keycode == SDLK_LSHIFT || keycode == SDLK_RSHIFT ||
		keycode == SDLK_LALT || keycode == SDLK_RALT ||
		keycode == SDLK_LGUI || keycode == SDLK_RGUI;
}

static std::string formatHotkeyName(SDL_Keycode keycode, Uint16 modifiers, bool trailingColon)
{
	std::string displayName;
	modifiers = normalizeHotkeyModifiers(modifiers);
	if((modifiers & KMOD_CTRL) != 0)
		displayName += "Ctrl+";
	if((modifiers & KMOD_SHIFT) != 0)
		displayName += "Shift+";
	if((modifiers & KMOD_ALT) != 0)
		displayName += "Alt+";
	displayName += SDL_GetKeyName(keycode);
	if(trailingColon)
		displayName += ':';
	return displayName;
}

class GUI_HotkeyItemBox : public GUI_Element
{
	public:
		GUI_HotkeyItemBox(iRect boxRect, Uint32 internalID = 0)
		{
			setRect(boxRect);
			m_internalID = internalID;
		}

		~GUI_HotkeyItemBox() = default;

		void setItemData(Uint16 itemId, Uint8 itemSubtype)
		{
			m_item.reset();
			if(itemId != 0)
				m_item.reset(ItemUI::createItemUI(itemId, itemSubtype));
		}

		void render()
		{
			auto& renderer = g_engine.getRender();
			renderer->drawPictureRepeat(GUI_UI_IMAGE, GUI_UI_ICON_HORIZONTAL_LINE_DARK_X, GUI_UI_ICON_HORIZONTAL_LINE_DARK_Y, GUI_UI_ICON_HORIZONTAL_LINE_DARK_W, GUI_UI_ICON_HORIZONTAL_LINE_DARK_H, m_tRect.x1, m_tRect.y1, m_tRect.x2, 1);
			renderer->drawPictureRepeat(GUI_UI_IMAGE, GUI_UI_ICON_VERTICAL_LINE_DARK_X, GUI_UI_ICON_VERTICAL_LINE_DARK_Y, GUI_UI_ICON_VERTICAL_LINE_DARK_W, GUI_UI_ICON_VERTICAL_LINE_DARK_H, m_tRect.x1, m_tRect.y1 + 1, 1, m_tRect.y2 - 1);
			renderer->drawPictureRepeat(GUI_UI_IMAGE, GUI_UI_ICON_HORIZONTAL_LINE_BRIGHT_X, GUI_UI_ICON_HORIZONTAL_LINE_BRIGHT_Y, GUI_UI_ICON_HORIZONTAL_LINE_BRIGHT_W, GUI_UI_ICON_HORIZONTAL_LINE_BRIGHT_H, m_tRect.x1 + 1, m_tRect.y1 + m_tRect.y2 - 1, m_tRect.x2 - 1, 1);
			renderer->drawPictureRepeat(GUI_UI_IMAGE, GUI_UI_ICON_VERTICAL_LINE_BRIGHT_X, GUI_UI_ICON_VERTICAL_LINE_BRIGHT_Y, GUI_UI_ICON_VERTICAL_LINE_BRIGHT_W, GUI_UI_ICON_VERTICAL_LINE_BRIGHT_H, m_tRect.x1 + m_tRect.x2 - 1, m_tRect.y1 + 1, 1, m_tRect.y2 - 2);
			renderer->fillRectangle(m_tRect.x1 + 1, m_tRect.y1 + 1, m_tRect.x2 - 2, m_tRect.y2 - 2, 64, 64, 64, 255);
			if(m_item)
				m_item->render(m_tRect.x1 + 10, m_tRect.y1 + 10, 32);
		}

	protected:
		std::unique_ptr<ItemUI> m_item;
};

static GUI_TextBox* getHotkeyTextBox(GUI_Window* window)
{
	return SDL_static_cast(GUI_TextBox*, window->getChild(HOTKEYS_TEXTBOX_EVENTID));
}

static GUI_CheckBox* getSendAutoCheckBox(GUI_Window* window)
{
	return SDL_static_cast(GUI_CheckBox*, window->getChild(HOTKEYS_SEND_AUTO_EVENTID));
}

static GUI_HotkeyItemBox* getHotkeyItemBox(GUI_Window* window)
{
	return SDL_static_cast(GUI_HotkeyItemBox*, window->getChild(HOTKEYS_OBJECT_DISPLAY_EVENTID));
}

static std::string trimHotkeyPart(const std::string& value)
{
	size_t first = value.find_first_not_of(" \t");
	if(first == std::string::npos)
		return std::string();
	size_t last = value.find_last_not_of(" \t");
	return value.substr(first, last - first + 1);
}

static bool parseCustomHotkey(const std::string& value, SDL_Keycode& keycode, Uint16& modifiers, std::string& displayName)
{
	std::vector<std::string> parts;
	size_t start = 0;
	for(;;)
	{
		size_t separator = value.find('+', start);
		parts.push_back(trimHotkeyPart(value.substr(start, separator == std::string::npos ? std::string::npos : separator - start)));
		if(separator == std::string::npos)
			break;
		start = separator + 1;
	}
	if(parts.empty() || parts.back().empty())
		return false;

	modifiers = KMOD_NONE;
	for(size_t i = 0; i + 1 < parts.size(); ++i)
	{
		if(SDL_strcasecmp(parts[i].c_str(), "Ctrl") == 0 || SDL_strcasecmp(parts[i].c_str(), "Control") == 0)
			modifiers |= KMOD_CTRL;
		else if(SDL_strcasecmp(parts[i].c_str(), "Shift") == 0)
			modifiers |= KMOD_SHIFT;
		else if(SDL_strcasecmp(parts[i].c_str(), "Alt") == 0)
			modifiers |= KMOD_ALT;
		else
			return false;
	}

	keycode = SDL_GetKeyFromName(parts.back().c_str());
	if(keycode == SDLK_UNKNOWN || keycode == SDLK_LCTRL || keycode == SDLK_RCTRL || keycode == SDLK_LSHIFT || keycode == SDLK_RSHIFT || keycode == SDLK_LALT || keycode == SDLK_RALT)
		return false;

	modifiers = normalizeHotkeyModifiers(modifiers);
	displayName = formatHotkeyName(keycode, modifiers, true);
	return true;
}

static Sint32 findClassicHotkeySlot(SDL_Keycode keycode, Uint16 modifiers)
{
	for(size_t i = 0; i < g_classicHotkeySlotCount; ++i)
	{
		if(classicHotkeySlots[i].keycode == keycode && classicHotkeySlots[i].modifiers == modifiers)
			return SDL_static_cast(Sint32, i);
	}
	return -1;
}

static const char* getHotkeyItemActionText(Uint8 usageType)
{
	switch(usageType)
	{
		case CLIENT_HOTKEY_ACTION_USEONYOURSELF: return "(use object on yourself)";
		case CLIENT_HOTKEY_ACTION_USEONTARGET: return "(use object on target)";
		case CLIENT_HOTKEY_ACTION_WITHCROSSHAIRS: return "(use object with crosshairs)";
		default: return "(use object)";
	}
}

static const char* getHotkeyItemActionColor(Uint8 usageType)
{
	switch(usageType)
	{
		case CLIENT_HOTKEY_ACTION_USEONYOURSELF: return "\x0E\x99\xFF\x99";
		case CLIENT_HOTKEY_ACTION_USEONTARGET: return "\x0E\xFF\x99\x99";
		case CLIENT_HOTKEY_ACTION_WITHCROSSHAIRS: return "\x0E\xFF\x99\x99";
		default: return "\x0E\xFF\x99\x99";
	}
}

static std::string getHotkeyListText(size_t index)
{
	// Keep the key name bright even while it has no action yet. Without an
	// explicit color the generic list box renders unselected rows dimmed,
	// which makes a newly added hotkey look disabled or missing.
	std::string text("\x0E\xFF\xFF\xFF");
	text += classicHotkeySlots[index].name;
	if(!g_classicHotkeyEditStates[index].text.empty())
	{
		text += " ";
		// Commands that are only placed in the chat input are intentionally
		// dimmed. Automatically sent commands keep the normal bright color.
		if(g_classicHotkeyEditStates[index].sendAutomatically)
			text += "\x0E\xFF\xFF\xFF";
		else
			text += "\x0E\x90\x90\x90";
		text += g_classicHotkeyEditStates[index].text;
	}
	else if(g_classicHotkeyEditStates[index].itemId != 0)
	{
		text += " ";
		text += getHotkeyItemActionColor(g_classicHotkeyEditStates[index].itemUsageType);
		text += getHotkeyItemActionText(g_classicHotkeyEditStates[index].itemUsageType);
	}
	text += "\x0F";
	return text;
}

static void refreshHotkeyList(GUI_Window* window)
{
	GUI_ListBox* listBox = SDL_static_cast(GUI_ListBox*, window->getChild(HOTKEYS_LISTBOX_EVENTID));
	if(!listBox)
		return;

	for(size_t i = 0; i < g_classicHotkeySlotCount; ++i)
		listBox->set(SDL_static_cast(Sint32, i), getHotkeyListText(i));
}

static void loadHotkeyEditorState()
{
	g_removedClassicHotkeys.clear();
	g_classicHotkeySlotCount = g_defaultClassicHotkeySlotCount;
	const std::vector<HotkeyUsage>& hotkeys = g_engine.getHotkeys();
	for(std::vector<HotkeyUsage>::const_iterator it = hotkeys.begin(), end = hotkeys.end(); it != end && g_classicHotkeySlotCount < CLASSIC_HOTKEY_MAX_SLOTS; ++it)
	{
		if(it->hotkey != CLIENT_HOTKEY_ACTION || findClassicHotkeySlot(it->keycode, it->modifers) >= 0 || g_engine.getHotkey(it->keycode, it->modifers) != &(*it))
			continue;

		std::string displayName;
		SDL_Keycode keycode;
		Uint16 modifiers;
		std::string configuredName = SDL_GetKeyName(it->keycode);
		if(it->modifers & KMOD_ALT)
			configuredName.insert(0, "Alt+");
		if(it->modifers & KMOD_SHIFT)
			configuredName.insert(0, "Shift+");
		if(it->modifers & KMOD_CTRL)
			configuredName.insert(0, "Ctrl+");
		if(!parseCustomHotkey(configuredName, keycode, modifiers, displayName))
			continue;

		classicHotkeySlots[g_classicHotkeySlotCount].name = displayName;
		classicHotkeySlots[g_classicHotkeySlotCount].keycode = keycode;
		classicHotkeySlots[g_classicHotkeySlotCount].modifiers = modifiers;
		++g_classicHotkeySlotCount;
	}

	for(size_t i = 0; i < g_classicHotkeySlotCount; ++i)
	{
		HotkeyUsage* hotkey = g_engine.getHotkey(classicHotkeySlots[i].keycode, classicHotkeySlots[i].modifiers);
		if(hotkey && hotkey->action.type == CLIENT_HOTKEY_ACTION_TEXT && hotkey->action.text.text)
		{
			g_classicHotkeyEditStates[i].text = *hotkey->action.text.text;
			g_classicHotkeyEditStates[i].sendAutomatically = hotkey->action.text.sendAutomatically;
			g_classicHotkeyEditStates[i].itemId = 0;
			g_classicHotkeyEditStates[i].itemSubtype = 0;
			g_classicHotkeyEditStates[i].itemUsageType = CLIENT_HOTKEY_ACTION_WITHCROSSHAIRS;
		}
		else if(hotkey && hotkey->action.type >= CLIENT_HOTKEY_ACTION_USEITEM && hotkey->action.type <= CLIENT_HOTKEY_ACTION_WITHCROSSHAIRS && hotkey->action.item.itemId != 0)
		{
			g_classicHotkeyEditStates[i].text.clear();
			g_classicHotkeyEditStates[i].sendAutomatically = false;
			g_classicHotkeyEditStates[i].itemId = hotkey->action.item.itemId;
			g_classicHotkeyEditStates[i].itemSubtype = hotkey->action.item.itemSubtype;
			g_classicHotkeyEditStates[i].itemUsageType = hotkey->action.item.usageType;
		}
		else
		{
			g_classicHotkeyEditStates[i].text.clear();
			g_classicHotkeyEditStates[i].sendAutomatically = false;
			g_classicHotkeyEditStates[i].itemId = 0;
			g_classicHotkeyEditStates[i].itemSubtype = 0;
			g_classicHotkeyEditStates[i].itemUsageType = CLIENT_HOTKEY_ACTION_WITHCROSSHAIRS;
		}
	}
	g_selectedClassicHotkey = 0;
}

static void storeCurrentEditorWidgets(GUI_Window* window)
{
	if(!window)
		return;

	GUI_TextBox* textBox = getHotkeyTextBox(window);
	GUI_CheckBox* sendAutoBox = getSendAutoCheckBox(window);
	if(!textBox || !sendAutoBox)
		return;

	const Sint32 slotCount = SDL_static_cast(Sint32, g_classicHotkeySlotCount);
	Sint32 select = UTIL_max<Sint32>(0, UTIL_min<Sint32>(slotCount - 1, g_selectedClassicHotkey));
	g_classicHotkeyEditStates[select].text = textBox->getActualText();
	g_classicHotkeyEditStates[select].sendAutomatically = sendAutoBox->isChecked();
	if(!g_classicHotkeyEditStates[select].text.empty())
		g_classicHotkeyEditStates[select].itemId = 0;
}

static void refreshHotkeyEditor(GUI_Window* window, Sint32 select)
{
	if(!window)
		return;

	const Sint32 slotCount = SDL_static_cast(Sint32, g_classicHotkeySlotCount);
	select = UTIL_max<Sint32>(0, UTIL_min<Sint32>(slotCount - 1, select));
	GUI_TextBox* textBox = getHotkeyTextBox(window);
	GUI_CheckBox* sendAutoBox = getSendAutoCheckBox(window);
	GUI_HotkeyItemBox* itemBox = getHotkeyItemBox(window);

	if(textBox)
		textBox->setText(g_classicHotkeyEditStates[select].text);

	if(sendAutoBox)
		sendAutoBox->setChecked(g_classicHotkeyEditStates[select].sendAutomatically);

	if(itemBox)
		itemBox->setItemData(g_classicHotkeyEditStates[select].itemId, g_classicHotkeyEditStates[select].itemSubtype);

	g_selectedClassicHotkey = select;
}

static void saveEditedHotkeys(GUI_Window* window)
{
	if(!window)
		return;

	storeCurrentEditorWidgets(window);
	for(std::vector<std::pair<SDL_Keycode, Uint16>>::const_iterator it = g_removedClassicHotkeys.begin(), end = g_removedClassicHotkeys.end(); it != end; ++it)
		g_engine.removeHotkeyAction(it->first, it->second);

	for(size_t i = 0; i < g_classicHotkeySlotCount; ++i)
	{
		if(!g_classicHotkeyEditStates[i].text.empty())
		{
			g_engine.bindHotkeyAction(classicHotkeySlots[i].keycode, classicHotkeySlots[i].modifiers,
				g_classicHotkeyEditStates[i].text, g_classicHotkeyEditStates[i].sendAutomatically);
		}
		else if(g_classicHotkeyEditStates[i].itemId != 0)
		{
			g_engine.bindHotkeyItem(classicHotkeySlots[i].keycode, classicHotkeySlots[i].modifiers,
				g_classicHotkeyEditStates[i].itemId, g_classicHotkeyEditStates[i].itemSubtype, g_classicHotkeyEditStates[i].itemUsageType);
		}
		else
			g_engine.bindHotkeyAction(classicHotkeySlots[i].keycode, classicHotkeySlots[i].modifiers, "", false);
	}

	// Persist custom slots immediately. Empty slots have no action payload,
	// so saveCFG also writes their key separately as HotkeySlot.
	g_engine.saveCFG();
}

static void openHotkeyCaptureWindow()
{
	GUI_Window* oldCapture = g_engine.getWindow(GUI_WINDOW_HOTKEYCAPTURE);
	if(oldCapture)
		g_engine.removeWindow(oldCapture);

	g_waitingHotkeyCapture = true;
	GUI_Window* captureWindow = new GUI_Window(iRect(0, 0, HOTKEY_CAPTURE_WIDTH, HOTKEY_CAPTURE_HEIGHT), "Adicionar Hotkey", GUI_WINDOW_HOTKEYCAPTURE);

	GUI_Label* label = new GUI_Label(iRect(HOTKEY_CAPTURE_WIDTH / 2, 31, 0, 0), "Pressione a tecla ou combinacao que deseja adicionar:", 0, 223, 223, 223);
	label->setAlign(CLIENT_FONT_ALIGN_CENTER);
	captureWindow->addChild(label);

	label = new GUI_Label(iRect(HOTKEY_CAPTURE_WIDTH / 2, 59, 0, 0), "Aguardando uma tecla...", 0, 160, 160, 160);
	label->setAlign(CLIENT_FONT_ALIGN_CENTER);
	captureWindow->addChild(label);

	label = new GUI_Label(iRect(HOTKEY_CAPTURE_WIDTH / 2, 87, 0, 0), "Exemplos: 1, Q, F8, Shift+Q ou Ctrl+1.", 0, 180, 180, 180);
	label->setAlign(CLIENT_FONT_ALIGN_CENTER);
	captureWindow->addChild(label);

	GUI_Separator* separator = new GUI_Separator(iRect(13, HOTKEY_CAPTURE_HEIGHT - 40, HOTKEY_CAPTURE_WIDTH - 26, 2));
	captureWindow->addChild(separator);

	GUI_Button* button = new GUI_Button(iRect((HOTKEY_CAPTURE_WIDTH - GUI_UI_BUTTON_58PX_GRAY_UP_W) / 2, HOTKEY_CAPTURE_HEIGHT - 30, GUI_UI_BUTTON_58PX_GRAY_UP_W, GUI_UI_BUTTON_58PX_GRAY_UP_H), "Cancel", CLIENT_GUI_ESCAPE_TRIGGER);
	button->setButtonEventCallback(&hotkey_Events, HOTKEY_CAPTURE_CANCEL_EVENTID);
	button->startEvents();
	captureWindow->addChild(button);

	g_engine.addWindow(captureWindow, true);
}

static void addCapturedHotkeyToEditor(SDL_Keycode keycode, Uint16 modifiers)
{
	GUI_Window* window = g_engine.getWindow(GUI_WINDOW_HOTKEYS);
	if(!window)
		return;

	if(keycode == SDLK_SPACE && modifiers == KMOD_NONE)
	{
		g_game.processTextMessage(MessageFailure, "Space esta reservado para atacar o proximo alvo da Battle List.");
		return;
	}

	storeCurrentEditorWidgets(window);
	Sint32 slot = findClassicHotkeySlot(keycode, modifiers);
	GUI_ListBox* listBox = SDL_static_cast(GUI_ListBox*, window->getChild(HOTKEYS_LISTBOX_EVENTID));
	if(slot < 0)
	{
		if(g_classicHotkeySlotCount >= CLASSIC_HOTKEY_MAX_SLOTS)
		{
			g_game.processTextMessage(MessageFailure, "Limite de hotkeys personalizadas atingido.");
			return;
		}

		slot = SDL_static_cast(Sint32, g_classicHotkeySlotCount++);
		classicHotkeySlots[slot].name = formatHotkeyName(keycode, modifiers, true);
		classicHotkeySlots[slot].keycode = keycode;
		classicHotkeySlots[slot].modifiers = modifiers;
		g_classicHotkeyEditStates[slot] = ClassicHotkeyEditState();
		if(listBox)
			listBox->add(getHotkeyListText(SDL_static_cast(size_t, slot)));
	}

	for(std::vector<std::pair<SDL_Keycode, Uint16>>::iterator it = g_removedClassicHotkeys.begin(); it != g_removedClassicHotkeys.end();)
	{
		if(it->first == keycode && it->second == modifiers)
			it = g_removedClassicHotkeys.erase(it);
		else
			++it;
	}

	refreshHotkeyList(window);
	refreshHotkeyEditor(window, slot);
	g_preferredHotkeyKeycode = keycode;
	g_preferredHotkeyModifiers = modifiers;
	if(listBox)
		listBox->setSelect(slot);

	g_game.processTextMessage(MessageLogin, std::string("Hotkey ") + formatHotkeyName(keycode, modifiers, false) + " adicionada. Configure a acao e clique em Ok.");
}

bool UTIL_isHotkeyCaptureActive()
{
	return g_waitingHotkeyCapture;
}

void UTIL_hotkeyCaptureKeyDown(SDL_Event& event)
{
	if(!g_waitingHotkeyCapture || event.key.repeat != 0)
		return;

	GUI_Window* captureWindow = g_engine.getWindow(GUI_WINDOW_HOTKEYCAPTURE);
	if(event.key.keysym.sym == SDLK_ESCAPE && normalizeHotkeyModifiers(event.key.keysym.mod) == KMOD_NONE)
	{
		g_waitingHotkeyCapture = false;
		if(captureWindow)
			g_engine.removeWindow(captureWindow);
		return;
	}
	if(isModifierKey(event.key.keysym.sym))
		return;

	SDL_Keycode keycode = event.key.keysym.sym;
	Uint16 modifiers = normalizeHotkeyModifiers(event.key.keysym.mod);
	g_waitingHotkeyCapture = false;
	if(captureWindow)
		g_engine.removeWindow(captureWindow);
	addCapturedHotkeyToEditor(keycode, modifiers);
}

void hotkey_Events(Uint32 event, Sint32 status)
{
	switch(event)
	{
		case HOTKEYS_LISTBOX_EVENTID:
		{
			GUI_Window* pWindow = g_engine.getCurrentWindow();
			if(pWindow && pWindow->getInternalID() == GUI_WINDOW_HOTKEYS)
			{
				storeCurrentEditorWidgets(pWindow);
				refreshHotkeyList(pWindow);
				refreshHotkeyEditor(pWindow, status);
				if(status >= 0 && SDL_static_cast(size_t, status) < g_classicHotkeySlotCount)
				{
					g_preferredHotkeyKeycode = classicHotkeySlots[status].keycode;
					g_preferredHotkeyModifiers = classicHotkeySlots[status].modifiers;
				}
			}
		}
		break;
		case HOTKEYS_TEXTBOX_EVENTID:
		{
			GUI_Window* pWindow = g_engine.getCurrentWindow();
			if(pWindow && pWindow->getInternalID() == GUI_WINDOW_HOTKEYS)
			{
				storeCurrentEditorWidgets(pWindow);
				refreshHotkeyList(pWindow);
			}
		}
		break;
		case HOTKEYS_ADD_KEY_BUTTON_EVENTID:
		{
			GUI_Window* pWindow = g_engine.getCurrentWindow();
			if(pWindow && pWindow->getInternalID() == GUI_WINDOW_HOTKEYS)
			{
				storeCurrentEditorWidgets(pWindow);
				openHotkeyCaptureWindow();
			}
		}
		break;
		case HOTKEYS_SEND_AUTO_EVENTID:
		{
			GUI_Window* pWindow = g_engine.getCurrentWindow();
			if(pWindow && pWindow->getInternalID() == GUI_WINDOW_HOTKEYS)
			{
				storeCurrentEditorWidgets(pWindow);
				refreshHotkeyList(pWindow);
			}
		}
		break;
		case HOTKEYS_REMOVE_KEY_BUTTON_EVENTID:
		{
			GUI_Window* pWindow = g_engine.getCurrentWindow();
			if(pWindow && pWindow->getInternalID() == GUI_WINDOW_HOTKEYS)
			{
				storeCurrentEditorWidgets(pWindow);
				Sint32 select = UTIL_max<Sint32>(0, UTIL_min<Sint32>(SDL_static_cast(Sint32, g_classicHotkeySlotCount) - 1, g_selectedClassicHotkey));
				if(SDL_static_cast(size_t, select) < g_defaultClassicHotkeySlotCount)
				{
					g_classicHotkeyEditStates[select] = ClassicHotkeyEditState();
					refreshHotkeyList(pWindow);
					refreshHotkeyEditor(pWindow, select);
					break;
				}

				g_removedClassicHotkeys.push_back(std::make_pair(classicHotkeySlots[select].keycode, classicHotkeySlots[select].modifiers));
				for(size_t i = SDL_static_cast(size_t, select); i + 1 < g_classicHotkeySlotCount; ++i)
				{
					classicHotkeySlots[i] = classicHotkeySlots[i + 1];
					g_classicHotkeyEditStates[i] = g_classicHotkeyEditStates[i + 1];
				}
				--g_classicHotkeySlotCount;
				Sint32 nextSelect = UTIL_min<Sint32>(select, SDL_static_cast(Sint32, g_classicHotkeySlotCount) - 1);
				g_preferredHotkeyKeycode = classicHotkeySlots[nextSelect].keycode;
				g_preferredHotkeyModifiers = classicHotkeySlots[nextSelect].modifiers;
				refreshHotkeyEditor(pWindow, nextSelect);
				GUI_ListBox* listBox = SDL_static_cast(GUI_ListBox*, pWindow->getChild(HOTKEYS_LISTBOX_EVENTID));
				if(listBox)
				{
					listBox->erase(select);
					refreshHotkeyList(pWindow);
					listBox->setSelect(nextSelect);
				}
			}
		}
		break;
		case HOTKEY_CAPTURE_CANCEL_EVENTID:
		{
			g_waitingHotkeyCapture = false;
			GUI_Window* pWindow = g_engine.getCurrentWindow();
			if(pWindow && pWindow->getInternalID() == GUI_WINDOW_HOTKEYCAPTURE)
				g_engine.removeWindow(pWindow);
		}
		break;
		case HOTKEYS_SELECT_OBJECT_EVENTID:
		{
			GUI_Window* pWindow = g_engine.getCurrentWindow();
			if(pWindow && pWindow->getInternalID() == GUI_WINDOW_HOTKEYS)
			{
				storeCurrentEditorWidgets(pWindow);
				const Sint32 slotCount = SDL_static_cast(Sint32, g_classicHotkeySlotCount);
				Sint32 select = UTIL_max<Sint32>(0, UTIL_min<Sint32>(slotCount - 1, g_selectedClassicHotkey));
				g_engine.removeWindow(pWindow);
				GUI_Window* optionsWindow = g_engine.getWindow(GUI_WINDOW_OPTIONS);
				if(optionsWindow)
					g_engine.removeWindow(optionsWindow);
				g_engine.beginHotkeyItemSelection(classicHotkeySlots[select].keycode, classicHotkeySlots[select].modifiers);
			}
		}
		break;
		case HOTKEYS_CLEAR_OBJECT_EVENTID:
		{
			GUI_Window* pWindow = g_engine.getCurrentWindow();
			if(pWindow && pWindow->getInternalID() == GUI_WINDOW_HOTKEYS)
			{
				g_classicHotkeyEditStates[g_selectedClassicHotkey].itemId = 0;
				g_classicHotkeyEditStates[g_selectedClassicHotkey].itemSubtype = 0;
				g_classicHotkeyEditStates[g_selectedClassicHotkey].itemUsageType = CLIENT_HOTKEY_ACTION_WITHCROSSHAIRS;
				refreshHotkeyList(pWindow);
				refreshHotkeyEditor(pWindow, g_selectedClassicHotkey);
			}
		}
		break;
		case HOTKEYS_USE_YOURSELF_EVENTID:
		case HOTKEYS_USE_TARGET_EVENTID:
		case HOTKEYS_CROSSHAIRS_EVENTID:
		{
			GUI_Window* pWindow = g_engine.getCurrentWindow();
			if(pWindow && pWindow->getInternalID() == GUI_WINDOW_HOTKEYS && g_classicHotkeyEditStates[g_selectedClassicHotkey].itemId != 0)
			{
				if(event == HOTKEYS_USE_YOURSELF_EVENTID)
					g_classicHotkeyEditStates[g_selectedClassicHotkey].itemUsageType = CLIENT_HOTKEY_ACTION_USEONYOURSELF;
				else if(event == HOTKEYS_USE_TARGET_EVENTID)
					g_classicHotkeyEditStates[g_selectedClassicHotkey].itemUsageType = CLIENT_HOTKEY_ACTION_USEONTARGET;
				else
					g_classicHotkeyEditStates[g_selectedClassicHotkey].itemUsageType = CLIENT_HOTKEY_ACTION_WITHCROSSHAIRS;
				g_classicHotkeyEditStates[g_selectedClassicHotkey].text.clear();
				GUI_TextBox* textBox = getHotkeyTextBox(pWindow);
				if(textBox)
					textBox->setText("");
				refreshHotkeyList(pWindow);
			}
		}
		break;
		case HOTKEYS_CANCEL_EVENTID:
		{
			GUI_Window* pWindow = g_engine.getCurrentWindow();
			if(pWindow && pWindow->getInternalID() == GUI_WINDOW_HOTKEYS)
				g_engine.removeWindow(pWindow);
		}
		break;
		case HOTKEYS_OK_EVENTID:
		{
			GUI_Window* pWindow = g_engine.getCurrentWindow();
			if(pWindow && pWindow->getInternalID() == GUI_WINDOW_HOTKEYS)
			{
				saveEditedHotkeys(pWindow);
				g_engine.removeWindow(pWindow);
			}
		}
		break;
		default: break;
	}
}

void UTIL_hotkeyOptions()
{
	GUI_Window* pWindow = g_engine.getWindow(GUI_WINDOW_HOTKEYS);
	if(pWindow)
		g_engine.removeWindow(pWindow);

	loadHotkeyEditorState();
	Sint32 initialSelect = 0;
	if(g_preferredHotkeyKeycode != SDLK_UNKNOWN)
	{
		Sint32 preferredSelect = findClassicHotkeySlot(g_preferredHotkeyKeycode, g_preferredHotkeyModifiers);
		if(preferredSelect >= 0)
			initialSelect = preferredSelect;
	}

	GUI_Window* newWindow = new GUI_Window(iRect(0, 0, HOTKEYS_WIDTH, HOTKEYS_HEIGHT), HOTKEYS_TITLE, GUI_WINDOW_HOTKEYS);

	GUI_Label* newLabel = new GUI_Label(iRect(HOTKEYS_LABEL_X, HOTKEYS_AVAILABLE_LABEL_Y, 0, 0), "Available hotkeys:", 0, HOTKEYS_LABEL_COLOR, HOTKEYS_LABEL_COLOR, HOTKEYS_LABEL_COLOR);
	newWindow->addChild(newLabel);

	GUI_ListBox* newListBox = new GUI_ListBox(iRect(HOTKEYS_LIST_X, HOTKEYS_LIST_Y, HOTKEYS_LIST_W, HOTKEYS_LIST_H), HOTKEYS_LISTBOX_EVENTID);
	for(size_t i = 0; i < g_classicHotkeySlotCount; ++i)
		newListBox->add(getHotkeyListText(i));
	newListBox->setEventCallback(&hotkey_Events, HOTKEYS_LISTBOX_EVENTID);
	newListBox->setSelect(initialSelect);
	newListBox->startEvents();
	newWindow->addChild(newListBox);

	GUI_Button* addKeyButton = new GUI_Button(iRect(HOTKEYS_LIST_X, HOTKEYS_MANAGE_BUTTON_Y, GUI_UI_BUTTON_58PX_GRAY_UP_W, GUI_UI_BUTTON_58PX_GRAY_UP_H), "Add", HOTKEYS_ADD_KEY_BUTTON_EVENTID);
	addKeyButton->setButtonEventCallback(&hotkey_Events, HOTKEYS_ADD_KEY_BUTTON_EVENTID);
	addKeyButton->startEvents();
	newWindow->addChild(addKeyButton);

	GUI_Button* removeKeyButton = new GUI_Button(iRect(HOTKEYS_LIST_X + 68, HOTKEYS_MANAGE_BUTTON_Y, GUI_UI_BUTTON_58PX_GRAY_UP_W, GUI_UI_BUTTON_58PX_GRAY_UP_H), "Remove", HOTKEYS_REMOVE_KEY_BUTTON_EVENTID);
	removeKeyButton->setButtonEventCallback(&hotkey_Events, HOTKEYS_REMOVE_KEY_BUTTON_EVENTID);
	removeKeyButton->startEvents();
	newWindow->addChild(removeKeyButton);

	newLabel = new GUI_Label(iRect(HOTKEYS_LABEL_X, HOTKEYS_TEXT_LABEL_Y, 0, 0), "Edit hotkey text:", 0, HOTKEYS_LABEL_COLOR, HOTKEYS_LABEL_COLOR, HOTKEYS_LABEL_COLOR);
	newWindow->addChild(newLabel);

	GUI_TextBox* newTextBox = new GUI_TextBox(iRect(HOTKEYS_TEXTBOX_X, HOTKEYS_TEXTBOX_Y, HOTKEYS_TEXTBOX_W, HOTKEYS_TEXTBOX_H), "", HOTKEYS_TEXTBOX_EVENTID, 223, 223, 223);
	newTextBox->setMaxLength(255);
	newTextBox->setTextEventCallback(&hotkey_Events, HOTKEYS_TEXTBOX_EVENTID);
	newTextBox->startEvents();
	newWindow->addChild(newTextBox);

	GUI_CheckBox* newCheckBox = new GUI_CheckBox(iRect(HOTKEYS_SEND_AUTO_X, HOTKEYS_SEND_AUTO_Y, HOTKEYS_SEND_AUTO_W, HOTKEYS_SEND_AUTO_H), "Send automatically", false, HOTKEYS_SEND_AUTO_EVENTID);
	newCheckBox->setBoxEventCallback(&hotkey_Events, HOTKEYS_SEND_AUTO_EVENTID);
	newCheckBox->startEvents();
	newWindow->addChild(newCheckBox);

	newLabel = new GUI_Label(iRect(HOTKEYS_LABEL_X, HOTKEYS_OBJECT_LABEL_Y, 0, 0), "Edit hotkey object:", 0, HOTKEYS_LABEL_COLOR, HOTKEYS_LABEL_COLOR, HOTKEYS_LABEL_COLOR);
	newWindow->addChild(newLabel);

	GUI_HotkeyItemBox* newItemBox = new GUI_HotkeyItemBox(iRect(HOTKEYS_OBJECT_BOX_X, HOTKEYS_OBJECT_BOX_Y, HOTKEYS_OBJECT_BOX_W, HOTKEYS_OBJECT_BOX_H), HOTKEYS_OBJECT_DISPLAY_EVENTID);
	newWindow->addChild(newItemBox);

	GUI_Button* newButton = new GUI_Button(iRect(HOTKEYS_OBJECT_BUTTON_X, HOTKEYS_OBJECT_BOX_Y, HOTKEYS_OBJECT_BUTTON_W, HOTKEYS_OBJECT_BUTTON_H), "Select Object", HOTKEYS_SELECT_OBJECT_EVENTID);
	newButton->setButtonEventCallback(&hotkey_Events, HOTKEYS_SELECT_OBJECT_EVENTID);
	newButton->startEvents();
	newWindow->addChild(newButton);

	newButton = new GUI_Button(iRect(HOTKEYS_OBJECT_BUTTON_X, HOTKEYS_OBJECT_BOX_Y + HOTKEYS_OBJECT_BUTTON_GAP, HOTKEYS_OBJECT_BUTTON_W, HOTKEYS_OBJECT_BUTTON_H), "Clear Object", HOTKEYS_CLEAR_OBJECT_EVENTID);
	newButton->setButtonEventCallback(&hotkey_Events, HOTKEYS_CLEAR_OBJECT_EVENTID);
	newButton->startEvents();
	newWindow->addChild(newButton);

	newButton = new GUI_Button(iRect(HOTKEYS_USE_BUTTON_X, HOTKEYS_OBJECT_BOX_Y, HOTKEYS_USE_BUTTON_W, HOTKEYS_USE_BUTTON_H), "Use on yourself", HOTKEYS_USE_YOURSELF_EVENTID);
	newButton->setButtonEventCallback(&hotkey_Events, HOTKEYS_USE_YOURSELF_EVENTID);
	newButton->startEvents();
	newWindow->addChild(newButton);

	newButton = new GUI_Button(iRect(HOTKEYS_USE_BUTTON_X, HOTKEYS_OBJECT_BOX_Y + HOTKEYS_OBJECT_BUTTON_GAP, HOTKEYS_USE_BUTTON_W, HOTKEYS_USE_BUTTON_H), "Use on target", HOTKEYS_USE_TARGET_EVENTID);
	newButton->setButtonEventCallback(&hotkey_Events, HOTKEYS_USE_TARGET_EVENTID);
	newButton->startEvents();
	newWindow->addChild(newButton);

	newButton = new GUI_Button(iRect(HOTKEYS_USE_BUTTON_X, HOTKEYS_OBJECT_BOX_Y + HOTKEYS_OBJECT_BUTTON_GAP * 2, HOTKEYS_USE_BUTTON_W, HOTKEYS_USE_BUTTON_H), "With crosshairs", HOTKEYS_CROSSHAIRS_EVENTID);
	newButton->setButtonEventCallback(&hotkey_Events, HOTKEYS_CROSSHAIRS_EVENTID);
	newButton->startEvents();
	newWindow->addChild(newButton);

	newButton = new GUI_Button(iRect(HOTKEYS_WIDTH - 56, HOTKEYS_HEIGHT - 30, GUI_UI_BUTTON_43PX_GRAY_UP_W, GUI_UI_BUTTON_43PX_GRAY_UP_H), "Cancel", CLIENT_GUI_ESCAPE_TRIGGER);
	newButton->setButtonEventCallback(&hotkey_Events, HOTKEYS_CANCEL_EVENTID);
	newButton->startEvents();
	newWindow->addChild(newButton);

	newButton = new GUI_Button(iRect(HOTKEYS_WIDTH - 109, HOTKEYS_HEIGHT - 30, GUI_UI_BUTTON_43PX_GRAY_UP_W, GUI_UI_BUTTON_43PX_GRAY_UP_H), "Ok", CLIENT_GUI_ENTER_TRIGGER);
	newButton->setButtonEventCallback(&hotkey_Events, HOTKEYS_OK_EVENTID);
	newButton->startEvents();
	newWindow->addChild(newButton);

	GUI_Separator* newSeparator = new GUI_Separator(iRect(13, HOTKEYS_HEIGHT - 40, HOTKEYS_WIDTH - 26, 2));
	newWindow->addChild(newSeparator);

	g_engine.addWindow(newWindow, true);
	refreshHotkeyEditor(newWindow, initialSelect);
}
