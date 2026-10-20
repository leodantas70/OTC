/* Configuration window for the native CaveBot. */

#include "GUI_UTIL.h"
#include "../cavebot.h"
#include "../creature.h"
#include "../engine.h"
#include "../game.h"
#include "../map.h"
#include "../thingManager.h"
#include "itemUI.h"
#include "../GUI_Elements/GUI_Window.h"
#include "../GUI_Elements/GUI_Button.h"
#include "../GUI_Elements/GUI_CheckBox.h"
#include "../GUI_Elements/GUI_Label.h"
#include "../GUI_Elements/GUI_ListBox.h"
#include "../GUI_Elements/GUI_Separator.h"
#include "../GUI_Elements/GUI_TextBox.h"

#include <algorithm>
#include <cctype>
#include <memory>
#include <unordered_map>

#define CAVEBOT_OPTIONS_WIDTH 580
#define CAVEBOT_OPTIONS_HEIGHT 742

#define CAVEBOT_CANCEL_EVENTID 1000
#define CAVEBOT_OK_EVENTID 1001
#define CAVEBOT_ADD_WALK_EVENTID 1002
#define CAVEBOT_ADD_STAND_EVENTID 1003
#define CAVEBOT_ADD_USE_EVENTID 1004
#define CAVEBOT_ADD_EXANI_EVENTID 1005
#define CAVEBOT_ADD_SAY_EVENTID 1006
#define CAVEBOT_SHIFT_UP_EVENTID 1007
#define CAVEBOT_SHIFT_DOWN_EVENTID 1008
#define CAVEBOT_SHIFT_LEFT_EVENTID 1009
#define CAVEBOT_SHIFT_RIGHT_EVENTID 1010
#define CAVEBOT_REMOVE_EVENTID 1011
#define CAVEBOT_CLEAR_EVENTID 1012
#define CAVEBOT_LOOT_ITEM_LIST_EVENTID 1013

#define CAVEBOT_ENABLED_EVENTID 1100
#define CAVEBOT_ROUTE_ENABLED_EVENTID 1101
#define CAVEBOT_LOOP_EVENTID 1102
#define CAVEBOT_PAUSE_TARGET_EVENTID 1103
#define CAVEBOT_TARGET_ENABLED_EVENTID 1104
#define CAVEBOT_LOOT_ENABLED_EVENTID 1105
#define CAVEBOT_ALERT_ENABLED_EVENTID 1106
#define CAVEBOT_PAUSE_PLAYER_EVENTID 1107
#define CAVEBOT_RECORD_EVENTID 1108
#define CAVEBOT_LOOT_GOLD_EVENTID 1109
#define CAVEBOT_LOOT_AFTER_KILL_EVENTID 1110
#define CAVEBOT_BURST_ARROW_EVENTID 1111
#define CAVEBOT_DROP_LOOT_NO_CAP_EVENTID 1112
#define CAVEBOT_DROP_GOLD_NO_CAP_EVENTID 1113

#define CAVEBOT_REACH_EVENTID 1200
#define CAVEBOT_RETRY_EVENTID 1201
#define CAVEBOT_ACTION_ITEM_EVENTID 1202
#define CAVEBOT_ACTION_SUBTYPE_EVENTID 1203
#define CAVEBOT_ACTION_DELAY_EVENTID 1204
#define CAVEBOT_ACTION_TEXT_EVENTID 1205
#define CAVEBOT_TARGET_NAMES_EVENTID 1206
#define CAVEBOT_RUNE_ID_EVENTID 1207
#define CAVEBOT_RUNE_SUBTYPE_EVENTID 1208
#define CAVEBOT_ATTACK_DELAY_EVENTID 1209
#define CAVEBOT_LOOT_IDS_EVENTID 1210
#define CAVEBOT_LOOT_DESTINATION_EVENTID 1211
#define CAVEBOT_IGNORED_PLAYERS_EVENTID 1212
#define CAVEBOT_DROP_LOOT_CAPACITY_EVENTID 1213
#define CAVEBOT_DROP_GOLD_CAPACITY_EVENTID 1214
#define CAVEBOT_WAYPOINT_LIST_EVENTID 1300
#define CAVEBOT_TARGET_MOVEMENT_LIST_EVENTID 1301

#define CAVEBOT_LOOT_CATALOG_LIST_EVENTID 1400
#define CAVEBOT_LOOT_CATALOG_ADD_EVENTID 1401
#define CAVEBOT_LOOT_CATALOG_CLOSE_EVENTID 1402
#define CAVEBOT_LOOT_CATALOG_SEARCH_EVENTID 1403
#define CAVEBOT_LOOT_CATALOG_FIND_EVENTID 1404
#define CAVEBOT_LOOT_CATALOG_PREVIEW_EVENTID 1405
#define CAVEBOT_LOOT_CATALOG_SELECTED_EVENTID 1406

#define CAVEBOT_LOOT_CATALOG_WIDTH 420
#define CAVEBOT_LOOT_CATALOG_HEIGHT 455

extern Engine g_engine;
extern Game g_game;
extern Map g_map;
extern ThingManager g_thingManager;
extern std::string g_basePath;

