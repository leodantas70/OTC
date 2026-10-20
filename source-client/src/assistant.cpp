/*
  Native assistant additions for The Forgotten Client.
  This module uses the client's public game state and protocol methods only.
*/

#include "assistant.h"
#include "container.h"
#include "config.h"
#include "const.h"
#include "engine.h"
#include "game.h"
#include "map.h"
#include "creature.h"
#include "position.h"
#include "GUI/GUI_UTIL.h"
#include "GUI/itemUI.h"

#include <cctype>

extern Engine g_engine;
extern Game g_game;
extern Map g_map;
extern std::string g_prefPath;

Assistant g_assistant;

namespace
{
	const Uint32 ASSISTANT_ACTION_DELAY = 650;
	const Uint32 ASSISTANT_HASTE_DELAY = 2000;
	// The emergency ring has its own short retry/confirmation cycle.  It must
	// react before the normal spell/item action cooldown without flooding the
	// server with several move packets in the same frame.
	const Uint32 ASSISTANT_EQUIPMENT_RETRY_DELAY = 80;
	const Uint32 ASSISTANT_EQUIPMENT_CONFIRM_DELAY = 120;
	const Uint32 ASSISTANT_EQUIPMENT_EMPTY_CONFIRM_DELAY = 40;

	bool isConfiguredRingId(Uint16 itemId, Uint16 configuredId)
	{
		if(itemId == 0 || configuredId == 0)
			return false;
		if(itemId == configuredId)
			return true;

		struct RingTransform
		{
			Uint16 baseId;
			Uint16 activeId;
		};

		// Every transforming ring from the supported 7.x item set. Both the
		// classic server ids and the modern client ids used by this data pack are
		// listed because the offset is not the same for every ring family.
		static const RingTransform transforms[] = {
			{2165, 2202}, // stealth ring
			{2166, 2203}, // power ring
			{2167, 2204}, // energy ring
			{2168, 2205}, // life ring
			{2169, 2206}, // time ring
			{2207, 2210}, // sword ring
			{2208, 2211}, // axe ring
			{2209, 2212}, // club ring
			{2213, 2215}, // dwarven ring
			{2214, 2216}, // ring of healing
			{3049, 3086}, // stealth ring (modern client id)
			{3050, 3087}, // power ring
			{3051, 3088}, // energy ring
			{3052, 3089}, // life ring
			{3053, 3090}, // time ring
			{3091, 3094}, // sword ring
			{3092, 3095}, // axe ring
			{3093, 3096}, // club ring
			{3097, 3099}, // dwarven ring
			{3098, 3100}  // ring of healing
		};

		for(const RingTransform& transform : transforms)
		{
			if(transform.baseId == configuredId && transform.activeId == itemId)
				return true;
		}
		return false;
	}

	bool readBool(Config& config, const char* key, bool fallback)
	{
		std::string value = config.fetchKey(key);
		if(value.empty())
			return fallback;
		return (SDL_strcasecmp(value.c_str(), "yes") == 0 || SDL_strcasecmp(value.c_str(), "true") == 0 || SDL_atoi(value.c_str()) != 0);
	}

	Uint32 readNumber(Config& config, const char* key, Uint32 fallback, Uint32 maximum)
	{
		std::string value = config.fetchKey(key);
		if(value.empty())
			return fallback;
		Uint32 result = SDL_static_cast(Uint32, SDL_strtoul(value.c_str(), NULL, 10));
		return (result > maximum ? maximum : result);
	}

	std::string readText(Config& config, const char* key, const std::string& fallback)
	{
		std::string value = config.fetchKey(key);
		return (value.empty() ? fallback : value);
	}

	std::string numberText(Uint32 value)
	{
		char buffer[32];
		SDL_snprintf(buffer, sizeof(buffer), "%u", value);
		return std::string(buffer);
	}

	const char* boolText(bool value)
	{
		return (value ? "yes" : "no");
	}

	std::string profileKey(const std::string& characterName)
	{
		std::string key;
		bool separator = false;
		for(unsigned char value : characterName)
		{
			if(std::isalnum(value))
			{
				key.push_back(SDL_static_cast(char, std::tolower(value)));
				separator = false;
			}
			else if(!key.empty() && !separator)
			{
				key.push_back('-');
				separator = true;
			}
		}
		while(!key.empty() && key.back() == '-')
			key.pop_back();
		return key;
	}

	std::string assistantConfigPath(const std::string& characterName)
	{
		std::string key = profileKey(characterName);
		return (key.empty() ? g_prefPath + "assistant.cfg" : g_prefPath + "assistant-" + key + ".cfg");
	}

	bool nameInCommaList(const std::string& name, const std::string& names)
	{
		size_t start = 0;
		while(start <= names.size())
		{
			size_t separator = names.find(',', start);
			size_t length = (separator == std::string::npos ? names.size() - start : separator - start);
			size_t first = names.find_first_not_of(" \t\r\n", start);
			if(first != std::string::npos && first < start + length)
			{
				size_t last = names.find_last_not_of(" \t\r\n", start + length - 1);
				if(last != std::string::npos)
				{
					std::string entry = names.substr(first, last - first + 1);
					if(SDL_strcasecmp(entry.c_str(), name.c_str()) == 0)
						return true;
				}
			}
			if(separator == std::string::npos)
				break;
			start = separator + 1;
		}
		return false;
	}
}

