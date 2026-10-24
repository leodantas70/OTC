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
#define CAVEBOT_TARGET_LIST_EVENTID 1014

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

#define CAVEBOT_LOOT_CATALOG_AVAILABLE_EVENTID 1400
#define CAVEBOT_LOOT_CATALOG_ADD_EVENTID 1401
#define CAVEBOT_LOOT_CATALOG_CLOSE_EVENTID 1402
#define CAVEBOT_LOOT_CATALOG_AVAILABLE_SEARCH_EVENTID 1403
#define CAVEBOT_LOOT_CATALOG_SELECTED_SEARCH_EVENTID 1404
#define CAVEBOT_LOOT_CATALOG_SELECTED_EVENTID 1405
#define CAVEBOT_LOOT_CATALOG_REMOVE_EVENTID 1406
#define CAVEBOT_LOOT_CATALOG_AVAILABLE_COUNT_EVENTID 1407
#define CAVEBOT_LOOT_CATALOG_SELECTED_COUNT_EVENTID 1408

#define CAVEBOT_TARGET_CATALOG_AVAILABLE_EVENTID 1500
#define CAVEBOT_TARGET_CATALOG_SELECTED_EVENTID 1501
#define CAVEBOT_TARGET_CATALOG_ADD_EVENTID 1502
#define CAVEBOT_TARGET_CATALOG_REMOVE_EVENTID 1503
#define CAVEBOT_TARGET_CATALOG_CLOSE_EVENTID 1504
#define CAVEBOT_TARGET_CATALOG_AVAILABLE_SEARCH_EVENTID 1505
#define CAVEBOT_TARGET_CATALOG_SELECTED_SEARCH_EVENTID 1506
#define CAVEBOT_TARGET_CATALOG_AVAILABLE_COUNT_EVENTID 1507
#define CAVEBOT_TARGET_CATALOG_SELECTED_COUNT_EVENTID 1508

#define CAVEBOT_CATALOG_WIDTH 760
#define CAVEBOT_CATALOG_HEIGHT 510

extern Engine g_engine;
extern Game g_game;
extern Map g_map;
extern ThingManager g_thingManager;
extern std::string g_basePath;

void caveBotOptions_Events(Uint32 event, Sint32 status);
void caveBotLootItems_Events(Uint32 event, Sint32 status);
void caveBotTargets_Events(Uint32 event, Sint32 status);

namespace
{
	std::vector<Uint16> g_lootCatalogAllItemIds;
	std::unordered_map<Uint16, std::string> g_lootCatalogNames;
	std::vector<Uint16> g_lootCatalogAvailableIds;
	std::vector<Uint16> g_lootCatalogSelectedIds;
	std::vector<Uint16> g_lootCatalogVisibleSelectedIds;
	Sint32 g_lootCatalogAvailableSelection = -1;
	Sint32 g_lootCatalogSelectedSelection = -1;
	bool g_lootCatalogLoaded = false;

	struct CaveBotMonsterCatalogEntry
	{
		std::string name;
		std::string key;
		Uint16 lookType = 0;
		Uint32 outfitColor = 0;
	};

	std::vector<CaveBotMonsterCatalogEntry> g_monsterCatalog;
	std::vector<size_t> g_monsterCatalogAvailable;
	std::vector<size_t> g_monsterCatalogVisibleSelected;
	std::vector<std::string> g_monsterCatalogSelected;
	Sint32 g_monsterCatalogAvailableSelection = -1;
	Sint32 g_monsterCatalogSelectedSelection = -1;
	bool g_monsterCatalogLoaded = false;
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

