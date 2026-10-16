/*
  Configuration window for the native assistant additions.
*/

#include "GUI_UTIL.h"
#include "../assistant.h"
#include "../engine.h"
#include "../GUI_Elements/GUI_Window.h"
#include "../GUI_Elements/GUI_Button.h"
#include "../GUI_Elements/GUI_CheckBox.h"
#include "../GUI_Elements/GUI_Label.h"
#include "../GUI_Elements/GUI_Separator.h"
#include "../GUI_Elements/GUI_TextBox.h"

#define ASSISTANT_OPTIONS_WIDTH 440
#define ASSISTANT_OPTIONS_HEIGHT 590
#define ASSISTANT_CANCEL_EVENTID 1000
#define ASSISTANT_OK_EVENTID 1001
#define ASSISTANT_ENABLED_EVENTID 1100
#define ASSISTANT_FULL_LIGHT_EVENTID 1101
#define ASSISTANT_HEAL_ENABLED_EVENTID 1102
#define ASSISTANT_HEAL_RUNE_EVENTID 1103
#define ASSISTANT_HASTE_ENABLED_EVENTID 1104
#define ASSISTANT_MANA_ENABLED_EVENTID 1105
#define ASSISTANT_ENERGY_RING_ENABLED_EVENTID 1106
#define ASSISTANT_AMULET_ENABLED_EVENTID 1107
#define ASSISTANT_HEAL_PERCENT_EVENTID 1200
#define ASSISTANT_HEAL_SPELL_EVENTID 1201
#define ASSISTANT_HEAL_RUNE_ID_EVENTID 1202
#define ASSISTANT_HEAL_RUNE_SUBTYPE_EVENTID 1203
#define ASSISTANT_HASTE_SPELL_EVENTID 1204
#define ASSISTANT_MANA_PERCENT_EVENTID 1205
#define ASSISTANT_MANA_ID_EVENTID 1206
#define ASSISTANT_MANA_SUBTYPE_EVENTID 1207
#define ASSISTANT_ENERGY_RING_EQUIP_PERCENT_EVENTID 1208
#define ASSISTANT_ENERGY_RING_UNEQUIP_PERCENT_EVENTID 1209
#define ASSISTANT_ENERGY_RING_ID_EVENTID 1210
#define ASSISTANT_ENERGY_RING_SUBTYPE_EVENTID 1211
#define ASSISTANT_AMULET_PERCENT_EVENTID 1212
#define ASSISTANT_LOW_AMULET_ID_EVENTID 1213
#define ASSISTANT_LOW_AMULET_SUBTYPE_EVENTID 1214
#define ASSISTANT_HIGH_AMULET_ID_EVENTID 1215
#define ASSISTANT_HIGH_AMULET_SUBTYPE_EVENTID 1216

extern Engine g_engine;

namespace
{
	bool getChecked(GUI_Window* window, Uint32 id)
	{
		GUI_CheckBox* checkBox = SDL_static_cast(GUI_CheckBox*, window->getChild(id));
		return (checkBox ? checkBox->isChecked() : false);
	}

	std::string getText(GUI_Window* window, Uint32 id)
	{
		GUI_TextBox* textBox = SDL_static_cast(GUI_TextBox*, window->getChild(id));
		return (textBox ? textBox->getActualText() : std::string());
	}

	Uint32 getNumber(GUI_Window* window, Uint32 id, Uint32 maximum)
	{
		Uint32 value = SDL_static_cast(Uint32, SDL_strtoul(getText(window, id).c_str(), NULL, 10));
		return (value > maximum ? maximum : value);
	}

	std::string numberText(Uint32 value)
	{
		char buffer[32];
		SDL_snprintf(buffer, sizeof(buffer), "%u", value);
		return std::string(buffer);
	}

	void addCheckBox(GUI_Window* window, Sint32 x, Sint32 y, Sint32 width, const char* label, bool checked, Uint32 id)
	{
		GUI_CheckBox* checkBox = new GUI_CheckBox(iRect(x, y, width, 22), label, checked, id);
		checkBox->startEvents();
		window->addChild(checkBox);
	}

	void addLabel(GUI_Window* window, Sint32 x, Sint32 y, const char* label)
	{
		window->addChild(new GUI_Label(iRect(x, y, 0, 0), label));
	}