void Assistant::loadConfig()
{
	loadCharacterConfig(std::string());
}

void Assistant::loadCharacterConfig(const std::string& characterName)
{
	AssistantSettings settings;
	Config profileConfig;
	Config legacyConfig;
	Config* config = &profileConfig;
	bool loadedProfile = profileConfig.openToRead(assistantConfigPath(characterName).c_str());
	bool loaded = loadedProfile;
	if(!loaded && !characterName.empty())
	{
		loaded = legacyConfig.openToRead((g_prefPath + "assistant.cfg").c_str());
		config = &legacyConfig;
	}
	if(loaded)
	{
		settings.enabled = readBool(*config, "Enabled", settings.enabled);
		settings.fullLight = readBool(*config, "FullLight", settings.fullLight);
		settings.revealInvisibleMonsters = readBool(*config, "RevealInvisibleMonsters", settings.revealInvisibleMonsters);
		settings.healingEnabled = readBool(*config, "HealingEnabled", settings.healingEnabled);
		settings.healingUseRune = readBool(*config, "HealingUseRune", settings.healingUseRune);
		settings.healingPercent = SDL_static_cast(Uint8, readNumber(*config, "HealingPercent", settings.healingPercent, 100));
		settings.healingSpell = readText(*config, "HealingSpell", settings.healingSpell);
		settings.healingRuneId = SDL_static_cast(Uint16, readNumber(*config, "HealingRuneId", settings.healingRuneId, 65535));
		settings.healingRuneSubtype = SDL_static_cast(Uint8, readNumber(*config, "HealingRuneSubtype", settings.healingRuneSubtype, 255));
		settings.hasteEnabled = readBool(*config, "HasteEnabled", settings.hasteEnabled);
		settings.hasteSpell = readText(*config, "HasteSpell", settings.hasteSpell);
		settings.manaEnabled = readBool(*config, "ManaEnabled", settings.manaEnabled);
		settings.manaPercent = SDL_static_cast(Uint8, readNumber(*config, "ManaPercent", settings.manaPercent, 100));
		settings.manaFluidId = SDL_static_cast(Uint16, readNumber(*config, "ManaFluidId", settings.manaFluidId, 65535));
		settings.manaFluidSubtype = SDL_static_cast(Uint8, readNumber(*config, "ManaFluidSubtype", settings.manaFluidSubtype, 255));
		settings.energyRingEnabled = readBool(*config, "EnergyRingEnabled", settings.energyRingEnabled);
		settings.energyRingEquipPercent = SDL_static_cast(Uint8, readNumber(*config, "EnergyRingEquipPercent", settings.energyRingEquipPercent, 100));
		settings.energyRingUnequipPercent = SDL_static_cast(Uint8, readNumber(*config, "EnergyRingUnequipPercent", settings.energyRingUnequipPercent, 100));
		settings.energyRingId = SDL_static_cast(Uint16, readNumber(*config, "EnergyRingId", settings.energyRingId, 65535));
		settings.energyRingSubtype = SDL_static_cast(Uint8, readNumber(*config, "EnergyRingSubtype", settings.energyRingSubtype, 255));
		settings.preferredRingId = SDL_static_cast(Uint16, readNumber(*config, "PreferredRingId", settings.preferredRingId, 65535));
		settings.preferredRingSubtype = SDL_static_cast(Uint8, readNumber(*config, "PreferredRingSubtype", settings.preferredRingSubtype, 255));
		settings.amuletEnabled = readBool(*config, "AmuletEnabled", settings.amuletEnabled);
		settings.amuletSwitchPercent = SDL_static_cast(Uint8, readNumber(*config, "AmuletSwitchPercent", settings.amuletSwitchPercent, 100));
		settings.lowHealthAmuletId = SDL_static_cast(Uint16, readNumber(*config, "LowHealthAmuletId", settings.lowHealthAmuletId, 65535));
		settings.lowHealthAmuletSubtype = SDL_static_cast(Uint8, readNumber(*config, "LowHealthAmuletSubtype", settings.lowHealthAmuletSubtype, 255));
		settings.highHealthAmuletId = SDL_static_cast(Uint16, readNumber(*config, "HighHealthAmuletId", settings.highHealthAmuletId, 65535));
		settings.highHealthAmuletSubtype = SDL_static_cast(Uint8, readNumber(*config, "HighHealthAmuletSubtype", settings.highHealthAmuletSubtype, 255));
		settings.ammunitionEnabled = readBool(*config, "AmmunitionEnabled", settings.ammunitionEnabled);
		settings.ammunitionId = SDL_static_cast(Uint16, readNumber(*config, "AmmunitionId", settings.ammunitionId, 65535));
		settings.autoRuneEnabled = readBool(*config, "AutoRuneEnabled", settings.autoRuneEnabled);
		settings.runeManaTraining = readBool(*config, "RuneManaTraining", settings.runeManaTraining);
		settings.runeSpell = readText(*config, "RuneSpell", settings.runeSpell);
		settings.runeMinimumMana = readNumber(*config, "RuneMinimumMana", settings.runeMinimumMana, 1000000);
		settings.runeAntiIdle = readBool(*config, "RuneAntiIdle", settings.runeAntiIdle);
		settings.runeAutoFood = readBool(*config, "RuneAutoFood", settings.runeAutoFood);
		settings.runeFoodIds = readText(*config, "RuneFoodIds", settings.runeFoodIds);
		settings.runeLogoutNoBlank = readBool(*config, "RuneLogoutNoBlank", settings.runeLogoutNoBlank);
		settings.blankRuneId = SDL_static_cast(Uint16, readNumber(*config, "BlankRuneId", settings.blankRuneId, 65535));
		settings.runeLogoutNonVip = readBool(*config, "RuneLogoutNonVip", settings.runeLogoutNonVip);
		settings.runeTrustedPlayers = readText(*config, "RuneTrustedPlayers", settings.runeTrustedPlayers);
	}

	m_profileCharacter = characterName;
	m_configLoaded = true;
	applySettings(settings);
	if(!characterName.empty() && !loadedProfile)
		saveConfig();
}