	std::vector<Uint16> parseLootItemIds(const std::string& text)
	{
		std::vector<Uint16> ids;
		Uint32 value = 0;
		bool readingNumber = false;
		for(size_t i = 0; i <= text.size(); ++i)
		{
			char character = (i < text.size() ? text[i] : ',');
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
		return ids;
	}

	std::string serializeLootItemIds(const std::vector<Uint16>& ids)
	{
		std::string result;
		for(Uint16 id : ids)
		{
			if(!result.empty())
				result += ',';
			result += std::to_string(SDL_static_cast(Uint32, id));
		}
		return result;
	}

	std::string trimText(const std::string& text)
	{
		size_t first = text.find_first_not_of(" \t\r\n");
		if(first == std::string::npos)
			return std::string();
		size_t last = text.find_last_not_of(" \t\r\n");
		return text.substr(first, last - first + 1);
	}

	std::string titleText(std::string text)
	{
		bool upper = true;
		for(char& character : text)
		{
			if(upper && character >= 'a' && character <= 'z')
				character = SDL_static_cast(char, character - ('a' - 'A'));
			upper = (character == ' ' || character == '-' || character == '\'');
		}
		return text;
	}

	std::vector<std::string> parseNameList(const std::string& text)
	{
		std::vector<std::string> names;
		size_t start = 0;
		while(start <= text.size())
		{
			size_t end = text.find(',', start);
			std::string name = trimText(text.substr(start, end == std::string::npos ? std::string::npos : end - start));
			if(!name.empty())
			{
				std::string key = lowerText(name);
				bool exists = std::any_of(names.begin(), names.end(), [&key](const std::string& current) {return lowerText(current) == key;});
				if(!exists)
					names.push_back(name == "*" ? "*" : titleText(name));
			}
			if(end == std::string::npos)
				break;
			start = end + 1;
		}
		return names;
	}

	std::string serializeNameList(const std::vector<std::string>& names)
	{
		std::string result;
		for(const std::string& name : names)
		{
			if(!result.empty())
				result += ',';
			result += name;
		}
		return result;
	}

	void setCatalogCount(GUI_Window* window, Uint32 id, size_t count, const char* singular, const char* plural)
	{
		GUI_Label* label = SDL_static_cast(GUI_Label*, window->getChild(id));
		if(label)
			label->setName(std::to_string(count) + (count == 1 ? singular : plural));
	}

	void refreshLootCatalogLists(GUI_Window* window)
	{
		if(!window)
			return;
		GUI_ListBox* selectedList = SDL_static_cast(GUI_ListBox*, window->getChild(CAVEBOT_LOOT_CATALOG_SELECTED_EVENTID));
		GUI_ListBox* availableList = SDL_static_cast(GUI_ListBox*, window->getChild(CAVEBOT_LOOT_CATALOG_AVAILABLE_EVENTID));
		GUI_TextBox* selectedSearch = SDL_static_cast(GUI_TextBox*, window->getChild(CAVEBOT_LOOT_CATALOG_SELECTED_SEARCH_EVENTID));
		GUI_TextBox* availableSearch = SDL_static_cast(GUI_TextBox*, window->getChild(CAVEBOT_LOOT_CATALOG_AVAILABLE_SEARCH_EVENTID));
		if(!selectedList || !availableList)
			return;

		const std::string selectedFilter = lowerText(selectedSearch ? selectedSearch->getActualText() : std::string());
		const std::string availableFilter = lowerText(availableSearch ? availableSearch->getActualText() : std::string());
		selectedList->clear();
		availableList->clear();
		g_lootCatalogVisibleSelectedIds.clear();
		g_lootCatalogAvailableIds.clear();
		for(Uint16 itemId : g_lootCatalogSelectedIds)
		{
			std::string name = lootCatalogItemText(itemId);
			if(selectedFilter.empty() || lowerText(name).find(selectedFilter) != std::string::npos)
			{
				g_lootCatalogVisibleSelectedIds.push_back(itemId);
				selectedList->addItem(name, itemId);
			}
		}
		for(Uint16 itemId : g_lootCatalogAllItemIds)
		{
			if(std::find(g_lootCatalogSelectedIds.begin(), g_lootCatalogSelectedIds.end(), itemId) != g_lootCatalogSelectedIds.end())
				continue;
			std::string name = lootCatalogItemText(itemId);
			if(availableFilter.empty() || lowerText(name).find(availableFilter) != std::string::npos)
			{
				g_lootCatalogAvailableIds.push_back(itemId);
				availableList->addItem(name, itemId);
			}
		}
		if(!g_lootCatalogVisibleSelectedIds.empty())
			selectedList->setSelect(0);
		if(!g_lootCatalogAvailableIds.empty())
			availableList->setSelect(0);
		g_lootCatalogSelectedSelection = (g_lootCatalogVisibleSelectedIds.empty() ? -1 : 0);
		g_lootCatalogAvailableSelection = (g_lootCatalogAvailableIds.empty() ? -1 : 0);
		setCatalogCount(window, CAVEBOT_LOOT_CATALOG_SELECTED_COUNT_EVENTID, g_lootCatalogSelectedIds.size(), " item", " itens");
		setCatalogCount(window, CAVEBOT_LOOT_CATALOG_AVAILABLE_COUNT_EVENTID, g_lootCatalogAvailableIds.size(), " item", " itens");
	}

	void updateLootHiddenField()
	{
		GUI_Window* caveWindow = g_engine.getWindow(GUI_WINDOW_CAVEBOTOPTIONS);
		GUI_TextBox* ids = (caveWindow ? SDL_static_cast(GUI_TextBox*, caveWindow->getChild(CAVEBOT_LOOT_IDS_EVENTID)) : NULL);
		if(ids)
			ids->setText(serializeLootItemIds(g_lootCatalogSelectedIds));
	}

	void openLootItemCatalog()
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
		GUI_Window* caveWindow = g_engine.getWindow(GUI_WINDOW_CAVEBOTOPTIONS);
		GUI_TextBox* ids = (caveWindow ? SDL_static_cast(GUI_TextBox*, caveWindow->getChild(CAVEBOT_LOOT_IDS_EVENTID)) : NULL);
		g_lootCatalogSelectedIds = parseLootItemIds(ids ? ids->getActualText() : g_caveBot.getSettings().lootItemIds);

		GUI_Window* window = new GUI_Window(iRect(0, 0, CAVEBOT_CATALOG_WIDTH, CAVEBOT_CATALOG_HEIGHT), "Autoloot - lista de itens", GUI_WINDOW_CAVEBOTLOOTITEMS);
		window->addChild(new GUI_Label(iRect(18, 27, 0, 0), "Itens no autoloot"));
		window->addChild(new GUI_Label(iRect(300, 27, 0, 0), "", CAVEBOT_LOOT_CATALOG_SELECTED_COUNT_EVENTID));
		window->addChild(new GUI_Label(iRect(392, 27, 0, 0), "Itens para adicionar"));
		window->addChild(new GUI_Label(iRect(675, 27, 0, 0), "", CAVEBOT_LOOT_CATALOG_AVAILABLE_COUNT_EVENTID));
		window->addChild(new GUI_Label(iRect(18, 51, 0, 0), "Buscar:"));
		GUI_TextBox* search = new GUI_TextBox(iRect(68, 47, 300, 16), std::string(), CAVEBOT_LOOT_CATALOG_SELECTED_SEARCH_EVENTID);
		search->setMaxLength(80);
		search->setTextEventCallback(&caveBotLootItems_Events, CAVEBOT_LOOT_CATALOG_SELECTED_SEARCH_EVENTID);
		search->startEvents();
		window->addChild(search);
		window->addChild(new GUI_Label(iRect(392, 51, 0, 0), "Buscar:"));
		search = new GUI_TextBox(iRect(442, 47, 300, 16), std::string(), CAVEBOT_LOOT_CATALOG_AVAILABLE_SEARCH_EVENTID);
		search->setMaxLength(80);
		search->setTextEventCallback(&caveBotLootItems_Events, CAVEBOT_LOOT_CATALOG_AVAILABLE_SEARCH_EVENTID);
		search->startEvents();
		window->addChild(search);

		GUI_ListBox* list = new GUI_ListBox(iRect(18, 73, 350, 345), CAVEBOT_LOOT_CATALOG_SELECTED_EVENTID);
		list->setEventCallback(&caveBotLootItems_Events, CAVEBOT_LOOT_CATALOG_SELECTED_EVENTID);
		list->startEvents();
		window->addChild(list);
		list = new GUI_ListBox(iRect(392, 73, 350, 345), CAVEBOT_LOOT_CATALOG_AVAILABLE_EVENTID);
		list->setEventCallback(&caveBotLootItems_Events, CAVEBOT_LOOT_CATALOG_AVAILABLE_EVENTID);
		list->startEvents();
		window->addChild(list);

		GUI_Button* button = new GUI_Button(iRect(282, 430, GUI_UI_BUTTON_86PX_GRAY_UP_W, GUI_UI_BUTTON_86PX_GRAY_UP_H), "Remover >");
		button->setButtonEventCallback(&caveBotLootItems_Events, CAVEBOT_LOOT_CATALOG_REMOVE_EVENTID);
		button->startEvents();
		window->addChild(button);
		button = new GUI_Button(iRect(392, 430, GUI_UI_BUTTON_86PX_GREEN_UP_W, GUI_UI_BUTTON_86PX_GREEN_UP_H), "< Adicionar");
		button->setButtonEventCallback(&caveBotLootItems_Events, CAVEBOT_LOOT_CATALOG_ADD_EVENTID);
		button->startEvents();
		window->addChild(button);

		window->addChild(new GUI_Separator(iRect(13, CAVEBOT_CATALOG_HEIGHT - 40, CAVEBOT_CATALOG_WIDTH - 26, 2)));
		button = new GUI_Button(iRect(CAVEBOT_CATALOG_WIDTH - 56, CAVEBOT_CATALOG_HEIGHT - 30, GUI_UI_BUTTON_43PX_GRAY_UP_W, GUI_UI_BUTTON_43PX_GRAY_UP_H), "Fechar", CLIENT_GUI_ESCAPE_TRIGGER);
		button->setButtonEventCallback(&caveBotLootItems_Events, CAVEBOT_LOOT_CATALOG_CLOSE_EVENTID);
		button->startEvents();
		window->addChild(button);

		g_engine.addWindow(window, true);
		refreshLootCatalogLists(window);
	}

