/*
  Old FasterOT custom store client.
  The server exchanges JSON through the OTClient extended opcode 0xC9.
*/

#include "GUI_UTIL.h"
#include "ShopWindow.h"
#include "../engine.h"
#include "../game.h"
#include "../json/json.h"
#include "../GUI_Elements/GUI_Window.h"
#include "../GUI_Elements/GUI_Button.h"
#include "../GUI_Elements/GUI_Label.h"
#include "../GUI_Elements/GUI_ListBox.h"
#include "../GUI_Elements/GUI_MultiTextBox.h"
#include "../GUI_Elements/GUI_Separator.h"

#define STORE_WIDTH 700
#define STORE_HEIGHT 470
#define STORE_CLOSE_EVENTID 4100
#define STORE_BUYPOINTS_EVENTID 4101
#define STORE_HISTORY_EVENTID 4102
#define STORE_CATEGORY_EVENTID 4103
#define STORE_OFFER_EVENTID 4104
#define STORE_BUY_EVENTID 4105
#define STORE_POINTS_LABEL_EVENTID 4106
#define STORE_DESCRIPTION_EVENTID 4107
#define STORE_ITEM_EVENTID 4108

extern Engine g_engine;
extern Game g_game;

struct OldFasterStoreOffer
{
	std::string title;
	std::string description;
	Uint16 itemId = 0;
	Uint32 count = 1;
	Uint32 cost = 0;
};

struct OldFasterStoreCategory
{
	std::string name;
	Uint16 itemId = 0;
	std::vector<OldFasterStoreOffer> offers;
};

static std::vector<OldFasterStoreCategory> g_storeCategories;
static size_t g_storeCategory = 0;
static size_t g_storeOffer = 0;
static Uint32 g_storePoints = 0;
static std::string g_storeBuyUrl = "https://oldfasterot.com/index.php/account/points";

static std::string storeString(JSON_VALUE* value, const std::string& fallback = std::string())
{
	return (value && value->IsString() ? value->AsString() : fallback);
}

static Uint32 storeNumber(JSON_VALUE* value, Uint32 fallback = 0)
{
	return (value && value->IsNumber() ? SDL_static_cast(Uint32, value->AsNumber()) : fallback);
}

static void storeReadStatus(JSON_VALUE* status)
{
	if(!status || !status->IsObject())
		return;

	g_storePoints = storeNumber(status->find("points"), g_storePoints);
	std::string buyUrl = storeString(status->find("buyUrl"));
	if(!buyUrl.empty())
		g_storeBuyUrl = std::move(buyUrl);
}

static void storeReadCategories(JSON_VALUE* data)
{
	if(!data || !data->IsArray())
		return;

	std::vector<OldFasterStoreCategory> categories;
	for(size_t categoryIndex = 0; categoryIndex < data->size(); ++categoryIndex)
	{
		JSON_VALUE* categoryJson = data->find(categoryIndex);
		if(!categoryJson || !categoryJson->IsObject())
			continue;

		OldFasterStoreCategory category;
		category.name = storeString(categoryJson->find("name"), "Category");
		category.itemId = SDL_static_cast(Uint16, storeNumber(categoryJson->find("item")));
		JSON_VALUE* offersJson = categoryJson->find("offers");
		if(offersJson && offersJson->IsArray())
		{
			for(size_t offerIndex = 0; offerIndex < offersJson->size(); ++offerIndex)
			{
				JSON_VALUE* offerJson = offersJson->find(offerIndex);
				if(!offerJson || !offerJson->IsObject())
					continue;

				OldFasterStoreOffer offer;
				offer.title = storeString(offerJson->find("title"), "Offer");
				offer.description = storeString(offerJson->find("description"));
				offer.itemId = SDL_static_cast(Uint16, storeNumber(offerJson->find("itemId"), storeNumber(offerJson->find("item"))));
				offer.count = storeNumber(offerJson->find("count"), 1);
				offer.cost = storeNumber(offerJson->find("cost"));
				category.offers.push_back(std::move(offer));
			}
		}
		categories.push_back(std::move(category));
	}

	if(!categories.empty())
	{
		g_storeCategories = std::move(categories);
		g_storeCategory = 0;
		g_storeOffer = 0;
	}
}