void caveBotOptions_Events(Uint32 event, Sint32 status);
void caveBotLootItems_Events(Uint32 event, Sint32 status);

namespace
{
	std::vector<Uint16> g_lootCatalogItemIds;
	std::vector<Uint16> g_lootCatalogAllItemIds;
	std::unordered_map<Uint16, std::string> g_lootCatalogNames;
	Sint32 g_lootCatalogSelection = 0;
	bool g_lootCatalogLoaded = false;
	std::string lowerText(std::string text);
	const CaveBotTargetMovement g_targetMovementModes[] = {
		CAVEBOT_TARGET_STAND,
		CAVEBOT_TARGET_FOLLOW,
		CAVEBOT_TARGET_DIAGONAL,
		CAVEBOT_TARGET_STAND_DIAGONAL,
		CAVEBOT_TARGET_DIAGONAL_DISTANCE_2,
		CAVEBOT_TARGET_DIAGONAL_DISTANCE_3,
		CAVEBOT_TARGET_DIAGONAL_DISTANCE_4,
		CAVEBOT_TARGET_DIAGONAL_DISTANCE_5
	};

	Sint32 targetMovementSelection(CaveBotTargetMovement movement)
	{
		for(size_t index = 0; index < sizeof(g_targetMovementModes) / sizeof(g_targetMovementModes[0]); ++index)
		{
			if(g_targetMovementModes[index] == movement)
				return SDL_static_cast(Sint32, index);
		}
		return 0;
	}

	class GUI_CaveBotLootPreview : public GUI_Element
	{
		public:
			GUI_CaveBotLootPreview(iRect boxRect, Uint32 internalID = 0)
			{
				setRect(boxRect);
				m_internalID = internalID;
			}

			void setItem(Uint16 itemId)
			{
				m_item.reset();
				if(itemId != 0)
					m_item.reset(ItemUI::createItemUI(itemId, 1));
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

		private:
			std::unique_ptr<ItemUI> m_item;
	};

	std::string lootCatalogItemText(Uint16 itemId)
	{
		auto catalogName = g_lootCatalogNames.find(itemId);
		if(catalogName != g_lootCatalogNames.end() && !catalogName->second.empty())
			return catalogName->second;

		ThingType* type = g_thingManager.getThingType(ThingCategory_Item, itemId);
		if(!type)
			return "Item sem nome";

		std::string name = type->m_marketData.m_name;
		if(name.empty() && !type->m_salesData.empty())
			name = type->m_salesData.front().m_name;
		return (name.empty() ? "Item sem nome" : name);
	}

	void loadLootItemCatalog()
	{
		if(g_lootCatalogLoaded)
			return;
		g_lootCatalogLoaded = true;

		std::string path = g_basePath + ASSETS_CATALOG + PATH_PLATFORM_SLASH + "itemprices.ofip";
		SDL_RWops* file = SDL_RWFromFile(path.c_str(), "rb");
		if(!file)
			return;

		char magic[4];
		Uint8 version = 0;
		if(SDL_RWread(file, magic, 1, sizeof(magic)) != sizeof(magic) ||
			SDL_memcmp(magic, "OFIP", sizeof(magic)) != 0 ||
			SDL_RWread(file, &version, 1, 1) != 1 || version != 1)
		{
			SDL_RWclose(file);
			return;
		}

		Uint32 count = SDL_ReadLE32(file);
		count = UTIL_min<Uint32>(count, 65535);
		for(Uint32 i = 0; i < count; ++i)
		{
			Uint16 itemId = SDL_ReadLE16(file);
			SDL_ReadLE32(file); // Preco de referencia armazenado no catalogo OFIP.
			Uint8 nameLength = 0;
			if(SDL_RWread(file, &nameLength, 1, 1) != 1)
				break;
			std::string name(nameLength, '\0');
			if(nameLength != 0 && SDL_RWread(file, &name[0], 1, nameLength) != nameLength)
				break;
			if(itemId == 0 || name.empty())
				continue;

			if(g_lootCatalogNames.find(itemId) == g_lootCatalogNames.end())
				g_lootCatalogAllItemIds.push_back(itemId);
			g_lootCatalogNames[itemId] = name;
		}
		SDL_RWclose(file);

		std::stable_sort(g_lootCatalogAllItemIds.begin(), g_lootCatalogAllItemIds.end(), [](Uint16 left, Uint16 right) {
			std::string leftName = lowerText(lootCatalogItemText(left));
			std::string rightName = lowerText(lootCatalogItemText(right));
			return (leftName == rightName ? left < right : leftName < rightName);
		});
	}

	std::string lowerText(std::string text)
	{
		std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {return SDL_static_cast(char, std::tolower(c));});
		return text;
	}

