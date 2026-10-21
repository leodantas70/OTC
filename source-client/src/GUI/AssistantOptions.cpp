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
#define ASSISTANT_OPTIONS_HEIGHT 700
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
#define ASSISTANT_AMMUNITION_ENABLED_EVENTID 1108
#define ASSISTANT_REVEAL_INVISIBLE_EVENTID 1109
#define ASSISTANT_DASH_ENABLED_EVENTID 1110
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
#define ASSISTANT_AMMUNITION_ID_EVENTID 1217
#define ASSISTANT_PREFERRED_RING_ID_EVENTID 1218
#define ASSISTANT_PREFERRED_RING_SUBTYPE_EVENTID 1219

#define AUTORUNE_OPTIONS_WIDTH 400
#define AUTORUNE_OPTIONS_HEIGHT 400
#define AUTORUNE_CANCEL_EVENTID 1500
#define AUTORUNE_OK_EVENTID 1501
#define AUTORUNE_ENABLED_EVENTID 1502
#define AUTORUNE_MANA_TRAINING_EVENTID 1503
#define AUTORUNE_SPELL_EVENTID 1504
#define AUTORUNE_MINIMUM_MANA_EVENTID 1505
#define AUTORUNE_ANTI_IDLE_EVENTID 1506
#define AUTORUNE_AUTO_FOOD_EVENTID 1507
#define AUTORUNE_FOOD_IDS_EVENTID 1508
#define AUTORUNE_LOGOUT_BLANK_EVENTID 1509
#define AUTORUNE_BLANK_ID_EVENTID 1510
#define AUTORUNE_LOGOUT_NONVIP_EVENTID 1511
#define AUTORUNE_TRUSTED_PLAYERS_EVENTID 1512

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
			settings.revealInvisibleMonsters = getChecked(window, ASSISTANT_REVEAL_INVISIBLE_EVENTID);
			settings.dashEnabled = getChecked(window, ASSISTANT_DASH_ENABLED_EVENTID);
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
			settings.preferredRingId = SDL_static_cast(Uint16, getNumber(window, ASSISTANT_PREFERRED_RING_ID_EVENTID, 65535));
			settings.preferredRingSubtype = SDL_static_cast(Uint8, getNumber(window, ASSISTANT_PREFERRED_RING_SUBTYPE_EVENTID, 255));
			settings.amuletEnabled = getChecked(window, ASSISTANT_AMULET_ENABLED_EVENTID);
			settings.amuletSwitchPercent = SDL_static_cast(Uint8, getNumber(window, ASSISTANT_AMULET_PERCENT_EVENTID, 100));
			settings.lowHealthAmuletId = SDL_static_cast(Uint16, getNumber(window, ASSISTANT_LOW_AMULET_ID_EVENTID, 65535));
			settings.lowHealthAmuletSubtype = SDL_static_cast(Uint8, getNumber(window, ASSISTANT_LOW_AMULET_SUBTYPE_EVENTID, 255));
			settings.highHealthAmuletId = SDL_static_cast(Uint16, getNumber(window, ASSISTANT_HIGH_AMULET_ID_EVENTID, 65535));
			settings.highHealthAmuletSubtype = SDL_static_cast(Uint8, getNumber(window, ASSISTANT_HIGH_AMULET_SUBTYPE_EVENTID, 255));
			settings.ammunitionEnabled = getChecked(window, ASSISTANT_AMMUNITION_ENABLED_EVENTID);
			settings.ammunitionId = SDL_static_cast(Uint16, getNumber(window, ASSISTANT_AMMUNITION_ID_EVENTID, 65535));

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

	addCheckBox(window, 18, 30, 140, "Ativar automacoes", settings.enabled, ASSISTANT_ENABLED_EVENTID);
	addCheckBox(window, 160, 30, 85, "Full light", settings.fullLight, ASSISTANT_FULL_LIGHT_EVENTID);
	addCheckBox(window, 247, 30, 125, "Revelar invis.", settings.revealInvisibleMonsters, ASSISTANT_REVEAL_INVISIBLE_EVENTID);
	addCheckBox(window, 374, 30, 52, "Dash", settings.dashEnabled, ASSISTANT_DASH_ENABLED_EVENTID);
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
	addLabel(window, 26, 420, "Ring normal ID:");
	addTextBox(window, 112, 416, 70, numberText(settings.preferredRingId), ASSISTANT_PREFERRED_RING_ID_EVENTID, 5, true);
	addLabel(window, 198, 420, "Subtipo:");
	addTextBox(window, 249, 416, 48, numberText(settings.preferredRingSubtype), ASSISTANT_PREFERRED_RING_SUBTYPE_EVENTID, 3, true);
	addLabel(window, 307, 420, "0 = deixar vazio");
	window->addChild(new GUI_Separator(iRect(13, 445, ASSISTANT_OPTIONS_WIDTH - 26, 2)));

	addCheckBox(window, 18, 454, 250, "Auto SSA / troca de amuleto", settings.amuletEnabled, ASSISTANT_AMULET_ENABLED_EVENTID);
	addLabel(window, 26, 487, "Trocar em HP:");
	addTextBox(window, 106, 483, 42, numberText(settings.amuletSwitchPercent), ASSISTANT_AMULET_PERCENT_EVENTID, 3, true);
	addLabel(window, 153, 487, "%");
	addLabel(window, 26, 516, "HP baixo ID:");
	addTextBox(window, 106, 512, 70, numberText(settings.lowHealthAmuletId), ASSISTANT_LOW_AMULET_ID_EVENTID, 5, true);
	addLabel(window, 190, 516, "Sub:");
	addTextBox(window, 223, 512, 48, numberText(settings.lowHealthAmuletSubtype), ASSISTANT_LOW_AMULET_SUBTYPE_EVENTID, 3, true);
	addLabel(window, 26, 545, "HP alto ID:");
	addTextBox(window, 106, 541, 70, numberText(settings.highHealthAmuletId), ASSISTANT_HIGH_AMULET_ID_EVENTID, 5, true);
	addLabel(window, 190, 545, "Sub:");
	addTextBox(window, 223, 541, 48, numberText(settings.highHealthAmuletSubtype), ASSISTANT_HIGH_AMULET_SUBTYPE_EVENTID, 3, true);
	window->addChild(new GUI_Separator(iRect(13, 570, ASSISTANT_OPTIONS_WIDTH - 26, 2)));

	addCheckBox(window, 18, 579, 250, "Auto municao (slot arrows)", settings.ammunitionEnabled, ASSISTANT_AMMUNITION_ENABLED_EVENTID);
	addLabel(window, 26, 612, "Item ID:");
	addTextBox(window, 78, 608, 70, numberText(settings.ammunitionId), ASSISTANT_AMMUNITION_ID_EVENTID, 5, true);
	addLabel(window, 165, 612, "Bolt 3446 | Burst 3449 | Power 3450");
	addLabel(window, 26, 637, "Equipa outra pilha automaticamente quando o slot ficar vazio.");

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