void Assistant::saveConfig()
{
	Config config;
	if(!config.openToSave(assistantConfigPath(m_profileCharacter).c_str()))
		return;

	config.insertKey("Enabled", boolText(m_settings.enabled));
	config.insertKey("FullLight", boolText(m_settings.fullLight));
	config.insertKey("RevealInvisibleMonsters", boolText(m_settings.revealInvisibleMonsters));
	config.insertKey("HealingEnabled", boolText(m_settings.healingEnabled));
	config.insertKey("HealingUseRune", boolText(m_settings.healingUseRune));
	config.insertKey("HealingPercent", numberText(m_settings.healingPercent));
	config.insertKey("HealingSpell", m_settings.healingSpell);
	config.insertKey("HealingRuneId", numberText(m_settings.healingRuneId));
	config.insertKey("HealingRuneSubtype", numberText(m_settings.healingRuneSubtype));
	config.insertKey("HasteEnabled", boolText(m_settings.hasteEnabled));
	config.insertKey("HasteSpell", m_settings.hasteSpell);
	config.insertKey("ManaEnabled", boolText(m_settings.manaEnabled));
	config.insertKey("ManaPercent", numberText(m_settings.manaPercent));
	config.insertKey("ManaFluidId", numberText(m_settings.manaFluidId));
	config.insertKey("ManaFluidSubtype", numberText(m_settings.manaFluidSubtype));
	config.insertKey("EnergyRingEnabled", boolText(m_settings.energyRingEnabled));
	config.insertKey("EnergyRingEquipPercent", numberText(m_settings.energyRingEquipPercent));
	config.insertKey("EnergyRingUnequipPercent", numberText(m_settings.energyRingUnequipPercent));
	config.insertKey("EnergyRingId", numberText(m_settings.energyRingId));
	config.insertKey("EnergyRingSubtype", numberText(m_settings.energyRingSubtype));
	config.insertKey("PreferredRingId", numberText(m_settings.preferredRingId));
	config.insertKey("PreferredRingSubtype", numberText(m_settings.preferredRingSubtype));
	config.insertKey("AmuletEnabled", boolText(m_settings.amuletEnabled));
	config.insertKey("AmuletSwitchPercent", numberText(m_settings.amuletSwitchPercent));
	config.insertKey("LowHealthAmuletId", numberText(m_settings.lowHealthAmuletId));
	config.insertKey("LowHealthAmuletSubtype", numberText(m_settings.lowHealthAmuletSubtype));
	config.insertKey("HighHealthAmuletId", numberText(m_settings.highHealthAmuletId));
	config.insertKey("HighHealthAmuletSubtype", numberText(m_settings.highHealthAmuletSubtype));
	config.insertKey("AmmunitionEnabled", boolText(m_settings.ammunitionEnabled));
	config.insertKey("AmmunitionId", numberText(m_settings.ammunitionId));
	config.insertKey("AutoRuneEnabled", boolText(m_settings.autoRuneEnabled));
	config.insertKey("RuneManaTraining", boolText(m_settings.runeManaTraining));
	config.insertKey("RuneSpell", m_settings.runeSpell);
	config.insertKey("RuneMinimumMana", numberText(m_settings.runeMinimumMana));
	config.insertKey("RuneAntiIdle", boolText(m_settings.runeAntiIdle));
	config.insertKey("RuneAutoFood", boolText(m_settings.runeAutoFood));
	config.insertKey("RuneFoodIds", m_settings.runeFoodIds);
	config.insertKey("RuneLogoutNoBlank", boolText(m_settings.runeLogoutNoBlank));
	config.insertKey("BlankRuneId", numberText(m_settings.blankRuneId));
	config.insertKey("RuneLogoutNonVip", boolText(m_settings.runeLogoutNonVip));
	config.insertKey("RuneTrustedPlayers", m_settings.runeTrustedPlayers);
}