	std::string appendLootItemId(const std::string& currentText, Uint16 itemId)
	{
		std::vector<Uint16> ids;
		Uint32 value = 0;
		bool readingNumber = false;
		for(size_t i = 0; i <= currentText.size(); ++i)
		{
			char character = (i < currentText.size() ? currentText[i] : ',');
			if(character >= '0' && character <= '9')
			{
				value = UTIL_min<Uint32>(65535, value * 10 + SDL_static_cast(Uint32, character - '0'));
				readingNumber = true;
			}
			else if(readingNumber)
			{
				Uint16 parsed = SDL_static_cast(Uint16, value);
				if(parsed != 0 && std::find(ids.begin(), ids.end(), parsed) == ids.end())
					ids.push_back(parsed);
				value = 0;
				readingNumber = false;
			}
		}
		if(itemId != 0 && std::find(ids.begin(), ids.end(), itemId) == ids.end())
			ids.push_back(itemId);

		std::string result;
		for(Uint16 id : ids)
		{
			if(!result.empty())
				result += ',';
			result += std::to_string(SDL_static_cast(Uint32, id));
		}
		return result;
	}

	void refreshLootCatalogSelection(GUI_Window* window, Sint32 selection)
	{
		if(!window || g_lootCatalogItemIds.empty())
			return;
		selection = UTIL_max<Sint32>(0, UTIL_min<Sint32>(SDL_static_cast(Sint32, g_lootCatalogItemIds.size()) - 1, selection));
		g_lootCatalogSelection = selection;
		Uint16 itemId = g_lootCatalogItemIds[SDL_static_cast(size_t, selection)];
		GUI_CaveBotLootPreview* preview = SDL_static_cast(GUI_CaveBotLootPreview*, window->getChild(CAVEBOT_LOOT_CATALOG_PREVIEW_EVENTID));
		if(preview)
			preview->setItem(itemId);
		GUI_Label* label = SDL_static_cast(GUI_Label*, window->getChild(CAVEBOT_LOOT_CATALOG_SELECTED_EVENTID));
		if(label)
			label->setName(std::string("Selecionado: ") + lootCatalogItemText(itemId));
	}

	void openLootItemCatalog(const std::string& searchText = std::string())
	{
		GUI_Window* oldWindow = g_engine.getWindow(GUI_WINDOW_CAVEBOTLOOTITEMS);
		if(oldWindow)
			g_engine.removeWindow(oldWindow);

		loadLootItemCatalog();
		if(g_lootCatalogAllItemIds.empty())
		{
			for(Uint32 rawId = 1; rawId <= 65535; ++rawId)
			{
				Uint16 itemId = SDL_static_cast(Uint16, rawId);
				if(g_thingManager.isValidDatId(ThingCategory_Item, itemId))
					g_lootCatalogAllItemIds.push_back(itemId);
			}
		}
		g_lootCatalogItemIds.clear();
		std::string wantedName = lowerText(searchText);
		for(Uint16 itemId : g_lootCatalogAllItemIds)
		{
			if(wantedName.empty() || lowerText(lootCatalogItemText(itemId)).find(wantedName) != std::string::npos)
				g_lootCatalogItemIds.push_back(itemId);
		}

		GUI_Window* window = new GUI_Window(iRect(0, 0, CAVEBOT_LOOT_CATALOG_WIDTH, CAVEBOT_LOOT_CATALOG_HEIGHT), "Lista de itens para loot", GUI_WINDOW_CAVEBOTLOOTITEMS);
		window->addChild(new GUI_Label(iRect(18, 31, 0, 0), "Buscar nome:"));
		GUI_TextBox* search = new GUI_TextBox(iRect(94, 27, 150, 16), searchText, CAVEBOT_LOOT_CATALOG_SEARCH_EVENTID);
		search->setMaxLength(80);
		search->startEvents();
		window->addChild(search);
		GUI_Button* button = new GUI_Button(iRect(254, 25, GUI_UI_BUTTON_86PX_GRAY_UP_W, GUI_UI_BUTTON_86PX_GRAY_UP_H), "Buscar");
		button->setButtonEventCallback(&caveBotLootItems_Events, CAVEBOT_LOOT_CATALOG_FIND_EVENTID);
		button->startEvents();
		window->addChild(button);

		GUI_ListBox* list = new GUI_ListBox(iRect(18, 57, 265, 340), CAVEBOT_LOOT_CATALOG_LIST_EVENTID);
		for(Uint16 itemId : g_lootCatalogItemIds)
			list->add(lootCatalogItemText(itemId));
		list->setEventCallback(&caveBotLootItems_Events, CAVEBOT_LOOT_CATALOG_LIST_EVENTID);
		if(!g_lootCatalogItemIds.empty())
			list->setSelect(0);
		list->startEvents();
		window->addChild(list);

		GUI_CaveBotLootPreview* preview = new GUI_CaveBotLootPreview(iRect(315, 57, 52, 52), CAVEBOT_LOOT_CATALOG_PREVIEW_EVENTID);
		window->addChild(preview);
		window->addChild(new GUI_Label(iRect(300, 121, 0, 0), "Selecionado: -", CAVEBOT_LOOT_CATALOG_SELECTED_EVENTID));
		window->addChild(new GUI_Label(iRect(300, 146, 0, 0), "Selecione um item"));
		window->addChild(new GUI_Label(iRect(300, 160, 0, 0), "e clique Add."));

		button = new GUI_Button(iRect(300, 190, GUI_UI_BUTTON_86PX_GREEN_UP_W, GUI_UI_BUTTON_86PX_GREEN_UP_H), "Add");
		button->setButtonEventCallback(&caveBotLootItems_Events, CAVEBOT_LOOT_CATALOG_ADD_EVENTID);
		button->startEvents();
		window->addChild(button);

		window->addChild(new GUI_Separator(iRect(13, CAVEBOT_LOOT_CATALOG_HEIGHT - 40, CAVEBOT_LOOT_CATALOG_WIDTH - 26, 2)));
		button = new GUI_Button(iRect(CAVEBOT_LOOT_CATALOG_WIDTH - 56, CAVEBOT_LOOT_CATALOG_HEIGHT - 30, GUI_UI_BUTTON_43PX_GRAY_UP_W, GUI_UI_BUTTON_43PX_GRAY_UP_H), "Fechar", CLIENT_GUI_ESCAPE_TRIGGER);
		button->setButtonEventCallback(&caveBotLootItems_Events, CAVEBOT_LOOT_CATALOG_CLOSE_EVENTID);
		button->startEvents();
		window->addChild(button);

		g_engine.addWindow(window, true);
		if(!g_lootCatalogItemIds.empty())
			refreshLootCatalogSelection(window, 0);
		else
		{
			GUI_Label* label = SDL_static_cast(GUI_Label*, window->getChild(CAVEBOT_LOOT_CATALOG_SELECTED_EVENTID));
			if(label)
				label->setName("Nenhum item encontrado");
		}
	}

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