	const CaveBotMonsterCatalogEntry* findMonsterEntry(const std::string& name)
	{
		const std::string key = lowerText(name);
		for(const CaveBotMonsterCatalogEntry& entry : g_monsterCatalog)
		{
			if(entry.key == key)
				return &entry;
		}
		return NULL;
	}

	void loadMonsterCatalog()
	{
		if(g_monsterCatalogLoaded)
			return;
		g_monsterCatalogLoaded = true;
		g_monsterCatalog.push_back({"Todos", "*", 0, 0});

		std::string path = g_basePath + ASSETS_CATALOG + PATH_PLATFORM_SLASH + "spawns.ofsp";
		SDL_RWops* file = SDL_RWFromFile(path.c_str(), "rb");
		if(!file)
			return;
		char magic[4];
		if(SDL_RWread(file, magic, 1, sizeof(magic)) != sizeof(magic) || SDL_memcmp(magic, "OFSP", sizeof(magic)) != 0)
		{
			SDL_RWclose(file);
			return;
		}
		Uint32 version = SDL_ReadLE32(file);
		Uint16 count = SDL_ReadLE16(file);
		if(version != 2)
		{
			SDL_RWclose(file);
			return;
		}
		for(Uint16 index = 0; index < count; ++index)
		{
			Uint8 nameLength = 0;
			if(SDL_RWread(file, &nameLength, 1, 1) != 1)
				break;
			std::string name(nameLength, '\0');
			if(nameLength != 0 && SDL_RWread(file, &name[0], 1, nameLength) != nameLength)
				break;
			Uint16 lookType = SDL_ReadLE16(file);
			Uint8 colors[4] = {};
			if(SDL_RWread(file, colors, 1, sizeof(colors)) != sizeof(colors))
				break;
			SDL_ReadLE32(file);
			SDL_ReadLE32(file);
			Uint16 lootCount = SDL_ReadLE16(file);
			for(Uint16 lootIndex = 0; lootIndex < lootCount; ++lootIndex)
			{
				SDL_ReadLE16(file);
				SDL_ReadLE16(file);
				SDL_ReadLE16(file);
				Uint8 lootNameLength = 0;
				if(SDL_RWread(file, &lootNameLength, 1, 1) != 1)
				{
					lootCount = lootIndex;
					break;
				}
				if(lootNameLength != 0)
					SDL_RWseek(file, lootNameLength, RW_SEEK_CUR);
			}
			if(name.empty())
				continue;
			CaveBotMonsterCatalogEntry entry;
			entry.name = titleText(name);
			entry.key = lowerText(name);
			entry.lookType = lookType;
			entry.outfitColor = (SDL_static_cast(Uint32, colors[3]) << 24) | (SDL_static_cast(Uint32, colors[2]) << 16) |
				(SDL_static_cast(Uint32, colors[1]) << 8) | SDL_static_cast(Uint32, colors[0]);
			g_monsterCatalog.push_back(entry);
		}
		SDL_RWclose(file);
		std::stable_sort(g_monsterCatalog.begin() + 1, g_monsterCatalog.end(), [](const CaveBotMonsterCatalogEntry& left, const CaveBotMonsterCatalogEntry& right) {
			return left.key < right.key;
		});
	}