void autoRuneOptions_Events(Uint32 event, Sint32)
{
	GUI_Window* window = g_engine.getCurrentWindow();
	if(!window || window->getInternalID() != GUI_WINDOW_AUTORUNEOPTIONS)
		return;

	if(event == AUTORUNE_CANCEL_EVENTID)
	{
		g_engine.removeWindow(window);
		return;
	}
	if(event != AUTORUNE_OK_EVENTID)
		return;

	AssistantSettings settings = g_assistant.getSettings();
	settings.autoRuneEnabled = getChecked(window, AUTORUNE_ENABLED_EVENTID);
	settings.runeManaTraining = getChecked(window, AUTORUNE_MANA_TRAINING_EVENTID);
	settings.runeSpell = getText(window, AUTORUNE_SPELL_EVENTID);
	settings.runeMinimumMana = getNumber(window, AUTORUNE_MINIMUM_MANA_EVENTID, 1000000);
	settings.runeAntiIdle = getChecked(window, AUTORUNE_ANTI_IDLE_EVENTID);
	settings.runeAutoFood = getChecked(window, AUTORUNE_AUTO_FOOD_EVENTID);
	settings.runeFoodIds = getText(window, AUTORUNE_FOOD_IDS_EVENTID);
	settings.runeLogoutNoBlank = getChecked(window, AUTORUNE_LOGOUT_BLANK_EVENTID);
	settings.blankRuneId = SDL_static_cast(Uint16, getNumber(window, AUTORUNE_BLANK_ID_EVENTID, 65535));
	settings.runeLogoutNonVip = getChecked(window, AUTORUNE_LOGOUT_NONVIP_EVENTID);
	settings.runeTrustedPlayers = getText(window, AUTORUNE_TRUSTED_PLAYERS_EVENTID);
	g_assistant.applySettings(settings);
	g_assistant.saveConfig();
	g_engine.removeWindow(window);
}