	void addStatus(GUI_Window* window, Sint32 x, Sint32 y, const char* name, bool active)
	{
		std::string label = std::string(name) + (active ? ": ATIVO" : ": INATIVO");
		window->addChild(new GUI_Label(iRect(x, y, 0, 0), label, 0, active ? 80 : 235, active ? 230 : 70, 70));
	}

	void addTextBox(GUI_Window* window, Sint32 x, Sint32 y, Sint32 width, const std::string& text, Uint32 id, Uint32 maxLength, bool numbersOnly = false)
	{
		GUI_TextBox* textBox = new GUI_TextBox(iRect(x, y, width, 16), text, id);
		textBox->setMaxLength(maxLength);
		textBox->setOnlyNumbers(numbersOnly);
		textBox->startEvents();
		window->addChild(textBox);
	}

	void addButton(GUI_Window* window, Sint32 x, Sint32 y, const char* label, Uint32 event)
	{
		GUI_Button* button = new GUI_Button(iRect(x, y, GUI_UI_BUTTON_86PX_GRAY_UP_W, GUI_UI_BUTTON_86PX_GRAY_UP_H), label);
		button->setButtonEventCallback(&caveBotOptions_Events, event);
		button->startEvents();
		window->addChild(button);
	}

	CaveBotSettings readSettings(GUI_Window* window)
	{
		CaveBotSettings settings = g_caveBot.getSettings();
		settings.enabled = getChecked(window, CAVEBOT_ENABLED_EVENTID);
		settings.caveEnabled = getChecked(window, CAVEBOT_ROUTE_ENABLED_EVENTID);
		settings.recordEnabled = getChecked(window, CAVEBOT_RECORD_EVENTID);
		settings.loop = getChecked(window, CAVEBOT_LOOP_EVENTID);
		settings.pauseWhileTarget = getChecked(window, CAVEBOT_PAUSE_TARGET_EVENTID);
		settings.reachDistance = SDL_static_cast(Uint8, getNumber(window, CAVEBOT_REACH_EVENTID, 5));
		settings.retryDelay = getNumber(window, CAVEBOT_RETRY_EVENTID, 10000);
		settings.targetEnabled = getChecked(window, CAVEBOT_TARGET_ENABLED_EVENTID);
		settings.targetNames = getText(window, CAVEBOT_TARGET_NAMES_EVENTID);
		settings.attackRuneId = SDL_static_cast(Uint16, getNumber(window, CAVEBOT_RUNE_ID_EVENTID, 65535));
		settings.attackRuneSubtype = SDL_static_cast(Uint8, getNumber(window, CAVEBOT_RUNE_SUBTYPE_EVENTID, 255));
		settings.attackDelay = getNumber(window, CAVEBOT_ATTACK_DELAY_EVENTID, 10000);
		GUI_ListBox* movementList = SDL_static_cast(GUI_ListBox*, window->getChild(CAVEBOT_TARGET_MOVEMENT_LIST_EVENTID));
		if(movementList)
		{
			Sint32 selection = movementList->getSelect();
			if(selection >= 0 && SDL_static_cast(size_t, selection) < sizeof(g_targetMovementModes) / sizeof(g_targetMovementModes[0]))
				settings.targetMovement = g_targetMovementModes[selection];
		}
		settings.lootEnabled = getChecked(window, CAVEBOT_LOOT_ENABLED_EVENTID);
		settings.lootGold = getChecked(window, CAVEBOT_LOOT_GOLD_EVENTID);
		settings.lootAfterKill = getChecked(window, CAVEBOT_LOOT_AFTER_KILL_EVENTID);
		settings.dropLootNoCapacity = getChecked(window, CAVEBOT_DROP_LOOT_NO_CAP_EVENTID);
		settings.dropLootCapacity = getNumber(window, CAVEBOT_DROP_LOOT_CAPACITY_EVENTID, 100000);
		settings.dropGoldNoCapacity = getChecked(window, CAVEBOT_DROP_GOLD_NO_CAP_EVENTID);
		settings.dropGoldCapacity = getNumber(window, CAVEBOT_DROP_GOLD_CAPACITY_EVENTID, 100000);
		settings.lootItemIds = getText(window, CAVEBOT_LOOT_IDS_EVENTID);
		settings.lootDestinationContainer = SDL_static_cast(Uint8, getNumber(window, CAVEBOT_LOOT_DESTINATION_EVENTID, GAME_MAX_CONTAINERS - 1));
		settings.playerAlertEnabled = getChecked(window, CAVEBOT_ALERT_ENABLED_EVENTID);
		settings.pauseOnPlayer = getChecked(window, CAVEBOT_PAUSE_PLAYER_EVENTID);
		settings.ignoredPlayers = getText(window, CAVEBOT_IGNORED_PLAYERS_EVENTID);
		settings.burstArrowEnabled = getChecked(window, CAVEBOT_BURST_ARROW_EVENTID);
		return settings;
	}