	void addTextBox(GUI_Window* window, Sint32 x, Sint32 y, Sint32 width, const std::string& text, Uint32 id, Uint32 maxLength, bool numbersOnly = false)
	{
		GUI_TextBox* textBox = new GUI_TextBox(iRect(x, y, width, 16), text, id);
		textBox->setMaxLength(maxLength);
		textBox->setOnlyNumbers(numbersOnly);
		textBox->startEvents();
		window->addChild(textBox);
	}
}

void assistantOptions_Events(Uint32 event, Sint32)
{
	GUI_Window* window = g_engine.getCurrentWindow();
	if(!window || window->getInternalID() != GUI_WINDOW_ASSISTANTOPTIONS)
		return;

	switch(event)
	{
		case ASSISTANT_CANCEL_EVENTID:
			g_engine.removeWindow(window);
			break;
		case ASSISTANT_OK_EVENTID:
		{
			AssistantSettings settings = g_assistant.getSettings();
			settings.enabled = getChecked(window, ASSISTANT_ENABLED_EVENTID);
			settings.fullLight = getChecked(window, ASSISTANT_FULL_LIGHT_EVENTID);
			settings.healingEnabled = getChecked(window, ASSISTANT_HEAL_ENABLED_EVENTID);
			settings.healingUseRune = getChecked(window, ASSISTANT_HEAL_RUNE_EVENTID);
			settings.healingPercent = SDL_static_cast(Uint8, getNumber(window, ASSISTANT_HEAL_PERCENT_EVENTID, 100));
			settings.healingSpell = getText(window, ASSISTANT_HEAL_SPELL_EVENTID);
			settings.healingRuneId = SDL_static_cast(Uint16, getNumber(window, ASSISTANT_HEAL_RUNE_ID_EVENTID, 65535));
			settings.healingRuneSubtype = SDL_static_cast(Uint8, getNumber(window, ASSISTANT_HEAL_RUNE_SUBTYPE_EVENTID, 255));
			settings.hasteEnabled = getChecked(window, ASSISTANT_HASTE_ENABLED_EVENTID);
			settings.hasteSpell = getText(window, ASSISTANT_HASTE_SPELL_EVENTID);
			settings.manaEnabled = getChecked(window, ASSISTANT_MANA_ENABLED_EVENTID);
			settings.manaPercent = SDL_static_cast(Uint8, getNumber(window, ASSISTANT_MANA_PERCENT_EVENTID, 100));
			settings.manaFluidId = SDL_static_cast(Uint16, getNumber(window, ASSISTANT_MANA_ID_EVENTID, 65535));
			settings.manaFluidSubtype = SDL_static_cast(Uint8, getNumber(window, ASSISTANT_MANA_SUBTYPE_EVENTID, 255));
			settings.energyRingEnabled = getChecked(window, ASSISTANT_ENERGY_RING_ENABLED_EVENTID);
			settings.energyRingEquipPercent = SDL_static_cast(Uint8, getNumber(window, ASSISTANT_ENERGY_RING_EQUIP_PERCENT_EVENTID, 100));
			settings.energyRingUnequipPercent = SDL_static_cast(Uint8, getNumber(window, ASSISTANT_ENERGY_RING_UNEQUIP_PERCENT_EVENTID, 100));
			settings.energyRingId = SDL_static_cast(Uint16, getNumber(window, ASSISTANT_ENERGY_RING_ID_EVENTID, 65535));
			settings.energyRingSubtype = SDL_static_cast(Uint8, getNumber(window, ASSISTANT_ENERGY_RING_SUBTYPE_EVENTID, 255));
			settings.amuletEnabled = getChecked(window, ASSISTANT_AMULET_ENABLED_EVENTID);
			settings.amuletSwitchPercent = SDL_static_cast(Uint8, getNumber(window, ASSISTANT_AMULET_PERCENT_EVENTID, 100));
			settings.lowHealthAmuletId = SDL_static_cast(Uint16, getNumber(window, ASSISTANT_LOW_AMULET_ID_EVENTID, 65535));
			settings.lowHealthAmuletSubtype = SDL_static_cast(Uint8, getNumber(window, ASSISTANT_LOW_AMULET_SUBTYPE_EVENTID, 255));
			settings.highHealthAmuletId = SDL_static_cast(Uint16, getNumber(window, ASSISTANT_HIGH_AMULET_ID_EVENTID, 65535));
			settings.highHealthAmuletSubtype = SDL_static_cast(Uint8, getNumber(window, ASSISTANT_HIGH_AMULET_SUBTYPE_EVENTID, 255));

			g_assistant.applySettings(settings);
			g_assistant.saveConfig();
			g_engine.removeWindow(window);
		}
		break;
		default: break;
	}
}