void Assistant::applySettings(const AssistantSettings& settings)
{
	bool oldFullLight = m_settings.fullLight;
	bool revealInvisibleChanged = (m_settings.revealInvisibleMonsters != settings.revealInvisibleMonsters);
	m_settings = settings;
	Creature::setRevealInvisibleMonsters(m_settings.revealInvisibleMonsters);
	if(revealInvisibleChanged)
	{
		knownCreatures& creatures = g_map.getKnownCreatures();
		for(knownCreatures::iterator it = creatures.begin(), end = creatures.end(); it != end; ++it)
		{
			if(it->second)
				it->second->refreshInvisibleOutfit();
		}
	}
	if(m_settings.healingPercent > 100)
		m_settings.healingPercent = 100;
	if(m_settings.manaPercent > 100)
		m_settings.manaPercent = 100;
	if(m_settings.energyRingEquipPercent > 100)
		m_settings.energyRingEquipPercent = 100;
	if(m_settings.energyRingUnequipPercent > 100)
		m_settings.energyRingUnequipPercent = 100;
	if(m_settings.amuletSwitchPercent > 100)
		m_settings.amuletSwitchPercent = 100;

	if(oldFullLight != m_settings.fullLight || m_settings.fullLight)
		applyFullLight(m_settings.fullLight);
	resetRuntime();
}

void Assistant::applyFullLight(bool enabled)
{
	if(enabled)
	{
		Uint8 currentMode = g_engine.getLightMode();
		if(currentMode != CLIENT_LIGHT_MODE_NONE)
			m_previousLightMode = currentMode;
		g_engine.setLightMode(CLIENT_LIGHT_MODE_NONE);
	}
	else if(g_engine.getLightMode() == CLIENT_LIGHT_MODE_NONE)
	{
		if(m_previousLightMode == CLIENT_LIGHT_MODE_NONE)
			m_previousLightMode = CLIENT_LIGHT_MODE_OLD;
		g_engine.setLightMode(m_previousLightMode);
	}
}

void Assistant::castSpell(const std::string& spell)
{
	if(!spell.empty())
		g_game.sendSay(MessageSay, 0, std::string(), spell);
}

bool Assistant::findItem(Uint16 itemId, Uint8 subtype, Position& position, Uint8& stackpos)
{
	for(Uint8 slot = SLOT_HEAD; slot < SLOT_LAST; ++slot)
	{
		ItemUI* item = g_game.getInventoryItem(slot);
		if(item && item->getID() == itemId && item->getItemSubtype() == subtype)
		{
			position = item->getCurrentPosition();
			stackpos = 0;
			return true;
		}
	}

	for(Uint8 containerId = 0; containerId < GAME_MAX_CONTAINERS; ++containerId)
	{
		Container* container = g_game.findContainer(containerId);
		if(!container)
			continue;

		std::vector<ItemUI*>& items = container->getItems();
		for(std::vector<ItemUI*>::iterator it = items.begin(), end = items.end(); it != end; ++it)
		{
			ItemUI* item = *it;
			if(item && item->getID() == itemId && item->getItemSubtype() == subtype)
			{
				position = item->getCurrentPosition();
				stackpos = 0;
				return true;
			}
		}
	}

	return false;
}

bool Assistant::useOnSelf(Uint16 itemId, Uint8 subtype)
{
	Position position;
	Uint8 stackpos = subtype;
	if(!findItem(itemId, subtype, position, stackpos))
	{
		g_game.processTextMessage(MessageFailure, "Item nao encontrado. Abra a mochila que contem o item.");
		return false;
	}

	g_game.sendUseOnCreature(position, itemId, stackpos, g_game.getPlayerID());
	return true;
}

bool Assistant::isEquipped(Uint8 slot, Uint16 itemId)
{
	ItemUI* item = g_game.getInventoryItem(slot);
	return (item && itemId != 0 && item->getID() == itemId);
}

bool Assistant::equipItem(Uint8 slot, Uint16 itemId, Uint8 subtype)
{
	if(itemId == 0 || isEquipped(slot, itemId))
		return false;

	Position sourcePosition;
	Uint8 sourceStackpos = subtype;
	if(!findItem(itemId, subtype, sourcePosition, sourceStackpos))
	{
		g_game.processTextMessage(MessageFailure, "Item de equipamento nao encontrado. Abra a mochila que contem o item.");
		return false;
	}

	Position destination(0xFFFF, SDL_static_cast(Uint16, slot) + 1, 0);
	if(sourcePosition == destination)
		return false;

	g_game.sendMove(sourcePosition, itemId, sourceStackpos, destination, 1);
	return true;
}

bool Assistant::equipAmmunition(Uint16 itemId)
{
	if(itemId == 0 || g_game.getInventoryItem(SLOT_AMMO))
		return false;

	// Munições são empilháveis e o subtipo muda conforme a quantidade. Procure
	// apenas pelo ID e mova a pilha inteira para o slot de arrows/ammo.
	for(Uint8 containerId = 0; containerId < GAME_MAX_CONTAINERS; ++containerId)
	{
		Container* container = g_game.findContainer(containerId);
		if(!container)
			continue;

		for(ItemUI* item : container->getItems())
		{
			if(!item || item->getID() != itemId)
				continue;

			Position destination(0xFFFF, SDL_static_cast(Uint16, SLOT_AMMO) + 1, 0);
			g_game.sendMove(item->getCurrentPosition(), itemId, 0, destination, item->getItemCount());
			return true;
		}
	}
	return false;
}