	void saveWindowSettings(GUI_Window* window)
	{
		g_caveBot.applySettings(readSettings(window));
		g_caveBot.saveConfig();
	}

	Sint32 selectedWaypoint(GUI_Window* window)
	{
		GUI_ListBox* list = SDL_static_cast(GUI_ListBox*, window->getChild(CAVEBOT_WAYPOINT_LIST_EVENTID));
		return (list ? list->getSelect() : -1);
	}

	void reopenWindow(GUI_Window* window)
	{
		g_engine.removeWindow(window);
		UTIL_caveBotOptions();
	}
}

void caveBotLootItems_Events(Uint32 event, Sint32 status)
{
	GUI_Window* window = g_engine.getCurrentWindow();
	if(!window || window->getInternalID() != GUI_WINDOW_CAVEBOTLOOTITEMS)
		return;

	if(event == CAVEBOT_LOOT_CATALOG_CLOSE_EVENTID)
	{
		g_engine.removeWindow(window);
		return;
	}
	if(event == CAVEBOT_LOOT_CATALOG_LIST_EVENTID)
	{
		if(status < 0)
			status = ~status;
		refreshLootCatalogSelection(window, status);
		return;
	}
	if(event == CAVEBOT_LOOT_CATALOG_FIND_EVENTID)
	{
		GUI_TextBox* search = SDL_static_cast(GUI_TextBox*, window->getChild(CAVEBOT_LOOT_CATALOG_SEARCH_EVENTID));
		openLootItemCatalog(search ? search->getActualText() : std::string());
		return;
	}
	if(event == CAVEBOT_LOOT_CATALOG_ADD_EVENTID && !g_lootCatalogItemIds.empty())
	{
		Uint16 itemId = g_lootCatalogItemIds[SDL_static_cast(size_t, g_lootCatalogSelection)];
		GUI_Window* caveWindow = g_engine.getWindow(GUI_WINDOW_CAVEBOTOPTIONS);
		GUI_TextBox* ids = (caveWindow ? SDL_static_cast(GUI_TextBox*, caveWindow->getChild(CAVEBOT_LOOT_IDS_EVENTID)) : NULL);
		if(ids)
		{
			ids->setText(appendLootItemId(ids->getActualText(), itemId));
			GUI_Label* label = SDL_static_cast(GUI_Label*, window->getChild(CAVEBOT_LOOT_CATALOG_SELECTED_EVENTID));
			if(label)
				label->setName("Adicionado ao loot.");
		}
	}
}