static void storeSendBuy()
{
	if(g_storeCategory >= g_storeCategories.size())
		return;
	OldFasterStoreCategory& category = g_storeCategories[g_storeCategory];
	if(g_storeOffer >= category.offers.size())
		return;

	OldFasterStoreOffer& offer = category.offers[g_storeOffer];
	std::string payload = "{\"action\":\"buy\",\"data\":{\"category\":" + std::to_string(g_storeCategory + 1) +
		",\"offer\":" + std::to_string(g_storeOffer + 1) + ",\"title\":" + JSON_VALUE::MakeString(offer.title) +
		",\"cost\":" + std::to_string(offer.cost) + "}}";
	g_game.sendOldFasterStoreMessage(payload);
}

static void storeEvents(Uint32 event, Sint32 status)
{
	switch(event)
	{
		case STORE_CLOSE_EVENTID:
		{
			GUI_Window* window = g_engine.getWindow(GUI_WINDOW_OLDFASTERSTORE);
			if(window)
				g_engine.removeWindow(window);
		}
		break;
		case STORE_BUYPOINTS_EVENTID:
			if(!g_storeBuyUrl.empty())
				UTIL_OpenURL(g_storeBuyUrl.c_str());
		break;
		case STORE_HISTORY_EVENTID:
			g_game.sendOldFasterStoreMessage("{\"action\":\"history\",\"data\":{}}");
		break;
		case STORE_CATEGORY_EVENTID:
		{
			if(status < 0)
				status = ~status;
			if(status >= 0 && SDL_static_cast(size_t, status) < g_storeCategories.size())
			{
				g_storeCategory = SDL_static_cast(size_t, status);
				g_storeOffer = 0;
				UTIL_createOldFasterStore();
			}
		}
		break;
		case STORE_OFFER_EVENTID:
		{
			if(status < 0)
				status = ~status;
			if(g_storeCategory < g_storeCategories.size() && status >= 0 && SDL_static_cast(size_t, status) < g_storeCategories[g_storeCategory].offers.size())
			{
				g_storeOffer = SDL_static_cast(size_t, status);
				UTIL_createOldFasterStore();
			}
		}
		break;
		case STORE_BUY_EVENTID: storeSendBuy(); break;
		default: break;
	}
}