bool Assistant::hasItem(Uint16 itemId) const
{
	if(itemId == 0)
		return false;

	for(Uint8 slot = SLOT_HEAD; slot < SLOT_LAST; ++slot)
	{
		ItemUI* item = g_game.getInventoryItem(slot);
		if(item && item->getID() == itemId)
			return true;
	}

	for(Uint8 containerId = 0; containerId < GAME_MAX_CONTAINERS; ++containerId)
	{
		Container* container = g_game.findContainer(containerId);
		if(!container)
			continue;
		for(ItemUI* item : container->getItems())
		{
			if(item && item->getID() == itemId)
				return true;
		}
	}
	return false;
}

bool Assistant::useFood()
{
	const char* cursor = m_settings.runeFoodIds.c_str();
	while(*cursor)
	{
		char* end = NULL;
		Uint32 parsedId = SDL_strtoul(cursor, &end, 10);
		if(end == cursor)
		{
			++cursor;
			continue;
		}
		cursor = end;
		if(parsedId == 0 || parsedId > 65535)
			continue;

		Uint16 foodId = SDL_static_cast(Uint16, parsedId);
		for(Uint8 containerId = 0; containerId < GAME_MAX_CONTAINERS; ++containerId)
		{
			Container* container = g_game.findContainer(containerId);
			if(!container)
				continue;
			for(ItemUI* item : container->getItems())
			{
				if(!item || item->getID() != foodId)
					continue;
				g_game.sendUseItem(item->getCurrentPosition(), foodId, 0, g_game.findEmptyContainerId());
				return true;
			}
		}
	}
	return false;
}

bool Assistant::hasPlayerWithoutFriendHeartOnScreen() const
{
	Creature* local = g_map.getLocalCreature();
	if(!local)
		return false;
	Position localPosition = local->getCurrentPosition();
	for(auto& known : g_map.getKnownCreatures())
	{
		Creature* creature = known.second;
		if(!creature || creature == local || !creature->isPlayer() || !creature->isVisible())
			continue;
		if(creature->getCurrentPosition().z != localPosition.z)
			continue;
		if(!UTIL_isVipPlayerFriend(creature->getName()) &&
			!nameInCommaList(creature->getName(), m_settings.runeTrustedPlayers))
			return true;
	}
	return false;
}

bool Assistant::checkAutoRuneSafety(Uint32 now)
{
	if(!m_settings.autoRuneEnabled || m_autoRuneLogoutIssued)
		return m_autoRuneLogoutIssued;
	if(m_autoRuneSessionStart == 0)
	{
		m_autoRuneSessionStart = now;
		return false;
	}

	// Aguarda a lista VIP chegar do servidor antes de classificar jogadores.
	if(!SDL_TICKS_PASSED(now, m_autoRuneSessionStart + 8000))
		return false;

	const char* reason = NULL;
	if(m_settings.runeLogoutNonVip && hasPlayerWithoutFriendHeartOnScreen())
		reason = "Auto Rune: jogador sem coracao na VIP apareceu. Saindo do jogo.";
	else if(m_settings.runeLogoutNoBlank && m_settings.blankRuneId != 0 && !hasItem(m_settings.blankRuneId))
		reason = "Auto Rune: acabaram as blank runes abertas. Saindo do jogo.";

	if(!reason)
		return false;
	m_autoRuneLogoutIssued = true;
	g_game.processTextMessage(MessageWarning, reason);
	g_game.sendLogout();
	return true;
}

bool Assistant::tickAutoRuneActions(Uint32 now)
{
	if(!m_settings.autoRuneEnabled || m_autoRuneLogoutIssued || m_autoRuneSessionStart == 0 ||
		!SDL_TICKS_PASSED(now, m_autoRuneSessionStart + 8000))
		return false;

	if(m_settings.runeAntiIdle && SDL_TICKS_PASSED(now, m_nextRuneIdle))
	{
		Creature* local = g_map.getLocalCreature();
		if(local)
		{
			Direction direction = local->getDirection();
			Direction nextDirection = (direction == DIRECTION_NORTH ? DIRECTION_EAST : DIRECTION_NORTH);
			g_game.sendTurn(nextDirection);
			m_nextRuneIdle = now + 180000;
			return true;
		}
	}

	if(m_settings.runeAutoFood && SDL_TICKS_PASSED(now, m_nextRuneFood))
	{
		if(useFood())
		{
			m_nextRuneFood = now + 60000;
			return true;
		}
		m_nextRuneFood = now + 5000;
	}

	if(m_settings.runeManaTraining && !m_settings.runeSpell.empty() &&
		g_game.getPlayerMana() >= m_settings.runeMinimumMana && SDL_TICKS_PASSED(now, m_nextRuneCast))
	{
		castSpell(m_settings.runeSpell);
		m_nextRuneCast = now + 1000;
		return true;
	}
	return false;
}

