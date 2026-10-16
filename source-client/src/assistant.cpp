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
#include "position.h"
#include "GUI/itemUI.h"

extern Engine g_engine;
extern Game g_game;
extern std::string g_prefPath;

Assistant g_assistant;

namespace
{
	const Uint32 ASSISTANT_ACTION_DELAY = 650;
	const Uint32 ASSISTANT_HASTE_DELAY = 2000;
	const Uint32 ASSISTANT_EQUIPMENT_RETRY_DELAY = 250;

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
}

void Assistant::loadConfig()
{
	AssistantSettings settings;
	Config config;
	if(config.openToRead((g_prefPath + "assistant.cfg").c_str()))
	{
		settings.enabled = readBool(config, "Enabled", settings.enabled);
		settings.fullLight = readBool(config, "FullLight", settings.fullLight);
		settings.healingEnabled = readBool(config, "HealingEnabled", settings.healingEnabled);
		settings.healingUseRune = readBool(config, "HealingUseRune", settings.healingUseRune);
		settings.healingPercent = SDL_static_cast(Uint8, readNumber(config, "HealingPercent", settings.healingPercent, 100));
		settings.healingSpell = readText(config, "HealingSpell", settings.healingSpell);
		settings.healingRuneId = SDL_static_cast(Uint16, readNumber(config, "HealingRuneId", settings.healingRuneId, 65535));
		settings.healingRuneSubtype = SDL_static_cast(Uint8, readNumber(config, "HealingRuneSubtype", settings.healingRuneSubtype, 255));
		settings.hasteEnabled = readBool(config, "HasteEnabled", settings.hasteEnabled);
		settings.hasteSpell = readText(config, "HasteSpell", settings.hasteSpell);
		settings.manaEnabled = readBool(config, "ManaEnabled", settings.manaEnabled);
		settings.manaPercent = SDL_static_cast(Uint8, readNumber(config, "ManaPercent", settings.manaPercent, 100));
		settings.manaFluidId = SDL_static_cast(Uint16, readNumber(config, "ManaFluidId", settings.manaFluidId, 65535));
		settings.manaFluidSubtype = SDL_static_cast(Uint8, readNumber(config, "ManaFluidSubtype", settings.manaFluidSubtype, 255));
		settings.energyRingEnabled = readBool(config, "EnergyRingEnabled", settings.energyRingEnabled);
		settings.energyRingEquipPercent = SDL_static_cast(Uint8, readNumber(config, "EnergyRingEquipPercent", settings.energyRingEquipPercent, 100));
		settings.energyRingUnequipPercent = SDL_static_cast(Uint8, readNumber(config, "EnergyRingUnequipPercent", settings.energyRingUnequipPercent, 100));
		settings.energyRingId = SDL_static_cast(Uint16, readNumber(config, "EnergyRingId", settings.energyRingId, 65535));
		settings.energyRingSubtype = SDL_static_cast(Uint8, readNumber(config, "EnergyRingSubtype", settings.energyRingSubtype, 255));
		settings.amuletEnabled = readBool(config, "AmuletEnabled", settings.amuletEnabled);
		settings.amuletSwitchPercent = SDL_static_cast(Uint8, readNumber(config, "AmuletSwitchPercent", settings.amuletSwitchPercent, 100));
		settings.lowHealthAmuletId = SDL_static_cast(Uint16, readNumber(config, "LowHealthAmuletId", settings.lowHealthAmuletId, 65535));
		settings.lowHealthAmuletSubtype = SDL_static_cast(Uint8, readNumber(config, "LowHealthAmuletSubtype", settings.lowHealthAmuletSubtype, 255));
		settings.highHealthAmuletId = SDL_static_cast(Uint16, readNumber(config, "HighHealthAmuletId", settings.highHealthAmuletId, 65535));
		settings.highHealthAmuletSubtype = SDL_static_cast(Uint8, readNumber(config, "HighHealthAmuletSubtype", settings.highHealthAmuletSubtype, 255));
	}

	m_configLoaded = true;
	applySettings(settings);
}