void UTIL_autoRuneOptions()
{
	GUI_Window* existingWindow = g_engine.getWindow(GUI_WINDOW_AUTORUNEOPTIONS);
	if(existingWindow)
	{
		g_engine.removeWindow(existingWindow);
		return;
	}

	const AssistantSettings& settings = g_assistant.getSettings();
	GUI_Window* window = new GUI_Window(iRect(0, 0, AUTORUNE_OPTIONS_WIDTH, AUTORUNE_OPTIONS_HEIGHT), "Auto Rune", GUI_WINDOW_AUTORUNEOPTIONS);
	addCheckBox(window, 18, 30, 160, "Ativar Auto Rune", settings.autoRuneEnabled, AUTORUNE_ENABLED_EVENTID);
	window->addChild(new GUI_Label(iRect(190, 34, 0, 0), (settings.autoRuneEnabled ? "Auto ON" : "Auto OFF"), 0,
		(settings.autoRuneEnabled ? 70 : 230), (settings.autoRuneEnabled ? 220 : 60), 60));
	window->addChild(new GUI_Separator(iRect(13, 59, AUTORUNE_OPTIONS_WIDTH - 26, 2)));

	addCheckBox(window, 18, 68, 180, "Mana Training", settings.runeManaTraining, AUTORUNE_MANA_TRAINING_EVENTID);
	addLabel(window, 26, 101, "Magia:");
	addTextBox(window, 71, 97, 170, settings.runeSpell, AUTORUNE_SPELL_EVENTID, 80);
	addLabel(window, 255, 101, "Mana >=");
	addTextBox(window, 310, 97, 65, numberText(settings.runeMinimumMana), AUTORUNE_MINIMUM_MANA_EVENTID, 7, true);
	addCheckBox(window, 18, 126, 130, "Anti-Idle", settings.runeAntiIdle, AUTORUNE_ANTI_IDLE_EVENTID);
	addCheckBox(window, 155, 126, 145, "Auto-Eat Food", settings.runeAutoFood, AUTORUNE_AUTO_FOOD_EVENTID);
	addLabel(window, 26, 159, "IDs comida:");
	addTextBox(window, 94, 155, 281, settings.runeFoodIds, AUTORUNE_FOOD_IDS_EVENTID, 180);
	window->addChild(new GUI_Label(iRect(26, 181, 0, 0), "Use IDs separados por virgula. As mochilas com comida devem estar abertas."));
	window->addChild(new GUI_Separator(iRect(13, 204, AUTORUNE_OPTIONS_WIDTH - 26, 2)));

	addCheckBox(window, 18, 213, 235, "Logout sem blank rune", settings.runeLogoutNoBlank, AUTORUNE_LOGOUT_BLANK_EVENTID);
	addLabel(window, 260, 217, "ID:");
	addTextBox(window, 282, 213, 65, numberText(settings.blankRuneId), AUTORUNE_BLANK_ID_EVENTID, 5, true);
	addCheckBox(window, 18, 242, 300, "Logout ao aparecer jogador sem <3 na VIP", settings.runeLogoutNonVip, AUTORUNE_LOGOUT_NONVIP_EVENTID);
	addLabel(window, 26, 275, "Nomes permitidos fora da VIP:");
	addTextBox(window, 26, 291, 349, settings.runeTrustedPlayers, AUTORUNE_TRUSTED_PLAYERS_EVENTID, 250);
	window->addChild(new GUI_Label(iRect(26, 315, 0, 0), "Separe os nomes por virgula. Ex.: Gamemaster, Tutor"));
	window->addChild(new GUI_Label(iRect(26, 338, 0, 0), "A protecao contra jogador inicia 8 segundos apos o login."));

	window->addChild(new GUI_Separator(iRect(13, AUTORUNE_OPTIONS_HEIGHT - 40, AUTORUNE_OPTIONS_WIDTH - 26, 2)));
	GUI_Button* button = new GUI_Button(iRect(AUTORUNE_OPTIONS_WIDTH - 109, AUTORUNE_OPTIONS_HEIGHT - 30, GUI_UI_BUTTON_43PX_GREEN_UP_W, GUI_UI_BUTTON_43PX_GREEN_UP_H), "Salvar", CLIENT_GUI_ENTER_TRIGGER);
	button->setButtonEventCallback(&autoRuneOptions_Events, AUTORUNE_OK_EVENTID);
	button->startEvents();
	window->addChild(button);
	button = new GUI_Button(iRect(AUTORUNE_OPTIONS_WIDTH - 56, AUTORUNE_OPTIONS_HEIGHT - 30, GUI_UI_BUTTON_43PX_GRAY_UP_W, GUI_UI_BUTTON_43PX_GRAY_UP_H), "Fechar", CLIENT_GUI_ESCAPE_TRIGGER);
	button->setButtonEventCallback(&autoRuneOptions_Events, AUTORUNE_CANCEL_EVENTID);
	button->startEvents();
	window->addChild(button);
	g_engine.addWindow(window, true);
}