bool Assistant::findFreeContainerPosition(Position& position)
{
	for(Uint8 containerId = 0; containerId < GAME_MAX_CONTAINERS; ++containerId)
	{
		Container* container = g_game.findContainer(containerId);
		if(!container || container->getItems().size() >= SDL_static_cast(size_t, container->getCapacity()))
			continue;

		position = Position(0xFFFF, SDL_static_cast(Uint16, containerId) | 0x40,
			SDL_static_cast(Uint8, container->getItems().size()));
		return true;
	}
	return false;
}

bool Assistant::unequipItem(Uint8 slot, Uint16 itemId)
{
	ItemUI* item = g_game.getInventoryItem(slot);
	if(!item || (itemId != 0 && item->getID() != itemId))
		return false;

	Position destination;
	if(!findFreeContainerPosition(destination))
	{
		g_game.processTextMessage(MessageFailure, "Nao ha espaco livre nas mochilas abertas para retirar o equipamento.");
		return false;
	}

	g_game.sendMove(item->getCurrentPosition(), item->getID(), 0, destination, 1);
	return true;
}

bool Assistant::shouldBlockTargetRune() const
{
	if(!m_configLoaded || !m_settings.enabled || !g_engine.isIngame() || g_game.getPlayerID() == 0)
		return false;

	const Uint8 healthPercent = g_game.getPlayerHealthPercent();
	if(healthPercent == 0)
		return false;

	if(m_settings.energyRingEnabled && m_settings.energyRingId != 0 && healthPercent <= m_settings.energyRingEquipPercent)
	{
		ItemUI* equippedRing = g_game.getInventoryItem(SLOT_RING);
		Uint16 equippedRingId = (equippedRing ? equippedRing->getID() : 0);
		bool emergencyRingEquipped = isConfiguredRingId(equippedRingId, m_settings.energyRingId) ||
			(m_energyRingActive && equippedRing && m_energyRingEquippedId != 0 && equippedRingId == m_energyRingEquippedId);
		if(!emergencyRingEquipped)
			return true;
	}

	bool healingConfigured = (m_settings.healingUseRune ? m_settings.healingRuneId != 0 : !m_settings.healingSpell.empty());
	return (m_settings.healingEnabled && healingConfigured && healthPercent <= m_settings.healingPercent);
}