void Assistant::saveConfig()
{
	Config config;
	if(!config.openToSave((g_prefPath + "assistant.cfg").c_str()))
		return;

	config.insertKey("Enabled", boolText(m_settings.enabled));
	config.insertKey("FullLight", boolText(m_settings.fullLight));
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
	config.insertKey("AmuletEnabled", boolText(m_settings.amuletEnabled));
	config.insertKey("AmuletSwitchPercent", numberText(m_settings.amuletSwitchPercent));
	config.insertKey("LowHealthAmuletId", numberText(m_settings.lowHealthAmuletId));
	config.insertKey("LowHealthAmuletSubtype", numberText(m_settings.lowHealthAmuletSubtype));
	config.insertKey("HighHealthAmuletId", numberText(m_settings.highHealthAmuletId));
	config.insertKey("HighHealthAmuletSubtype", numberText(m_settings.highHealthAmuletSubtype));
}

void Assistant::applySettings(const AssistantSettings& settings)
{
	bool oldFullLight = m_settings.fullLight;
	m_settings = settings;
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

void Assistant::tick()
{
	if(!m_configLoaded)
		return;

	if(m_settings.fullLight && g_engine.getLightMode() != CLIENT_LIGHT_MODE_NONE)
		applyFullLight(true);

	if(!m_settings.enabled || !g_engine.isIngame() || g_game.getPlayerID() == 0)
		return;

	Uint32 now = SDL_GetTicks();
	const Uint8 healthPercent = g_game.getPlayerHealthPercent();
	if(healthPercent == 0)
		return;

	// Energy Ring is an emergency action. It must not wait for the shared
	// healing/mana/haste delay and it must be able to replace a normal ring.
	if(m_settings.energyRingEnabled && m_settings.energyRingId != 0 && SDL_TICKS_PASSED(now, m_nextEnergyRingRetry))
	{
		ItemUI* equippedRing = g_game.getInventoryItem(SLOT_RING);
		if(m_energyRingActive && !equippedRing)
			m_energyRingActive = false;

		if(healthPercent <= m_settings.energyRingEquipPercent && !m_energyRingActive && !isEquipped(SLOT_RING, m_settings.energyRingId))
		{
			bool actionSent = equipItem(SLOT_RING, m_settings.energyRingId, m_settings.energyRingSubtype);
			if(actionSent)
			{
				// The server changes an active Energy Ring to another item id. Keep a
				// local latch so that the transformed ring is not replaced every tick,
				// starving higher-priority healing actions.
				m_energyRingActive = true;
				m_nextEnergyRingRetry = now + ASSISTANT_EQUIPMENT_RETRY_DELAY;
			}
			else
				m_nextEnergyRingRetry = now + ASSISTANT_EQUIPMENT_RETRY_DELAY;
		}
		else if(healthPercent >= m_settings.energyRingUnequipPercent && m_energyRingActive)
		{
			if(equippedRing)
			{
				// The transformed id is unknown, so remove the slot only while the
				// Assistant knows that it placed the emergency ring.
				bool actionSent = unequipItem(SLOT_RING, 0);
				if(actionSent)
				{
					m_energyRingActive = false;
					m_nextEnergyRingRetry = now + ASSISTANT_EQUIPMENT_RETRY_DELAY;
				}
				else
					m_nextEnergyRingRetry = now + ASSISTANT_EQUIPMENT_RETRY_DELAY;
			}
			else
				m_energyRingActive = false;
		}
	}

	if(!SDL_TICKS_PASSED(now, m_nextAction))
		return;

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

void Assistant::resetRuntime()
{
	m_nextAction = 0;
	m_nextHaste = 0;
	m_nextEnergyRingRetry = 0;
	m_nextAmuletRetry = 0;
	// Preserve the emergency-ring latch while settings are saved. A normal ring
	// placed manually must never be classified as an Assistant-equipped ring.
	if(!g_game.getInventoryItem(SLOT_RING))
		m_energyRingActive = false;
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