	void addKnownMonstersToCatalog()
	{
		for(const auto& known : g_map.getKnownCreatures())
		{
			Creature* creature = known.second;
			if(!creature || !creature->isMonster() || creature->getName().empty() || findMonsterEntry(creature->getName()))
				continue;
			CaveBotMonsterCatalogEntry entry;
			entry.name = creature->getName();
			entry.key = lowerText(entry.name);
			entry.lookType = (creature->getThingType() ? creature->getThingType()->m_id : 0);
			entry.outfitColor = creature->getOutfitPacked();
			g_monsterCatalog.push_back(entry);
		}
	}

	void updateTargetHiddenField()
	{
		GUI_Window* caveWindow = g_engine.getWindow(GUI_WINDOW_CAVEBOTOPTIONS);
		GUI_TextBox* names = (caveWindow ? SDL_static_cast(GUI_TextBox*, caveWindow->getChild(CAVEBOT_TARGET_NAMES_EVENTID)) : NULL);
		if(names)
			names->setText(serializeNameList(g_monsterCatalogSelected));
	}

	void addMonsterListRow(GUI_ListBox* list, const CaveBotMonsterCatalogEntry& entry)
	{
		list->addCreature(entry.name, entry.lookType, entry.outfitColor);
	}

	void refreshMonsterCatalogLists(GUI_Window* window)
	{
		if(!window)
			return;
		GUI_ListBox* selectedList = SDL_static_cast(GUI_ListBox*, window->getChild(CAVEBOT_TARGET_CATALOG_SELECTED_EVENTID));
		GUI_ListBox* availableList = SDL_static_cast(GUI_ListBox*, window->getChild(CAVEBOT_TARGET_CATALOG_AVAILABLE_EVENTID));
		GUI_TextBox* selectedSearch = SDL_static_cast(GUI_TextBox*, window->getChild(CAVEBOT_TARGET_CATALOG_SELECTED_SEARCH_EVENTID));
		GUI_TextBox* availableSearch = SDL_static_cast(GUI_TextBox*, window->getChild(CAVEBOT_TARGET_CATALOG_AVAILABLE_SEARCH_EVENTID));
		if(!selectedList || !availableList)
			return;
		const std::string selectedFilter = lowerText(selectedSearch ? selectedSearch->getActualText() : std::string());
		const std::string availableFilter = lowerText(availableSearch ? availableSearch->getActualText() : std::string());
		selectedList->clear();
		availableList->clear();
		g_monsterCatalogVisibleSelected.clear();
		g_monsterCatalogAvailable.clear();
		for(size_t selectedIndex = 0; selectedIndex < g_monsterCatalogSelected.size(); ++selectedIndex)
		{
			const std::string& selected = g_monsterCatalogSelected[selectedIndex];
			const CaveBotMonsterCatalogEntry* entry = findMonsterEntry(selected);
			std::string displayName = (selected == "*" ? "Todos" : (entry ? entry->name : selected));
			if(selectedFilter.empty() || lowerText(displayName).find(selectedFilter) != std::string::npos)
			{
				g_monsterCatalogVisibleSelected.push_back(selectedIndex);
				if(entry)
					addMonsterListRow(selectedList, *entry);
				else
					selectedList->addCreature(displayName, 0, 0);
			}
		}
		for(size_t catalogIndex = 0; catalogIndex < g_monsterCatalog.size(); ++catalogIndex)
		{
			const CaveBotMonsterCatalogEntry& entry = g_monsterCatalog[catalogIndex];
			bool alreadySelected = std::any_of(g_monsterCatalogSelected.begin(), g_monsterCatalogSelected.end(), [&entry](const std::string& selected) {
				return lowerText(selected) == entry.key;
			});
			if(alreadySelected || (!availableFilter.empty() && lowerText(entry.name).find(availableFilter) == std::string::npos))
				continue;
			g_monsterCatalogAvailable.push_back(catalogIndex);
			addMonsterListRow(availableList, entry);
		}
		if(!g_monsterCatalogVisibleSelected.empty())
			selectedList->setSelect(0);
		if(!g_monsterCatalogAvailable.empty())
			availableList->setSelect(0);
		g_monsterCatalogSelectedSelection = (g_monsterCatalogVisibleSelected.empty() ? -1 : 0);
		g_monsterCatalogAvailableSelection = (g_monsterCatalogAvailable.empty() ? -1 : 0);
		setCatalogCount(window, CAVEBOT_TARGET_CATALOG_SELECTED_COUNT_EVENTID, g_monsterCatalogSelected.size(), " alvo", " alvos");
		setCatalogCount(window, CAVEBOT_TARGET_CATALOG_AVAILABLE_COUNT_EVENTID, g_monsterCatalogAvailable.size(), " opcao", " opcoes");
	}