void Assistant::tick()
{
	ensureCharacterProfile();
	if(!m_configLoaded)
		return;

	if(m_settings.fullLight && g_engine.getLightMode() != CLIENT_LIGHT_MODE_NONE)
		applyFullLight(true);

	if(!g_engine.isIngame() || g_game.getPlayerID() == 0)
	{
		m_autoRuneSessionStart = 0;
		m_autoRuneLogoutIssued = false;
		return;
	}

	Uint32 now = SDL_GetTicks();
	if(checkAutoRuneSafety(now))
		return;
	if(!m_settings.enabled)
	{
		if(tickAutoRuneActions(now))
			m_nextAction = now + ASSISTANT_ACTION_DELAY;
		return;
	}
	const Uint8 healthPercent = g_game.getPlayerHealthPercent();
	if(healthPercent == 0)
		return;

	// Energy Ring is an emergency action. It does not use the shared action
	// cooldown, and a move is only considered complete after the inventory slot
	// actually changes. This keeps retrying both equip and unequip when a packet
	// is delayed or rejected by the server.
	if(m_settings.energyRingEnabled && m_settings.energyRingId != 0)
	{
		ItemUI* equippedRing = g_game.getInventoryItem(SLOT_RING);
		Uint16 equippedRingId = (equippedRing ? equippedRing->getID() : 0);

		// The untransformed id is enough to recognize a ring that was already in
		// the slot. After an equip request, remember the id sent back by the
		// server because an active Energy Ring commonly has a different id.
		if(isConfiguredRingId(equippedRingId, m_settings.energyRingId))
		{
			m_energyRingActive = true;
			m_energyRingEquippedId = equippedRingId;
			m_energyRingEquipPending = false;
		}

		if(m_energyRingEquipPending)
		{
			bool slotChanged = (equippedRing &&
				(m_energyRingPreviousId == 0 || equippedRingId != m_energyRingPreviousId));
			if(slotChanged)
			{
				m_energyRingActive = true;
				m_energyRingEquippedId = equippedRingId;
				m_energyRingEquipPending = false;
			}
			else if(SDL_TICKS_PASSED(now, m_energyRingRequestTime + ASSISTANT_EQUIPMENT_CONFIRM_DELAY))
				m_energyRingEquipPending = false;
		}

		if(m_energyRingUnequipPending)
		{
			bool restoreConfirmed = (m_energyRingRestoreTargetId != 0 && equippedRing && equippedRingId == m_energyRingRestoreTargetId);
			bool removalConfirmed = (m_energyRingRestoreTargetId == 0 && !equippedRing);
			if(restoreConfirmed || removalConfirmed)
			{
				// Server-confirmed removal/replacement. Do not clear the latch when
				// merely sending the move, otherwise a refused packet leaves the
				// emergency ring stuck forever.
				m_energyRingActive = false;
				m_energyRingEquippedId = 0;
				m_energyRingRestoreTargetId = 0;
				m_energyRingUnequipPending = false;
				m_energyRingSlotMissingSince = 0;
			}
			else if(SDL_TICKS_PASSED(now, m_energyRingRequestTime + ASSISTANT_EQUIPMENT_CONFIRM_DELAY))
				m_energyRingUnequipPending = false;
		}

		// A replacement request may arrive while HP is falling again, or the
		// player may manually change the ring. Re-arm the emergency logic as soon
		// as the slot no longer contains the ring that this assistant equipped.
		if(m_energyRingActive && equippedRing && m_energyRingEquippedId != 0 &&
			equippedRingId != m_energyRingEquippedId && !isConfiguredRingId(equippedRingId, m_settings.energyRingId))
		{
			m_energyRingActive = false;
			m_energyRingEquippedId = 0;
			m_energyRingRestoreTargetId = 0;
			m_energyRingUnequipPending = false;
		}

		if(healthPercent <= m_settings.energyRingEquipPercent)
		{
			m_energyRingUnequipPending = false;
			m_energyRingRestoreTargetId = 0;

			// If the server removed/expired the emergency ring, confirm the empty
			// slot very briefly and immediately look for another one.
			if(m_energyRingActive && !equippedRing)
			{
				if(m_energyRingSlotMissingSince == 0)
					m_energyRingSlotMissingSince = now;
				else if(SDL_TICKS_PASSED(now, m_energyRingSlotMissingSince + ASSISTANT_EQUIPMENT_EMPTY_CONFIRM_DELAY))
				{
					m_energyRingActive = false;
					m_energyRingEquippedId = 0;
				}
			}
			else if(equippedRing)
				m_energyRingSlotMissingSince = 0;

			// Old 7.x servers may reject a direct exchange with an occupied equipment
			// slot. Vacate the current ring first and equip the emergency ring on the
			// following fast retry (normally within 80 ms).
			if(!m_energyRingActive && !m_energyRingEquipPending && SDL_TICKS_PASSED(now, m_nextEnergyRingRetry))
			{
				m_energyRingPreviousId = equippedRingId;
				bool actionSent = false;
				if(equippedRing)
					actionSent = unequipItem(SLOT_RING, 0);
				else
					actionSent = equipItem(SLOT_RING, m_settings.energyRingId, m_settings.energyRingSubtype);

				if(actionSent && !equippedRing)
				{
					m_energyRingEquipPending = true;
					m_energyRingRequestTime = now;
				}
				m_nextEnergyRingRetry = now + ASSISTANT_EQUIPMENT_RETRY_DELAY;
			}
		}
		else if(healthPercent >= m_settings.energyRingUnequipPercent)
		{
			m_energyRingSlotMissingSince = 0;

			// If the equip crossed the threshold while its packet was in flight,
			// wait for the slot change and remove it on the very next tick. If the
			// packet never lands, the pending state expires above.
			if(m_energyRingActive)
			{
				// Never remove an unrelated ring that the player equipped manually.
				if(equippedRing && m_energyRingEquippedId != 0 && equippedRingId != m_energyRingEquippedId)
				{
					m_energyRingActive = false;
					m_energyRingEquippedId = 0;
					m_energyRingUnequipPending = false;
				}
				else if(!equippedRing)
				{
					m_energyRingActive = false;
					m_energyRingEquippedId = 0;
					m_energyRingUnequipPending = false;
				}
				else if(!m_energyRingUnequipPending && SDL_TICKS_PASSED(now, m_nextEnergyRingRetry))
				{
					// Remove the Energy Ring first. The normal-ring maintenance below
					// equips the selected ring as soon as the server confirms an empty slot.
					bool actionSent = unequipItem(SLOT_RING, 0);
					m_energyRingRestoreTargetId = 0;

					if(actionSent)
					{
						m_energyRingUnequipPending = true;
						m_energyRingRequestTime = now;
					}
					m_nextEnergyRingRetry = now + ASSISTANT_EQUIPMENT_RETRY_DELAY;
				}
			}

			// Keep the selected normal ring equipped whenever HP is safely above the
			// removal threshold, including after login or a manual removal. This does
			// not depend on the previous Energy Ring latch.
			if(!m_energyRingActive && !m_energyRingUnequipPending &&
				m_settings.preferredRingId != 0 &&
				!isConfiguredRingId(equippedRingId, m_settings.preferredRingId) &&
				SDL_TICKS_PASSED(now, m_nextEnergyRingRetry))
			{
				Position preferredPosition;
				Uint8 preferredStackpos = m_settings.preferredRingSubtype;
				if(findItem(m_settings.preferredRingId, m_settings.preferredRingSubtype, preferredPosition, preferredStackpos))
				{
					if(equippedRing)
						unequipItem(SLOT_RING, 0);
					else
						equipItem(SLOT_RING, m_settings.preferredRingId, m_settings.preferredRingSubtype);
					m_nextEnergyRingRetry = now + ASSISTANT_EQUIPMENT_RETRY_DELAY;
				}
			}
		}
	}

	if(!SDL_TICKS_PASSED(now, m_nextAction))
		return;

	bool healingConfigured = (m_settings.healingUseRune ? m_settings.healingRuneId != 0 : !m_settings.healingSpell.empty());
	if(m_settings.healingEnabled && healingConfigured && g_game.getPlayerHealthPercent() > 0 && g_game.getPlayerHealthPercent() <= m_settings.healingPercent)
	{
		bool actionSent = true;
		if(m_settings.healingUseRune)
			actionSent = useOnSelf(m_settings.healingRuneId, m_settings.healingRuneSubtype);
		else
			castSpell(m_settings.healingSpell);
		m_nextAction = now + (actionSent ? ASSISTANT_ACTION_DELAY : 3000);
		return;
	}

	// A situacao critica de ring continua acima do treinamento de runas.
	if(m_settings.energyRingEnabled && m_settings.energyRingId != 0 && healthPercent <= m_settings.energyRingEquipPercent)
		return;

	if(tickAutoRuneActions(now))
	{
		m_nextAction = now + ASSISTANT_ACTION_DELAY;
		return;
	}

	// Cura continua acima da munição na prioridade. Quando o slot estiver
	// vazio, equipe a pilha configurada sem depender do subtipo/quantidade.
	if(m_settings.ammunitionEnabled && m_settings.ammunitionId != 0 &&
		!g_game.getInventoryItem(SLOT_AMMO) && SDL_TICKS_PASSED(now, m_nextAmmunitionRetry))
	{
		if(equipAmmunition(m_settings.ammunitionId))
		{
			m_nextAmmunitionRetry = now + 150;
			m_nextAction = now + 150;
			return;
		}
		m_nextAmmunitionRetry = now + 1000;
	}

	if(m_settings.amuletEnabled && SDL_TICKS_PASSED(now, m_nextAmuletRetry))
	{
		Uint16 desiredId = (healthPercent <= m_settings.amuletSwitchPercent) ? m_settings.lowHealthAmuletId : m_settings.highHealthAmuletId;
		Uint8 desiredSubtype = (healthPercent <= m_settings.amuletSwitchPercent) ? m_settings.lowHealthAmuletSubtype : m_settings.highHealthAmuletSubtype;
		if(desiredId != 0 && !isEquipped(SLOT_NECKLACE, desiredId))
		{
			bool actionSent = equipItem(SLOT_NECKLACE, desiredId, desiredSubtype);
			if(actionSent)
			{
				m_nextAction = now + ASSISTANT_ACTION_DELAY;
				return;
			}
			m_nextAmuletRetry = now + 3000;
		}
	}

	if(m_settings.manaEnabled && m_settings.manaFluidId != 0 && g_game.getPlayerManaPercent() <= m_settings.manaPercent)
	{
		bool actionSent = useOnSelf(m_settings.manaFluidId, m_settings.manaFluidSubtype);
		m_nextAction = now + (actionSent ? ASSISTANT_ACTION_DELAY : 3000);
		return;
	}

	if(m_settings.hasteEnabled && !m_settings.hasteSpell.empty() && (g_game.getIcons() & ICON_HASTE) == 0 && SDL_TICKS_PASSED(now, m_nextHaste))
	{
		castSpell(m_settings.hasteSpell);
		m_nextAction = now + ASSISTANT_ACTION_DELAY;
		m_nextHaste = now + ASSISTANT_HASTE_DELAY;
	}
}