void caveBotOptions_Events(Uint32 event, Sint32 status)
{
	GUI_Window* window = g_engine.getCurrentWindow();
	if(!window || window->getInternalID() != GUI_WINDOW_CAVEBOTOPTIONS)
		return;

	if(event == CAVEBOT_CANCEL_EVENTID)
	{
		g_engine.removeWindow(window);
		return;
	}
	if(event == CAVEBOT_OK_EVENTID)
	{
		saveWindowSettings(window);
		g_engine.removeWindow(window);
		UTIL_createInventoryPanel(false);
		g_engine.recalculateGameWindow();
		return;
	}
	if(event == CAVEBOT_LOOT_ITEM_LIST_EVENTID)
	{
		openLootItemCatalog();
		return;
	}
	if(event == CAVEBOT_WAYPOINT_LIST_EVENTID)
	{
		if(status < 0)
		{
			Sint32 waypointIndex = ~status;
			if(waypointIndex >= 0 && SDL_static_cast(size_t, waypointIndex) < g_caveBot.getWaypoints().size())
			{
				g_caveBot.setCurrentWaypoint(SDL_static_cast(size_t, waypointIndex));
				GUI_ListBox* waypointList = SDL_static_cast(GUI_ListBox*, window->getChild(CAVEBOT_WAYPOINT_LIST_EVENTID));
				if(waypointList)
				{
					for(size_t i = 0; i < g_caveBot.getWaypoints().size(); ++i)
						waypointList->set(SDL_static_cast(Sint32, i), g_caveBot.waypointDescription(i));
					waypointList->setSelect(waypointIndex);
				}
			}
		}
		return;
	}

	if(event >= CAVEBOT_ADD_WALK_EVENTID && event <= CAVEBOT_ADD_SAY_EVENTID)
	{
		Creature* player = g_map.getLocalCreature();
		if(!player)
			return;
		saveWindowSettings(window);
		CaveBotWaypoint waypoint;
		waypoint.position = player->getCurrentPosition();
		switch(event)
		{
			case CAVEBOT_ADD_STAND_EVENTID: waypoint.action = CAVEBOT_STAND; break;
			case CAVEBOT_ADD_USE_EVENTID: waypoint.action = CAVEBOT_USE; break;
			case CAVEBOT_ADD_EXANI_EVENTID: waypoint.action = CAVEBOT_EXANI; break;
			case CAVEBOT_ADD_SAY_EVENTID: waypoint.action = CAVEBOT_SAY; break;
			default: waypoint.action = CAVEBOT_WALK; break;
		}
		waypoint.itemId = SDL_static_cast(Uint16, getNumber(window, CAVEBOT_ACTION_ITEM_EVENTID, 65535));
		waypoint.itemSubtype = SDL_static_cast(Uint8, getNumber(window, CAVEBOT_ACTION_SUBTYPE_EVENTID, 255));
		waypoint.delay = getNumber(window, CAVEBOT_ACTION_DELAY_EVENTID, 60000);
		waypoint.text = getText(window, CAVEBOT_ACTION_TEXT_EVENTID);
		g_caveBot.addWaypoint(waypoint);
		reopenWindow(window);
		return;
	}

	Sint32 selected = selectedWaypoint(window);
	if(event >= CAVEBOT_SHIFT_UP_EVENTID && event <= CAVEBOT_SHIFT_RIGHT_EVENTID && selected >= 0)
	{
		if(event == CAVEBOT_SHIFT_UP_EVENTID)
			g_caveBot.shiftWaypoint(SDL_static_cast(size_t, selected), 0, -1);
		else if(event == CAVEBOT_SHIFT_DOWN_EVENTID)
			g_caveBot.shiftWaypoint(SDL_static_cast(size_t, selected), 0, 1);
		else if(event == CAVEBOT_SHIFT_LEFT_EVENTID)
			g_caveBot.shiftWaypoint(SDL_static_cast(size_t, selected), -1, 0);
		else
			g_caveBot.shiftWaypoint(SDL_static_cast(size_t, selected), 1, 0);
		GUI_ListBox* list = SDL_static_cast(GUI_ListBox*, window->getChild(CAVEBOT_WAYPOINT_LIST_EVENTID));
		if(list)
		{
			list->set(selected, g_caveBot.waypointDescription(SDL_static_cast(size_t, selected)));
			list->setSelect(selected);
		}
		return;
	}
	else if(event == CAVEBOT_REMOVE_EVENTID && selected >= 0)
		g_caveBot.removeWaypoint(SDL_static_cast(size_t, selected));
	else if(event == CAVEBOT_CLEAR_EVENTID)
		g_caveBot.clearWaypoints();
	else
		return;
	reopenWindow(window);
}

void UTIL_caveBotWaypointRecorded(const std::string& description)
{
	GUI_Window* window = g_engine.getWindow(GUI_WINDOW_CAVEBOTOPTIONS);
	if(!window)
		return;
	GUI_ListBox* list = SDL_static_cast(GUI_ListBox*, window->getChild(CAVEBOT_WAYPOINT_LIST_EVENTID));
	if(list)
	{
		list->add(description);
		list->setSelect(SDL_static_cast(Sint32, g_caveBot.getWaypoints().size()) - 1);
	}
}