void UTIL_createOldFasterStore()
{
	GUI_Window* oldWindow = g_engine.getWindow(GUI_WINDOW_OLDFASTERSTORE);
	if(oldWindow)
		g_engine.removeWindow(oldWindow);

	GUI_Window* window = new GUI_Window(iRect(0, 0, STORE_WIDTH, STORE_HEIGHT), "Old FasterOT Store", GUI_WINDOW_OLDFASTERSTORE);
	window->addChild(new GUI_Label(iRect(18, 34, 145, 14), "Points: " + std::to_string(g_storePoints), STORE_POINTS_LABEL_EVENTID, 220, 220, 220));

	GUI_Button* button = new GUI_Button(iRect(18, 54, GUI_UI_BUTTON_86PX_GRAY_UP_W, GUI_UI_BUTTON_86PX_GRAY_UP_H), "Buy points");
	button->setButtonEventCallback(&storeEvents, STORE_BUYPOINTS_EVENTID);
	button->startEvents();
	window->addChild(button);

	button = new GUI_Button(iRect(112, 54, GUI_UI_BUTTON_86PX_GRAY_UP_W, GUI_UI_BUTTON_86PX_GRAY_UP_H), "History");
	button->setButtonEventCallback(&storeEvents, STORE_HISTORY_EVENTID);
	button->startEvents();
	window->addChild(button);

	window->addChild(new GUI_Label(iRect(220, 34, 460, 14), "Old FasterOT - Premium Shop", 0, 242, 211, 75));

	GUI_ListBox* categories = new GUI_ListBox(iRect(18, 84, 180, 320), STORE_CATEGORY_EVENTID);
	for(const OldFasterStoreCategory& category : g_storeCategories)
		categories->add(category.name);
	if(!g_storeCategories.empty())
		categories->setSelect(SDL_static_cast(Sint32, g_storeCategory));
	categories->setEventCallback(&storeEvents, STORE_CATEGORY_EVENTID);
	categories->startEvents();
	window->addChild(categories);

	GUI_ListBox* offers = new GUI_ListBox(iRect(220, 84, 462, 190), STORE_OFFER_EVENTID);
	if(g_storeCategory < g_storeCategories.size())
	{
		for(const OldFasterStoreOffer& offer : g_storeCategories[g_storeCategory].offers)
			offers->add(offer.title + "  (" + std::to_string(offer.cost) + " points)");
		if(!g_storeCategories[g_storeCategory].offers.empty())
			offers->setSelect(SDL_static_cast(Sint32, g_storeOffer));
	}
	offers->setEventCallback(&storeEvents, STORE_OFFER_EVENTID);
	offers->startEvents();
	window->addChild(offers);

	if(g_storeCategory < g_storeCategories.size() && g_storeOffer < g_storeCategories[g_storeCategory].offers.size())
	{
		OldFasterStoreOffer& offer = g_storeCategories[g_storeCategory].offers[g_storeOffer];
		window->addChild(new GUI_ShopItem(iRect(220, 292, 32, 32), offer.itemId, SDL_static_cast(Uint16, offer.count), STORE_ITEM_EVENTID));
		window->addChild(new GUI_DynamicLabel(iRect(266, 288, 410, 14), offer.title, 0, 235, 235, 235));
		window->addChild(new GUI_Label(iRect(266, 307, 410, 14), "Price: " + std::to_string(offer.cost) + " points", 0, 220, 190, 80));
		GUI_MultiTextBox* description = new GUI_MultiTextBox(iRect(220, 334, 462, 70), false, offer.description, STORE_DESCRIPTION_EVENTID, 205, 205, 205);
		description->startEvents();
		window->addChild(description);
	}

	window->addChild(new GUI_Separator(iRect(13, STORE_HEIGHT - 40, STORE_WIDTH - 26, 2)));
	button = new GUI_Button(iRect(STORE_WIDTH - 203, STORE_HEIGHT - 30, GUI_UI_BUTTON_86PX_GOLD_UP_W, GUI_UI_BUTTON_86PX_GOLD_UP_H), "BUY");
	button->setButtonEventCallback(&storeEvents, STORE_BUY_EVENTID);
	button->startEvents();
	window->addChild(button);
	button = new GUI_Button(iRect(STORE_WIDTH - 99, STORE_HEIGHT - 30, GUI_UI_BUTTON_75PX_GRAY_UP_W, GUI_UI_BUTTON_75PX_GRAY_UP_H), "Close", CLIENT_GUI_ESCAPE_TRIGGER);
	button->setButtonEventCallback(&storeEvents, STORE_CLOSE_EVENTID);
	button->startEvents();
	window->addChild(button);
	g_engine.addWindow(window, true);
}

void UTIL_processOldFasterStoreMessage(const std::string& payload)
{
	std::unique_ptr<JSON_VALUE> root(JSON_VALUE::decode(payload.c_str()));
	if(!root || !root->IsObject())
		return;

	const std::string action = storeString(root->find("action"));
	if(action == "categories")
		storeReadCategories(root->find("data"));

	storeReadStatus(root->find("status"));
	JSON_VALUE* data = root->find("data");
	if(data && data->IsObject())
	{
		storeReadStatus(data->find("status"));
		g_storePoints = storeNumber(data->find("points"), g_storePoints);
	}

	std::string message = storeString(root->find("message"), storeString(root->find("msg")));
	if(message.empty() && data && data->IsObject())
		message = storeString(data->find("message"), storeString(data->find("msg")));

	if(action == "history" && data && data->IsArray())
	{
		std::string history;
		for(size_t index = 0; index < data->size(); ++index)
		{
			JSON_VALUE* entry = data->find(index);
			if(!entry || !entry->IsObject())
				continue;
			std::string title = storeString(entry->find("title"), storeString(entry->find("name"), "Purchase"));
			Uint32 cost = storeNumber(entry->find("cost"));
			history += title + " - " + std::to_string(cost) + " points\n";
		}
		UTIL_messageBox("Transaction history", history.empty() ? "You have not bought anything yet." : history);
		return;
	}

	if(!message.empty())
		UTIL_messageBox("Store", message);

	if(action == "categories" || g_engine.getWindow(GUI_WINDOW_OLDFASTERSTORE))
		UTIL_createOldFasterStore();
}