void Assistant::ensureCharacterProfile()
{
	if(!g_engine.isIngame() || g_game.getPlayerID() == 0)
		return;
	Creature* local = g_map.getLocalCreature();
	if(!local || local->getName().empty() || local->getName() == m_profileCharacter)
		return;
	loadCharacterConfig(local->getName());
}

void Assistant::resetRuntime()
{
	m_nextAction = 0;
	m_nextHaste = 0;
	m_nextEnergyRingRetry = 0;
	m_nextAmuletRetry = 0;
	m_nextAmmunitionRetry = 0;
	m_nextRuneCast = 0;
	m_nextRuneFood = 0;
	m_nextRuneIdle = 0;
	m_autoRuneSessionStart = 0;
	m_autoRuneLogoutIssued = false;
	m_energyRingRequestTime = 0;
	m_energyRingSlotMissingSince = 0;
	m_energyRingPreviousId = 0;
	m_energyRingRestoreTargetId = 0;
	m_energyRingEquipPending = false;
	m_energyRingUnequipPending = false;
	// Preserve the emergency-ring latch while settings are saved. A normal ring
	// placed manually must never be classified as an Assistant-equipped ring.
	if(!g_game.getInventoryItem(SLOT_RING))
	{
		m_energyRingActive = false;
		m_energyRingEquippedId = 0;
	}
}

void Assistant::prepareShutdown()
{
	if(m_settings.fullLight)
	{
		bool keepEnabled = m_settings.fullLight;
		applyFullLight(false);
		m_settings.fullLight = keepEnabled;
	}
}