void UTIL_caveBotOptions()
{
	GUI_Window* existingWindow = g_engine.getWindow(GUI_WINDOW_CAVEBOTOPTIONS);
	if(existingWindow)
	{
		g_engine.removeWindow(existingWindow);
		return;
	}

	const CaveBotSettings& settings = g_caveBot.getSettings();
	GUI_Window* window = new GUI_Window(iRect(0, 0, CAVEBOT_OPTIONS_WIDTH, CAVEBOT_OPTIONS_HEIGHT), "CaveBot", GUI_WINDOW_CAVEBOTOPTIONS);

	addCheckBox(window, 18, 27, 150, "Bot principal", settings.enabled, CAVEBOT_ENABLED_EVENTID);
	addStatus(window, 170, 32, "Rota", settings.enabled && settings.caveEnabled);
	addStatus(window, 265, 32, "Alvo", settings.enabled && settings.targetEnabled);
	addStatus(window, 360, 32, "Loot", settings.enabled && settings.lootEnabled);
	if(settings.enabled && settings.playerAlertEnabled && g_caveBot.isPlayerDetected())
		window->addChild(new GUI_Label(iRect(455, 32, 0, 0), "ALERTA: PLAYER!", 0, 255, 210, 30));
	else
		addStatus(window, 455, 32, "Alerta", settings.enabled && settings.playerAlertEnabled);
	window->addChild(new GUI_Separator(iRect(13, 61, CAVEBOT_OPTIONS_WIDTH - 26, 2)));

	addCheckBox(window, 18, 69, 125, "Caminhar rota", settings.caveEnabled, CAVEBOT_ROUTE_ENABLED_EVENTID);
	addCheckBox(window, 145, 69, 125, "Record caminho", settings.recordEnabled, CAVEBOT_RECORD_EVENTID);
	addCheckBox(window, 280, 69, 90, "Repetir", settings.loop, CAVEBOT_LOOP_EVENTID);
	addCheckBox(window, 382, 69, 180, "Pausar em combate", settings.pauseWhileTarget, CAVEBOT_PAUSE_TARGET_EVENTID);
	addLabel(window, 18, 105, "Distancia para concluir:");
	addTextBox(window, 148, 101, 35, numberText(settings.reachDistance), CAVEBOT_REACH_EVENTID, 1, true);
	addLabel(window, 215, 105, "Tentar caminho a cada (ms):");
	addTextBox(window, 371, 101, 70, numberText(settings.retryDelay), CAVEBOT_RETRY_EVENTID, 5, true);

	GUI_ListBox* list = new GUI_ListBox(iRect(18, 130, CAVEBOT_OPTIONS_WIDTH - 36, 130), CAVEBOT_WAYPOINT_LIST_EVENTID);
	for(size_t i = 0; i < g_caveBot.getWaypoints().size(); ++i)
		list->add(g_caveBot.waypointDescription(i));
	if(!g_caveBot.getWaypoints().empty())
		list->setSelect(SDL_static_cast(Sint32, g_caveBot.getCurrentWaypoint()));
	list->setEventCallback(&caveBotOptions_Events, CAVEBOT_WAYPOINT_LIST_EVENTID);
	list->startEvents();
	window->addChild(list);
	addButton(window, 18, 270, "Add Walk", CAVEBOT_ADD_WALK_EVENTID);
	addButton(window, 112, 270, "Add Stand", CAVEBOT_ADD_STAND_EVENTID);
	addButton(window, 206, 270, "Add Use", CAVEBOT_ADD_USE_EVENTID);
	addButton(window, 300, 270, "Add Exani", CAVEBOT_ADD_EXANI_EVENTID);
	addButton(window, 394, 270, "Add Say", CAVEBOT_ADD_SAY_EVENTID);
	addButton(window, 18, 295, "Cima ^", CAVEBOT_SHIFT_UP_EVENTID);
	addButton(window, 112, 295, "Baixo v", CAVEBOT_SHIFT_DOWN_EVENTID);
	addButton(window, 206, 295, "< Esq.", CAVEBOT_SHIFT_LEFT_EVENTID);
	addButton(window, 300, 295, "Dir. >", CAVEBOT_SHIFT_RIGHT_EVENTID);
	addButton(window, 394, 295, "Excluir", CAVEBOT_REMOVE_EVENTID);
	addButton(window, 488, 295, "Limpar", CAVEBOT_CLEAR_EVENTID);
	addLabel(window, 18, 330, "Acao - Item ID:");
	addTextBox(window, 111, 326, 65, "0", CAVEBOT_ACTION_ITEM_EVENTID, 5, true);
	addLabel(window, 190, 330, "Sub:");
	addTextBox(window, 223, 326, 40, "0", CAVEBOT_ACTION_SUBTYPE_EVENTID, 3, true);
	addLabel(window, 278, 330, "Espera ms:");
	addTextBox(window, 342, 326, 65, "300", CAVEBOT_ACTION_DELAY_EVENTID, 5, true);
	addLabel(window, 18, 352, "Texto do Say:");
	addTextBox(window, 96, 348, 310, std::string(), CAVEBOT_ACTION_TEXT_EVENTID, 120);
	window->addChild(new GUI_Separator(iRect(13, 374, CAVEBOT_OPTIONS_WIDTH - 26, 2)));

	addCheckBox(window, 18, 382, 130, "TargetBot", settings.targetEnabled, CAVEBOT_TARGET_ENABLED_EVENTID);
	addLabel(window, 155, 387, "Monstros (* = todos):");
	addTextBox(window, 275, 383, 285, settings.targetNames, CAVEBOT_TARGET_NAMES_EVENTID, 200);
	addLabel(window, 18, 416, "Runa ID:");
	addTextBox(window, 71, 412, 65, numberText(settings.attackRuneId), CAVEBOT_RUNE_ID_EVENTID, 5, true);
	addLabel(window, 151, 416, "Sub:");
	addTextBox(window, 184, 412, 40, numberText(settings.attackRuneSubtype), CAVEBOT_RUNE_SUBTYPE_EVENTID, 3, true);
	addLabel(window, 240, 416, "Intervalo (ms):");
	addTextBox(window, 327, 412, 65, numberText(settings.attackDelay), CAVEBOT_ATTACK_DELAY_EVENTID, 5, true);
	addLabel(window, 18, 449, "Movimento:");
	GUI_ListBox* movementList = new GUI_ListBox(iRect(88, 438, 180, 91), CAVEBOT_TARGET_MOVEMENT_LIST_EVENTID);
	movementList->add("Atacar parado");
	movementList->add("Seguir o alvo");
	movementList->add("Manter diagonal");
	movementList->add("Parado + diagonal");
	movementList->add("Diagonal 2 SQM");
	movementList->add("Diagonal 3 SQM");
	movementList->add("Diagonal 4 SQM");
	movementList->add("Diagonal 5 SQM");
	movementList->setSelect(targetMovementSelection(settings.targetMovement));
	movementList->startEvents();
	window->addChild(movementList);
	window->addChild(new GUI_Label(iRect(285, 443, 0, 0), "Escolha como o personagem se posiciona durante o combate."));
	window->addChild(new GUI_Separator(iRect(13, 534, CAVEBOT_OPTIONS_WIDTH - 26, 2)));

	addCheckBox(window, 18, 542, 120, "Auto Loot", settings.lootEnabled, CAVEBOT_LOOT_ENABLED_EVENTID);
	addCheckBox(window, 145, 542, 100, "Loot gold", settings.lootGold, CAVEBOT_LOOT_GOLD_EVENTID);
	addLabel(window, 255, 547, "IDs adicionais:");
	addTextBox(window, 340, 543, 220, settings.lootItemIds, CAVEBOT_LOOT_IDS_EVENTID, 300);
	addLabel(window, 18, 574, "Mochila destino aberta (0-15):");
	addTextBox(window, 189, 570, 35, numberText(settings.lootDestinationContainer), CAVEBOT_LOOT_DESTINATION_EVENTID, 2, true);
	addCheckBox(window, 245, 568, 150, "Loot after kill", settings.lootAfterKill, CAVEBOT_LOOT_AFTER_KILL_EVENTID);
	addButton(window, 474, 566, "Lista itens", CAVEBOT_LOOT_ITEM_LIST_EVENTID);
	addCheckBox(window, 18, 594, 175, "Itens no chao com cap <", settings.dropLootNoCapacity, CAVEBOT_DROP_LOOT_NO_CAP_EVENTID);
	addTextBox(window, 195, 590, 55, numberText(settings.dropLootCapacity), CAVEBOT_DROP_LOOT_CAPACITY_EVENTID, 6, true);
	addCheckBox(window, 280, 594, 175, "Gold no chao com cap <", settings.dropGoldNoCapacity, CAVEBOT_DROP_GOLD_NO_CAP_EVENTID);
	addTextBox(window, 458, 590, 55, numberText(settings.dropGoldCapacity), CAVEBOT_DROP_GOLD_CAPACITY_EVENTID, 6, true);
	window->addChild(new GUI_Separator(iRect(13, 622, CAVEBOT_OPTIONS_WIDTH - 26, 2)));

	addCheckBox(window, 18, 630, 145, "Alerta de player", settings.playerAlertEnabled, CAVEBOT_ALERT_ENABLED_EVENTID);
	addCheckBox(window, 180, 630, 190, "Pausar rota", settings.pauseOnPlayer, CAVEBOT_PAUSE_PLAYER_EVENTID);
	addCheckBox(window, 380, 630, 180, "Auto burst arrow", settings.burstArrowEnabled, CAVEBOT_BURST_ARROW_EVENTID);
	addLabel(window, 18, 663, "Ignorar nomes:");
	addTextBox(window, 104, 659, 456, settings.ignoredPlayers, CAVEBOT_IGNORED_PLAYERS_EVENTID, 250);
	addLabel(window, 18, 685, "Record grava cada SQM ao caminhar. Dois cliques selecionam o waypoint atual.");

	window->addChild(new GUI_Separator(iRect(13, CAVEBOT_OPTIONS_HEIGHT - 40, CAVEBOT_OPTIONS_WIDTH - 26, 2)));
	GUI_Button* button = new GUI_Button(iRect(CAVEBOT_OPTIONS_WIDTH - 109, CAVEBOT_OPTIONS_HEIGHT - 30, GUI_UI_BUTTON_43PX_GREEN_UP_W, GUI_UI_BUTTON_43PX_GREEN_UP_H), "Salvar", CLIENT_GUI_ENTER_TRIGGER);
	button->setButtonEventCallback(&caveBotOptions_Events, CAVEBOT_OK_EVENTID);
	button->startEvents();
	window->addChild(button);
	button = new GUI_Button(iRect(CAVEBOT_OPTIONS_WIDTH - 56, CAVEBOT_OPTIONS_HEIGHT - 30, GUI_UI_BUTTON_43PX_GRAY_UP_W, GUI_UI_BUTTON_43PX_GRAY_UP_H), "Fechar", CLIENT_GUI_ESCAPE_TRIGGER);
	button->setButtonEventCallback(&caveBotOptions_Events, CAVEBOT_CANCEL_EVENTID);
	button->startEvents();
	window->addChild(button);
	g_engine.addWindow(window, true);
}