void UTIL_assistantOptions()
{
	GUI_Window* existingWindow = g_engine.getWindow(GUI_WINDOW_ASSISTANTOPTIONS);
	if(existingWindow)
	{
		g_engine.removeWindow(existingWindow);
		return;
	}

	const AssistantSettings& settings = g_assistant.getSettings();
	GUI_Window* window = new GUI_Window(iRect(0, 0, ASSISTANT_OPTIONS_WIDTH, ASSISTANT_OPTIONS_HEIGHT), "Assistente", GUI_WINDOW_ASSISTANTOPTIONS);

	addCheckBox(window, 18, 30, 200, "Ativar automacoes", settings.enabled, ASSISTANT_ENABLED_EVENTID);
	addCheckBox(window, 225, 30, 190, "Full light", settings.fullLight, ASSISTANT_FULL_LIGHT_EVENTID);
	window->addChild(new GUI_Separator(iRect(13, 61, ASSISTANT_OPTIONS_WIDTH - 26, 2)));

	addCheckBox(window, 18, 70, 190, "Auto cura", settings.healingEnabled, ASSISTANT_HEAL_ENABLED_EVENTID);
	addLabel(window, 26, 103, "Vida <=");
	addTextBox(window, 79, 99, 42, numberText(settings.healingPercent), ASSISTANT_HEAL_PERCENT_EVENTID, 3, true);
	addLabel(window, 126, 103, "%");
	addCheckBox(window, 185, 93, 225, "Usar runa (desmarcado = magia)", settings.healingUseRune, ASSISTANT_HEAL_RUNE_EVENTID);
	addLabel(window, 26, 132, "Magia de cura:");
	addTextBox(window, 121, 128, 285, settings.healingSpell, ASSISTANT_HEAL_SPELL_EVENTID, 80);
	addLabel(window, 26, 161, "Runa ID:");
	addTextBox(window, 82, 157, 70, numberText(settings.healingRuneId), ASSISTANT_HEAL_RUNE_ID_EVENTID, 5, true);
	addLabel(window, 174, 161, "Subtipo:");
	addTextBox(window, 225, 157, 48, numberText(settings.healingRuneSubtype), ASSISTANT_HEAL_RUNE_SUBTYPE_EVENTID, 3, true);
	window->addChild(new GUI_Separator(iRect(13, 186, ASSISTANT_OPTIONS_WIDTH - 26, 2)));

	addCheckBox(window, 18, 195, 190, "Auto haste", settings.hasteEnabled, ASSISTANT_HASTE_ENABLED_EVENTID);
	addLabel(window, 26, 228, "Magia de haste:");
	addTextBox(window, 121, 224, 285, settings.hasteSpell, ASSISTANT_HASTE_SPELL_EVENTID, 80);
	window->addChild(new GUI_Separator(iRect(13, 253, ASSISTANT_OPTIONS_WIDTH - 26, 2)));

	addCheckBox(window, 18, 262, 190, "Auto mana fluid", settings.manaEnabled, ASSISTANT_MANA_ENABLED_EVENTID);
	addLabel(window, 26, 295, "Mana <=");
	addTextBox(window, 82, 291, 42, numberText(settings.manaPercent), ASSISTANT_MANA_PERCENT_EVENTID, 3, true);
	addLabel(window, 129, 295, "%");
	addLabel(window, 174, 295, "Item ID:");
	addTextBox(window, 226, 291, 70, numberText(settings.manaFluidId), ASSISTANT_MANA_ID_EVENTID, 5, true);
	addLabel(window, 312, 295, "Sub:");
	addTextBox(window, 345, 291, 48, numberText(settings.manaFluidSubtype), ASSISTANT_MANA_SUBTYPE_EVENTID, 3, true);
	window->addChild(new GUI_Separator(iRect(13, 320, ASSISTANT_OPTIONS_WIDTH - 26, 2)));

	addCheckBox(window, 18, 329, 250, "Auto Energy Ring", settings.energyRingEnabled, ASSISTANT_ENERGY_RING_ENABLED_EVENTID);
	addLabel(window, 26, 362, "Equipar HP <=");
	addTextBox(window, 106, 358, 42, numberText(settings.energyRingEquipPercent), ASSISTANT_ENERGY_RING_EQUIP_PERCENT_EVENTID, 3, true);
	addLabel(window, 153, 362, "%");
	addLabel(window, 190, 362, "Retirar HP >=");
	addTextBox(window, 273, 358, 42, numberText(settings.energyRingUnequipPercent), ASSISTANT_ENERGY_RING_UNEQUIP_PERCENT_EVENTID, 3, true);
	addLabel(window, 320, 362, "%");
	addLabel(window, 26, 391, "Item ID:");
	addTextBox(window, 78, 387, 70, numberText(settings.energyRingId), ASSISTANT_ENERGY_RING_ID_EVENTID, 5, true);
	addLabel(window, 174, 391, "Subtipo:");
	addTextBox(window, 225, 387, 48, numberText(settings.energyRingSubtype), ASSISTANT_ENERGY_RING_SUBTYPE_EVENTID, 3, true);
	window->addChild(new GUI_Separator(iRect(13, 416, ASSISTANT_OPTIONS_WIDTH - 26, 2)));

	addCheckBox(window, 18, 425, 250, "Auto SSA / troca de amuleto", settings.amuletEnabled, ASSISTANT_AMULET_ENABLED_EVENTID);
	addLabel(window, 26, 458, "Trocar em HP:");
	addTextBox(window, 106, 454, 42, numberText(settings.amuletSwitchPercent), ASSISTANT_AMULET_PERCENT_EVENTID, 3, true);
	addLabel(window, 153, 458, "%");
	addLabel(window, 26, 487, "HP baixo ID:");
	addTextBox(window, 106, 483, 70, numberText(settings.lowHealthAmuletId), ASSISTANT_LOW_AMULET_ID_EVENTID, 5, true);
	addLabel(window, 190, 487, "Sub:");
	addTextBox(window, 223, 483, 48, numberText(settings.lowHealthAmuletSubtype), ASSISTANT_LOW_AMULET_SUBTYPE_EVENTID, 3, true);
	addLabel(window, 26, 516, "HP alto ID:");
	addTextBox(window, 106, 512, 70, numberText(settings.highHealthAmuletId), ASSISTANT_HIGH_AMULET_ID_EVENTID, 5, true);
	addLabel(window, 190, 516, "Sub:");
	addTextBox(window, 223, 512, 48, numberText(settings.highHealthAmuletSubtype), ASSISTANT_HIGH_AMULET_SUBTYPE_EVENTID, 3, true);

	window->addChild(new GUI_Separator(iRect(13, ASSISTANT_OPTIONS_HEIGHT - 40, ASSISTANT_OPTIONS_WIDTH - 26, 2)));
	GUI_Button* button = new GUI_Button(iRect(ASSISTANT_OPTIONS_WIDTH - 109, ASSISTANT_OPTIONS_HEIGHT - 30, GUI_UI_BUTTON_43PX_GRAY_UP_W, GUI_UI_BUTTON_43PX_GRAY_UP_H), "Ok", CLIENT_GUI_ENTER_TRIGGER);
	button->setButtonEventCallback(&assistantOptions_Events, ASSISTANT_OK_EVENTID);
	button->startEvents();
	window->addChild(button);
	button = new GUI_Button(iRect(ASSISTANT_OPTIONS_WIDTH - 56, ASSISTANT_OPTIONS_HEIGHT - 30, GUI_UI_BUTTON_43PX_GRAY_UP_W, GUI_UI_BUTTON_43PX_GRAY_UP_H), "Cancel", CLIENT_GUI_ESCAPE_TRIGGER);
	button->setButtonEventCallback(&assistantOptions_Events, ASSISTANT_CANCEL_EVENTID);
	button->startEvents();
	window->addChild(button);
	g_engine.addWindow(window, true);
}