	void openTargetCatalog()
	{
		GUI_Window* oldWindow = g_engine.getWindow(GUI_WINDOW_CAVEBOTTARGETS);
		if(oldWindow)
			g_engine.removeWindow(oldWindow);
		loadMonsterCatalog();
		addKnownMonstersToCatalog();
		GUI_Window* caveWindow = g_engine.getWindow(GUI_WINDOW_CAVEBOTOPTIONS);
		GUI_TextBox* names = (caveWindow ? SDL_static_cast(GUI_TextBox*, caveWindow->getChild(CAVEBOT_TARGET_NAMES_EVENTID)) : NULL);
		g_monsterCatalogSelected = parseNameList(names ? names->getActualText() : g_caveBot.getSettings().targetNames);

		GUI_Window* window = new GUI_Window(iRect(0, 0, CAVEBOT_CATALOG_WIDTH, CAVEBOT_CATALOG_HEIGHT), "TargetBot - lista de monstros", GUI_WINDOW_CAVEBOTTARGETS);
		window->addChild(new GUI_Label(iRect(18, 27, 0, 0), "Monstros atacados"));
		window->addChild(new GUI_Label(iRect(300, 27, 0, 0), "", CAVEBOT_TARGET_CATALOG_SELECTED_COUNT_EVENTID));
		window->addChild(new GUI_Label(iRect(392, 27, 0, 0), "Monstros para adicionar"));
		window->addChild(new GUI_Label(iRect(675, 27, 0, 0), "", CAVEBOT_TARGET_CATALOG_AVAILABLE_COUNT_EVENTID));
		window->addChild(new GUI_Label(iRect(18, 51, 0, 0), "Buscar:"));
		GUI_TextBox* search = new GUI_TextBox(iRect(68, 47, 300, 16), std::string(), CAVEBOT_TARGET_CATALOG_SELECTED_SEARCH_EVENTID);
		search->setMaxLength(80);
		search->setTextEventCallback(&caveBotTargets_Events, CAVEBOT_TARGET_CATALOG_SELECTED_SEARCH_EVENTID);
		search->startEvents();
		window->addChild(search);
		window->addChild(new GUI_Label(iRect(392, 51, 0, 0), "Buscar:"));
		search = new GUI_TextBox(iRect(442, 47, 300, 16), std::string(), CAVEBOT_TARGET_CATALOG_AVAILABLE_SEARCH_EVENTID);
		search->setMaxLength(80);
		search->setTextEventCallback(&caveBotTargets_Events, CAVEBOT_TARGET_CATALOG_AVAILABLE_SEARCH_EVENTID);
		search->startEvents();
		window->addChild(search);
		GUI_ListBox* list = new GUI_ListBox(iRect(18, 73, 350, 345), CAVEBOT_TARGET_CATALOG_SELECTED_EVENTID);
		list->setEventCallback(&caveBotTargets_Events, CAVEBOT_TARGET_CATALOG_SELECTED_EVENTID);
		list->startEvents();
		window->addChild(list);
		list = new GUI_ListBox(iRect(392, 73, 350, 345), CAVEBOT_TARGET_CATALOG_AVAILABLE_EVENTID);
		list->setEventCallback(&caveBotTargets_Events, CAVEBOT_TARGET_CATALOG_AVAILABLE_EVENTID);
		list->startEvents();
		window->addChild(list);
		GUI_Button* button = new GUI_Button(iRect(282, 430, GUI_UI_BUTTON_86PX_GRAY_UP_W, GUI_UI_BUTTON_86PX_GRAY_UP_H), "Remover >");
		button->setButtonEventCallback(&caveBotTargets_Events, CAVEBOT_TARGET_CATALOG_REMOVE_EVENTID);
		button->startEvents();
		window->addChild(button);
		button = new GUI_Button(iRect(392, 430, GUI_UI_BUTTON_86PX_GREEN_UP_W, GUI_UI_BUTTON_86PX_GREEN_UP_H), "< Adicionar");
		button->setButtonEventCallback(&caveBotTargets_Events, CAVEBOT_TARGET_CATALOG_ADD_EVENTID);
		button->startEvents();
		window->addChild(button);
		window->addChild(new GUI_Separator(iRect(13, CAVEBOT_CATALOG_HEIGHT - 40, CAVEBOT_CATALOG_WIDTH - 26, 2)));
		button = new GUI_Button(iRect(CAVEBOT_CATALOG_WIDTH - 56, CAVEBOT_CATALOG_HEIGHT - 30, GUI_UI_BUTTON_43PX_GRAY_UP_W, GUI_UI_BUTTON_43PX_GRAY_UP_H), "Fechar", CLIENT_GUI_ESCAPE_TRIGGER);
		button->setButtonEventCallback(&caveBotTargets_Events, CAVEBOT_TARGET_CATALOG_CLOSE_EVENTID);
		button->startEvents();
		window->addChild(button);
		g_engine.addWindow(window, true);
		refreshMonsterCatalogLists(window);
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

	void addHiddenTextBox(GUI_Window* window, const std::string& text, Uint32 id, Uint32 maxLength)
	{
		GUI_TextBox* textBox = new GUI_TextBox(iRect(-10, -10, 0, 0), text, id);
		textBox->setMaxLength(maxLength);
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
	if(event == CAVEBOT_LOOT_CATALOG_SELECTED_SEARCH_EVENTID || event == CAVEBOT_LOOT_CATALOG_AVAILABLE_SEARCH_EVENTID)
	{
		refreshLootCatalogLists(window);
		return;
	}
	if(event == CAVEBOT_LOOT_CATALOG_AVAILABLE_EVENTID)
	{
		if(status < 0)
		{
			status = ~status;
			g_lootCatalogAvailableSelection = status;
			caveBotLootItems_Events(CAVEBOT_LOOT_CATALOG_ADD_EVENTID, 0);
		}
		else
			g_lootCatalogAvailableSelection = status;
		return;
	}
	if(event == CAVEBOT_LOOT_CATALOG_SELECTED_EVENTID)
	{
		if(status < 0)
		{
			status = ~status;
			g_lootCatalogSelectedSelection = status;
			caveBotLootItems_Events(CAVEBOT_LOOT_CATALOG_REMOVE_EVENTID, 0);
		}
		else
			g_lootCatalogSelectedSelection = status;
		return;
	}
	if(event == CAVEBOT_LOOT_CATALOG_ADD_EVENTID && g_lootCatalogAvailableSelection >= 0 &&
		SDL_static_cast(size_t, g_lootCatalogAvailableSelection) < g_lootCatalogAvailableIds.size())
	{
		Uint16 itemId = g_lootCatalogAvailableIds[SDL_static_cast(size_t, g_lootCatalogAvailableSelection)];
		if(std::find(g_lootCatalogSelectedIds.begin(), g_lootCatalogSelectedIds.end(), itemId) == g_lootCatalogSelectedIds.end())
			g_lootCatalogSelectedIds.push_back(itemId);
		updateLootHiddenField();
		refreshLootCatalogLists(window);
		return;
	}
	if(event == CAVEBOT_LOOT_CATALOG_REMOVE_EVENTID && g_lootCatalogSelectedSelection >= 0 &&
		SDL_static_cast(size_t, g_lootCatalogSelectedSelection) < g_lootCatalogVisibleSelectedIds.size())
	{
		Uint16 itemId = g_lootCatalogVisibleSelectedIds[SDL_static_cast(size_t, g_lootCatalogSelectedSelection)];
		g_lootCatalogSelectedIds.erase(std::remove(g_lootCatalogSelectedIds.begin(), g_lootCatalogSelectedIds.end(), itemId), g_lootCatalogSelectedIds.end());
		updateLootHiddenField();
		refreshLootCatalogLists(window);
	}
}

void caveBotTargets_Events(Uint32 event, Sint32 status)
{
	GUI_Window* window = g_engine.getCurrentWindow();
	if(!window || window->getInternalID() != GUI_WINDOW_CAVEBOTTARGETS)
		return;
	if(event == CAVEBOT_TARGET_CATALOG_CLOSE_EVENTID)
	{
		g_engine.removeWindow(window);
		return;
	}
	if(event == CAVEBOT_TARGET_CATALOG_SELECTED_SEARCH_EVENTID || event == CAVEBOT_TARGET_CATALOG_AVAILABLE_SEARCH_EVENTID)
	{
		refreshMonsterCatalogLists(window);
		return;
	}
	if(event == CAVEBOT_TARGET_CATALOG_AVAILABLE_EVENTID)
	{
		if(status < 0)
		{
			status = ~status;
			g_monsterCatalogAvailableSelection = status;
			caveBotTargets_Events(CAVEBOT_TARGET_CATALOG_ADD_EVENTID, 0);
		}
		else
			g_monsterCatalogAvailableSelection = status;
		return;
	}
	if(event == CAVEBOT_TARGET_CATALOG_SELECTED_EVENTID)
	{
		if(status < 0)
		{
			status = ~status;
			g_monsterCatalogSelectedSelection = status;
			caveBotTargets_Events(CAVEBOT_TARGET_CATALOG_REMOVE_EVENTID, 0);
		}
		else
			g_monsterCatalogSelectedSelection = status;
		return;
	}
	if(event == CAVEBOT_TARGET_CATALOG_ADD_EVENTID && g_monsterCatalogAvailableSelection >= 0 &&
		SDL_static_cast(size_t, g_monsterCatalogAvailableSelection) < g_monsterCatalogAvailable.size())
	{
		const CaveBotMonsterCatalogEntry& entry = g_monsterCatalog[g_monsterCatalogAvailable[SDL_static_cast(size_t, g_monsterCatalogAvailableSelection)]];
		if(entry.key == "*")
		{
			g_monsterCatalogSelected.clear();
			g_monsterCatalogSelected.push_back("*");
		}
		else
		{
			g_monsterCatalogSelected.erase(std::remove(g_monsterCatalogSelected.begin(), g_monsterCatalogSelected.end(), "*"), g_monsterCatalogSelected.end());
			g_monsterCatalogSelected.push_back(entry.name);
		}
		updateTargetHiddenField();
		refreshMonsterCatalogLists(window);
		return;
	}
	if(event == CAVEBOT_TARGET_CATALOG_REMOVE_EVENTID && g_monsterCatalogSelectedSelection >= 0 &&
		SDL_static_cast(size_t, g_monsterCatalogSelectedSelection) < g_monsterCatalogVisibleSelected.size())
	{
		size_t selectedIndex = g_monsterCatalogVisibleSelected[SDL_static_cast(size_t, g_monsterCatalogSelectedSelection)];
		if(selectedIndex < g_monsterCatalogSelected.size())
			g_monsterCatalogSelected.erase(g_monsterCatalogSelected.begin() + selectedIndex);
		updateTargetHiddenField();
		refreshMonsterCatalogLists(window);
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
	if(event == CAVEBOT_TARGET_LIST_EVENTID)
	{
		openTargetCatalog();
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
	addLabel(window, 155, 387, "Monstros atacados:");
	addButton(window, 275, 381, "Lista alvos", CAVEBOT_TARGET_LIST_EVENTID);
	addLabel(window, 371, 387, settings.targetNames == "*" ? "Todos" : "Abrir para ver e editar");
	addHiddenTextBox(window, settings.targetNames, CAVEBOT_TARGET_NAMES_EVENTID, 500);
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
	addButton(window, 255, 540, "Lista itens", CAVEBOT_LOOT_ITEM_LIST_EVENTID);
	std::vector<Uint16> selectedLootItems = parseLootItemIds(settings.lootItemIds);
	std::string selectedLootText = std::to_string(selectedLootItems.size()) + (selectedLootItems.size() == 1 ? " item selecionado" : " itens selecionados");
	window->addChild(new GUI_Label(iRect(351, 547, 0, 0), selectedLootText));
	addHiddenTextBox(window, settings.lootItemIds, CAVEBOT_LOOT_IDS_EVENTID, 1000);
	addLabel(window, 18, 574, "Mochila destino aberta (0-15):");
	addTextBox(window, 189, 570, 35, numberText(settings.lootDestinationContainer), CAVEBOT_LOOT_DESTINATION_EVENTID, 2, true);
	addCheckBox(window, 245, 568, 150, "Loot after kill", settings.lootAfterKill, CAVEBOT_LOOT_AFTER_KILL_EVENTID);
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
