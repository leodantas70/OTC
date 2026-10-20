/* Native modular CaveBot. */

#include "cavebot.h"
#include "assistant.h"
#include "config.h"
#include "container.h"
#include "creature.h"
#include "engine.h"
#include "game.h"
#include "item.h"
#include "map.h"
#include "thing.h"
#include "thingManager.h"
#include "tile.h"
#include "GUI/itemUI.h"
#include "GUI/GUI_UTIL.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

extern Engine g_engine;
extern Game g_game;
extern Map g_map;
extern std::string g_prefPath;

CaveBot g_caveBot;

namespace
{
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

	std::string trimLower(const std::string& input)
	{
		size_t begin = input.find_first_not_of(" \t\r\n");
		if(begin == std::string::npos)
			return std::string();
		size_t end = input.find_last_not_of(" \t\r\n");
		std::string result = input.substr(begin, end - begin + 1);
		std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) {return SDL_static_cast(char, std::tolower(c));});
		return result;
	}

	std::vector<std::string> split(const std::string& input, char delimiter, size_t maximumParts = 0)
	{
		std::vector<std::string> result;
		size_t start = 0;
		while(start <= input.size())
		{
			if(maximumParts > 0 && result.size() + 1 == maximumParts)
			{
				result.push_back(input.substr(start));
				break;
			}
			size_t separator = input.find(delimiter, start);
			if(separator == std::string::npos)
			{
				result.push_back(input.substr(start));
				break;
			}
			result.push_back(input.substr(start, separator - start));
			start = separator + 1;
		}
		return result;
	}

	bool idInList(Uint16 id, const std::string& text)
	{
		std::vector<std::string> values = split(text, ',');
		for(const std::string& value : values)
		{
			if(SDL_static_cast(Uint16, SDL_strtoul(value.c_str(), NULL, 10)) == id)
				return true;
		}
		return false;
	}

	bool isCurrencyCoin(Uint16 itemId)
	{
		// Item ids used by the 7.72 assets: gold, platinum and crystal coin.
		return (itemId == 3031 || itemId == 3035 || itemId == 3043);
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

	std::string caveBotConfigPath(const std::string& characterName)
	{
		std::string key = profileKey(characterName);
		return (key.empty() ? g_prefPath + "cavebot.cfg" : g_prefPath + "cavebot-" + key + ".cfg");
	}
}

void CaveBot::loadConfig()
{
	loadCharacterConfig(std::string());
}

void CaveBot::loadCharacterConfig(const std::string& characterName)
{
	CaveBotSettings settings;
	std::vector<CaveBotWaypoint> waypoints;
	Config profileConfig;
	Config legacyConfig;
	Config* config = &profileConfig;
	bool loadedProfile = profileConfig.openToRead(caveBotConfigPath(characterName).c_str());
	bool loaded = loadedProfile;
	if(!loaded && !characterName.empty())
	{
		loaded = legacyConfig.openToRead((g_prefPath + "cavebot.cfg").c_str());
		config = &legacyConfig;
	}
	if(loaded)
	{
		settings.enabled = readBool(*config, "Enabled", settings.enabled);
		settings.caveEnabled = readBool(*config, "CaveEnabled", settings.caveEnabled);
		settings.recordEnabled = readBool(*config, "RecordEnabled", settings.recordEnabled);
		settings.loop = readBool(*config, "Loop", settings.loop);
		settings.pauseWhileTarget = readBool(*config, "PauseWhileTarget", settings.pauseWhileTarget);
		settings.reachDistance = SDL_static_cast(Uint8, readNumber(*config, "ReachDistance", settings.reachDistance, 5));
		settings.retryDelay = readNumber(*config, "RetryDelay", settings.retryDelay, 10000);
		settings.targetEnabled = readBool(*config, "TargetEnabled", settings.targetEnabled);
		settings.targetNames = readText(*config, "TargetNames", settings.targetNames);
		settings.attackRuneId = SDL_static_cast(Uint16, readNumber(*config, "AttackRuneId", settings.attackRuneId, 65535));
		settings.attackRuneSubtype = SDL_static_cast(Uint8, readNumber(*config, "AttackRuneSubtype", settings.attackRuneSubtype, 255));
		settings.attackDelay = readNumber(*config, "AttackDelay", settings.attackDelay, 10000);
		settings.targetMovement = SDL_static_cast(CaveBotTargetMovement, readNumber(*config, "TargetMovement", settings.targetMovement, CAVEBOT_TARGET_DIAGONAL_DISTANCE_2));
		settings.revealInvisibleMonsters = readBool(*config, "RevealInvisibleMonsters", settings.revealInvisibleMonsters);
		settings.lootEnabled = readBool(*config, "LootEnabled", settings.lootEnabled);
		settings.lootGold = readBool(*config, "LootGold", settings.lootGold);
		settings.lootAfterKill = readBool(*config, "LootAfterKill", settings.lootAfterKill);
		settings.dropLootNoCapacity = readBool(*config, "DropLootNoCapacity", settings.dropLootNoCapacity);
		settings.dropLootCapacity = readNumber(*config, "DropLootCapacity", settings.dropLootCapacity, 100000);
		settings.dropGoldNoCapacity = readBool(*config, "DropGoldNoCapacity", settings.dropLootNoCapacity);
		settings.dropGoldCapacity = readNumber(*config, "DropGoldCapacity", settings.dropLootCapacity, 100000);
		settings.lootItemIds = readText(*config, "LootItemIds", settings.lootItemIds);
		settings.lootDestinationContainer = SDL_static_cast(Uint8, readNumber(*config, "LootDestination", settings.lootDestinationContainer, GAME_MAX_CONTAINERS - 1));
		settings.playerAlertEnabled = readBool(*config, "PlayerAlertEnabled", settings.playerAlertEnabled);
		settings.pauseOnPlayer = readBool(*config, "PauseOnPlayer", settings.pauseOnPlayer);
		settings.ignoredPlayers = readText(*config, "IgnoredPlayers", settings.ignoredPlayers);
		settings.burstArrowEnabled = readBool(*config, "BurstArrowEnabled", settings.burstArrowEnabled);

		StringVector savedWaypoints = config->fetchKeys("Waypoint");
		for(const std::string& encoded : savedWaypoints)
		{
			std::vector<std::string> fields = split(encoded, ',', 8);
			if(fields.size() < 7)
				continue;
			CaveBotWaypoint waypoint;
			waypoint.action = SDL_static_cast(CaveBotWaypointAction, UTIL_min<Uint32>(CAVEBOT_EXANI, SDL_static_cast(Uint32, SDL_strtoul(fields[0].c_str(), NULL, 10))));
			waypoint.position.x = SDL_static_cast(Uint16, SDL_strtoul(fields[1].c_str(), NULL, 10));
			waypoint.position.y = SDL_static_cast(Uint16, SDL_strtoul(fields[2].c_str(), NULL, 10));
			waypoint.position.z = SDL_static_cast(Uint8, SDL_strtoul(fields[3].c_str(), NULL, 10));
			waypoint.itemId = SDL_static_cast(Uint16, SDL_strtoul(fields[4].c_str(), NULL, 10));
			waypoint.itemSubtype = SDL_static_cast(Uint8, SDL_strtoul(fields[5].c_str(), NULL, 10));
			waypoint.delay = SDL_static_cast(Uint32, SDL_strtoul(fields[6].c_str(), NULL, 10));
			if(fields.size() >= 8)
				waypoint.text = fields[7];
			waypoints.push_back(waypoint);
		}

		// Kept in a separate key so old waypoint files (whose final text field may
		// contain commas) remain fully compatible.
		StringVector savedTargetItems = config->fetchKeys("WaypointTargetItem");
		for(const std::string& encoded : savedTargetItems)
		{
			std::vector<std::string> fields = split(encoded, ',', 2);
			if(fields.size() != 2)
				continue;
			size_t index = SDL_static_cast(size_t, SDL_strtoul(fields[0].c_str(), NULL, 10));
			if(index < waypoints.size())
				waypoints[index].targetItemId = SDL_static_cast(Uint16, SDL_strtoul(fields[1].c_str(), NULL, 10));
		}
	}

	m_profileCharacter = characterName;
	m_waypoints.swap(waypoints);
	m_currentWaypoint = 0;
	m_configLoaded = true;
	applySettings(settings);
	if(!characterName.empty() && !loadedProfile)
		saveConfig();
}

void CaveBot::saveConfig()
{
	Config config;
	if(!config.openToSave(caveBotConfigPath(m_profileCharacter).c_str()))
		return;

	config.insertKey("Enabled", boolText(m_settings.enabled));
	config.insertKey("CaveEnabled", boolText(m_settings.caveEnabled));
	config.insertKey("RecordEnabled", boolText(m_settings.recordEnabled));
	config.insertKey("Loop", boolText(m_settings.loop));
	config.insertKey("PauseWhileTarget", boolText(m_settings.pauseWhileTarget));
	config.insertKey("ReachDistance", numberText(m_settings.reachDistance));
	config.insertKey("RetryDelay", numberText(m_settings.retryDelay));
	config.insertKey("TargetEnabled", boolText(m_settings.targetEnabled));
	config.insertKey("TargetNames", m_settings.targetNames);
	config.insertKey("AttackRuneId", numberText(m_settings.attackRuneId));
	config.insertKey("AttackRuneSubtype", numberText(m_settings.attackRuneSubtype));
	config.insertKey("AttackDelay", numberText(m_settings.attackDelay));
	config.insertKey("TargetMovement", numberText(m_settings.targetMovement));
	config.insertKey("RevealInvisibleMonsters", boolText(m_settings.revealInvisibleMonsters));
	config.insertKey("LootEnabled", boolText(m_settings.lootEnabled));
	config.insertKey("LootGold", boolText(m_settings.lootGold));
	config.insertKey("LootAfterKill", boolText(m_settings.lootAfterKill));
	config.insertKey("DropLootNoCapacity", boolText(m_settings.dropLootNoCapacity));
	config.insertKey("DropLootCapacity", numberText(m_settings.dropLootCapacity));
	config.insertKey("DropGoldNoCapacity", boolText(m_settings.dropGoldNoCapacity));
	config.insertKey("DropGoldCapacity", numberText(m_settings.dropGoldCapacity));
	config.insertKey("LootItemIds", m_settings.lootItemIds);
	config.insertKey("LootDestination", numberText(m_settings.lootDestinationContainer));
	config.insertKey("PlayerAlertEnabled", boolText(m_settings.playerAlertEnabled));
	config.insertKey("PauseOnPlayer", boolText(m_settings.pauseOnPlayer));
	config.insertKey("IgnoredPlayers", m_settings.ignoredPlayers);
	config.insertKey("BurstArrowEnabled", boolText(m_settings.burstArrowEnabled));
	config.insertNewLine();

	for(size_t index = 0; index < m_waypoints.size(); ++index)
	{
		const CaveBotWaypoint& waypoint = m_waypoints[index];
		std::ostringstream stream;
		stream << SDL_static_cast(Uint32, waypoint.action) << ',' << waypoint.position.x << ',' << waypoint.position.y << ','
			<< SDL_static_cast(Uint32, waypoint.position.z) << ',' << waypoint.itemId << ',' << SDL_static_cast(Uint32, waypoint.itemSubtype)
			<< ',' << waypoint.delay << ',' << waypoint.text;
		config.insertKey("Waypoint", stream.str());
		if(waypoint.targetItemId != 0)
		{
			std::ostringstream targetStream;
			targetStream << index << ',' << waypoint.targetItemId;
			config.insertKey("WaypointTargetItem", targetStream.str());
		}
	}
}

void CaveBot::applySettings(const CaveBotSettings& settings)
{
	bool recordChanged = (m_settings.recordEnabled != settings.recordEnabled);
	bool revealInvisibleChanged = (m_settings.revealInvisibleMonsters != settings.revealInvisibleMonsters);
	size_t currentWaypoint = m_currentWaypoint;
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
	m_settings.reachDistance = UTIL_min<Uint8>(5, m_settings.reachDistance);
	m_settings.retryDelay = UTIL_max<Uint32>(250, UTIL_min<Uint32>(10000, m_settings.retryDelay));
	m_settings.attackDelay = UTIL_max<Uint32>(250, UTIL_min<Uint32>(10000, m_settings.attackDelay));
	m_settings.dropLootCapacity = UTIL_min<Uint32>(100000, m_settings.dropLootCapacity);
	m_settings.dropGoldCapacity = UTIL_min<Uint32>(100000, m_settings.dropGoldCapacity);
	if(m_settings.targetMovement > CAVEBOT_TARGET_DIAGONAL_DISTANCE_2)
		m_settings.targetMovement = CAVEBOT_TARGET_STAND;
	resetRuntime();
	if(currentWaypoint < m_waypoints.size())
		m_currentWaypoint = currentWaypoint;
	if(recordChanged)
		m_recordPositionValid = false;
}

void CaveBot::setCurrentWaypoint(size_t index)
{
	if(index >= m_waypoints.size())
		return;
	g_game.stopAutoWalk();
	m_currentWaypoint = index;
	m_nextWalkAction = 0;
	m_lastWalkRequest = 0;
	m_walkPositionValid = false;
	m_reselectWaypointAfterCombat = false;
	if(g_engine.isIngame())
		g_game.processTextMessage(MessageLogin, std::string("CaveBot: waypoint selecionado: ") + numberText(SDL_static_cast(Uint32, index + 1)) + ".");
}

void CaveBot::notifyUserInput()
{
	Uint32 now = SDL_GetTicks();
	if(m_playerAlertSoundUntil == 0 || SDL_TICKS_PASSED(now, m_playerAlertSoundUntil))
		return;
	if(m_playerAlertMutedUntil != 0 && !SDL_TICKS_PASSED(now, m_playerAlertMutedUntil))
		return;
	m_playerAlertSoundUntil = 0;
	m_playerAlertMutedUntil = now + 60000;
}

void CaveBot::setEnabled(bool enabled)
{
	m_settings.enabled = enabled;
	resetRuntime();
	saveConfig();
	if(g_engine.isIngame())
		g_game.processTextMessage(enabled ? MessageLogin : MessageFailure, enabled ? "CaveBot ligado." : "CaveBot desligado.");
}

void CaveBot::resetRuntime()
{
	restoreTargetMovement();
	m_currentWaypoint = 0;
	m_nextWalkAction = 0;
	m_lastWalkRequest = 0;
	m_walkNoProgressSince = 0;
	m_walkPositionValid = false;
	m_reselectWaypointAfterCombat = false;
	m_floorTransitionPending = false;
	m_floorTransitionSourceZ = 0;
	m_floorTransitionSince = 0;
	m_nextTargetAction = 0;
	m_nextTargetMovement = 0;
	m_recentAttackTarget = 0;
	m_recentAttackAt = 0;
	m_nextLootAction = 0;
	m_nextPlayerAlert = 0;
	m_nextPlayerAlertBeep = 0;
	m_playerAlertSoundUntil = 0;
	m_playerAlertMutedUntil = 0;
	m_playerAlertWasDetected = false;
	m_nextBurstArrowRetry = 0;
	m_lootCorpseContainer = 0xFF;
	m_lootBagContainer = 0xFF;
	m_lootContainerRequest = 0;
	m_lootOpenAttempts = 0;
	m_lootCorpseBagScanned = false;
	m_lootTargets.clear();
	m_lastCorpseItemId = 0;
	m_lastCorpseStack = 0;
	m_playerDetected = false;
}

void CaveBot::notifyCreatureDeath(Uint32 creatureId, const Position& position)
{
	if(!m_configLoaded || !m_settings.enabled || !m_settings.lootEnabled)
		return;
	Uint32 now = SDL_GetTicks();
	bool wasOurTarget = (g_game.getAttackID() == creatureId ||
		(m_recentAttackTarget == creatureId && !SDL_TICKS_PASSED(now, m_recentAttackAt + 15000)));
	if(!wasOurTarget)
		return;

	for(const LootTarget& target : m_lootTargets)
	{
		if(target.creatureId == creatureId)
			return;
	}

	LootTarget target;
	target.creatureId = creatureId;
	target.position = position;
	target.queuedAt = now;
	m_lootTargets.push_back(target);
}

void CaveBot::notifyManualUse(const Position& targetPosition, Uint16 targetItemId)
{
	if(!m_configLoaded || !m_settings.recordEnabled || targetPosition.x == 0xFFFF || targetItemId == 0)
		return;

	CaveBotWaypoint waypoint;
	waypoint.position = targetPosition;
	waypoint.action = CAVEBOT_USE;
	waypoint.targetItemId = targetItemId;
	waypoint.delay = 300;
	m_waypoints.push_back(waypoint);
	m_recordDirty = true;
	UTIL_caveBotWaypointRecorded(waypointDescription(m_waypoints.size() - 1));
}

void CaveBot::notifyManualUseWith(const Position& sourcePosition, Uint16 sourceItemId,
	const Position& targetPosition, Uint16 targetItemId)
{
	if(!m_configLoaded || !m_settings.recordEnabled || targetPosition.x == 0xFFFF ||
		sourceItemId == 0 || targetItemId == 0)
		return;

	Uint8 sourceSubtype = 0;
	if(sourcePosition.x == 0xFFFF)
	{
		for(Uint8 slot = SLOT_HEAD; slot < SLOT_LAST; ++slot)
		{
			ItemUI* item = g_game.getInventoryItem(slot);
			if(item && item->getID() == sourceItemId && item->getCurrentPosition() == sourcePosition)
			{
				sourceSubtype = item->getItemSubtype();
				break;
			}
		}
		for(Uint8 containerId = 0; sourceSubtype == 0 && containerId < GAME_MAX_CONTAINERS; ++containerId)
		{
			Container* container = g_game.findContainer(containerId);
			if(!container)
				continue;
			for(ItemUI* item : container->getItems())
			{
				if(item && item->getID() == sourceItemId && item->getCurrentPosition() == sourcePosition)
				{
					sourceSubtype = item->getItemSubtype();
					break;
				}
			}
		}
	}

	CaveBotWaypoint waypoint;
	waypoint.position = targetPosition;
	waypoint.action = CAVEBOT_USE;
	waypoint.itemId = sourceItemId;
	waypoint.targetItemId = targetItemId;
	waypoint.itemSubtype = sourceSubtype;
	waypoint.delay = 300;
	m_waypoints.push_back(waypoint);
	m_recordDirty = true;
	UTIL_caveBotWaypointRecorded(waypointDescription(m_waypoints.size() - 1));
}

void CaveBot::addWaypoint(const CaveBotWaypoint& waypoint)
{
	m_waypoints.push_back(waypoint);
	saveConfig();
}

void CaveBot::removeWaypoint(size_t index)
{
	if(index >= m_waypoints.size())
		return;
	m_waypoints.erase(m_waypoints.begin() + index);
	if(m_currentWaypoint >= m_waypoints.size())
		m_currentWaypoint = 0;
	saveConfig();
}

void CaveBot::moveWaypoint(size_t index, Sint32 direction)
{
	if(index >= m_waypoints.size())
		return;
	Sint32 destination = SDL_static_cast(Sint32, index) + direction;
	if(destination < 0 || SDL_static_cast(size_t, destination) >= m_waypoints.size())
		return;
	std::swap(m_waypoints[index], m_waypoints[SDL_static_cast(size_t, destination)]);
	saveConfig();
}

void CaveBot::shiftWaypoint(size_t index, Sint32 offsetX, Sint32 offsetY)
{
	if(index >= m_waypoints.size())
		return;
	Sint32 x = SDL_static_cast(Sint32, m_waypoints[index].position.x) + offsetX;
	Sint32 y = SDL_static_cast(Sint32, m_waypoints[index].position.y) + offsetY;
	m_waypoints[index].position.x = SDL_static_cast(Uint16, UTIL_max<Sint32>(0, UTIL_min<Sint32>(65535, x)));
	m_waypoints[index].position.y = SDL_static_cast(Uint16, UTIL_max<Sint32>(0, UTIL_min<Sint32>(65535, y)));
	saveConfig();
}

void CaveBot::clearWaypoints()
{
	m_waypoints.clear();
	m_currentWaypoint = 0;
	saveConfig();
}

std::string CaveBot::waypointDescription(size_t index) const
{
	if(index >= m_waypoints.size())
		return std::string();
	const CaveBotWaypoint& waypoint = m_waypoints[index];
	const char* action = "Walk";
	switch(waypoint.action)
	{
		case CAVEBOT_STAND: action = "Stand"; break;
		case CAVEBOT_USE: action = "Use"; break;
		case CAVEBOT_SAY: action = "Say"; break;
		case CAVEBOT_EXANI: action = "Exani"; break;
		default: break;
	}
	char buffer[256];
	if(waypoint.action == CAVEBOT_USE && waypoint.targetItemId != 0)
	{
		if(waypoint.itemId != 0)
			SDL_snprintf(buffer, sizeof(buffer), "%02u. %-5s  %u,%u,%u  ID:%u->%u%s", SDL_static_cast(Uint32, index + 1), action,
				waypoint.position.x, waypoint.position.y, waypoint.position.z, waypoint.itemId, waypoint.targetItemId,
				(index == m_currentWaypoint ? "  <" : ""));
		else
			SDL_snprintf(buffer, sizeof(buffer), "%02u. %-5s  %u,%u,%u  ID:%u%s", SDL_static_cast(Uint32, index + 1), action,
				waypoint.position.x, waypoint.position.y, waypoint.position.z, waypoint.targetItemId,
				(index == m_currentWaypoint ? "  <" : ""));
	}
	else
		SDL_snprintf(buffer, sizeof(buffer), "%02u. %-5s  %u,%u,%u%s", SDL_static_cast(Uint32, index + 1), action,
			waypoint.position.x, waypoint.position.y, waypoint.position.z,
			(index == m_currentWaypoint ? "  <" : ""));
	return std::string(buffer);
}

void CaveBot::updateRecorder(Uint32 now)
{
	if(!m_settings.recordEnabled)
	{
		m_recordPositionValid = false;
		if(m_recordDirty)
		{
			saveConfig();
			m_recordDirty = false;
		}
		return;
	}
	Creature* player = g_map.getLocalCreature();
	if(!player)
		return;
	Position position = player->getCurrentPosition();
	if(!m_recordPositionValid || position != m_lastRecordPosition)
	{
		CaveBotWaypoint waypoint;
		waypoint.position = position;
		waypoint.action = CAVEBOT_WALK;
		waypoint.delay = 100;
		m_waypoints.push_back(waypoint);
		m_lastRecordPosition = position;
		m_recordPositionValid = true;
		m_recordDirty = true;
		UTIL_caveBotWaypointRecorded(waypointDescription(m_waypoints.size() - 1));
	}
	if(m_recordDirty && now >= m_nextRecordSave)
	{
		saveConfig();
		m_recordDirty = false;
		m_nextRecordSave = now + 2000;
	}
}

bool CaveBot::matchesNameList(const std::string& name, const std::string& list, bool wildcard) const
{
	std::string wantedName = trimLower(name);
	for(const std::string& entry : split(list, ','))
	{
		std::string wanted = trimLower(entry);
		if(wanted.empty())
			continue;
		if(wildcard && wanted == "*")
			return true;
		if(wanted == wantedName)
			return true;
	}
	return false;
}

void CaveBot::updatePlayerAlert(Uint32 now)
{
	if(m_playerAlertSoundUntil != 0 && SDL_TICKS_PASSED(now, m_playerAlertSoundUntil))
		m_playerAlertSoundUntil = 0;
	bool wasDetected = m_playerAlertWasDetected;
	m_playerDetected = false;
	Creature* local = g_map.getLocalCreature();
	if(!m_settings.playerAlertEnabled || !local)
		return;

	Position localPosition = local->getCurrentPosition();
	for(auto& known : g_map.getKnownCreatures())
	{
		Creature* creature = known.second;
		if(!creature || creature == local || !creature->isPlayer() || !creature->isVisible())
			continue;
		if(creature->getCurrentPosition().z != localPosition.z || matchesNameList(creature->getName(), m_settings.ignoredPlayers, false))
			continue;

		m_playerDetected = true;
		if(!wasDetected && SDL_TICKS_PASSED(now, m_playerAlertMutedUntil))
		{
			m_playerAlertSoundUntil = now + 5000;
			m_nextPlayerAlertBeep = 0;
		}
		else if(m_playerAlertMutedUntil != 0 && SDL_TICKS_PASSED(now, m_playerAlertMutedUntil))
		{
			m_playerAlertMutedUntil = 0;
			m_playerAlertSoundUntil = now + 5000;
			m_nextPlayerAlertBeep = 0;
		}
		if(m_playerAlertSoundUntil != 0 && !SDL_TICKS_PASSED(now, m_playerAlertSoundUntil) &&
			SDL_TICKS_PASSED(now, m_nextPlayerAlertBeep))
		{
#ifdef _WIN32
			MessageBeep(MB_ICONEXCLAMATION);
#endif
			m_nextPlayerAlertBeep = now + 650;
		}
		if(now >= m_nextPlayerAlert)
		{
			g_game.processTextMessage(MessageWarning, std::string("ALERTA: jogador na tela: ") + creature->getName());
			SDL_SetWindowTitle(g_engine.m_window, (std::string("ALERTA PLAYER - ") + creature->getName()).c_str());
			m_nextPlayerAlert = now + 3000;
		}
		break;
	}
	m_playerAlertWasDetected = m_playerDetected;
	if(!m_playerDetected)
		m_playerAlertSoundUntil = 0;
}

void CaveBot::updateTargetBot(Uint32 now)
{
	if(!m_settings.targetEnabled || now < m_nextTargetAction || shouldPauseForLoot())
		return;

	Creature* local = g_map.getLocalCreature();
	if(!local)
		return;
	Position localPosition = local->getCurrentPosition();
	Creature* target = g_map.getCreatureById(g_game.getAttackID());
	if(!target || !target->isMonster() || !target->isVisible() || target->getHealth() == 0 ||
		target->getCurrentPosition().z != localPosition.z || !matchesNameList(target->getName(), m_settings.targetNames, true))
	{
		target = NULL;
		Sint32 bestDistance = 0x7FFFFFFF;
		for(auto& known : g_map.getKnownCreatures())
		{
			Creature* creature = known.second;
			if(!creature || !creature->isMonster() || !creature->isVisible() || creature->getHealth() == 0 ||
				creature->getCurrentPosition().z != localPosition.z || !matchesNameList(creature->getName(), m_settings.targetNames, true))
				continue;
			Position position = creature->getCurrentPosition();
			Sint32 distance = UTIL_max<Sint32>(Position::getDistanceX(localPosition, position), Position::getDistanceY(localPosition, position));
			if(distance < bestDistance)
			{
				bestDistance = distance;
				target = creature;
			}
		}
		if(target)
			g_game.sendAttack(target);
	}

	if(target && m_settings.attackRuneId != 0 && !g_assistant.shouldBlockTargetRune())
	{
		Position runePosition;
		Uint8 stackpos = m_settings.attackRuneSubtype;
		if(g_assistant.findItem(m_settings.attackRuneId, m_settings.attackRuneSubtype, runePosition, stackpos))
			g_game.sendUseOnCreature(runePosition, m_settings.attackRuneId, stackpos, target->getId());
	}
	if(target)
	{
		m_recentAttackTarget = target->getId();
		m_recentAttackAt = now;
	}
	m_nextTargetAction = now + m_settings.attackDelay;
}

void CaveBot::restoreTargetMovement()
{
	if(m_targetMovementWalking && g_engine.isIngame())
		g_game.stopAutoWalk();
	m_targetMovementWalking = false;
	if(m_targetChaseOverride)
	{
		g_engine.setChaseMode(m_previousChaseMode);
		if(g_engine.isIngame())
			g_game.sendAttackModes();
		m_targetChaseOverride = false;
	}
}

void CaveBot::updateTargetMovement(Uint32 now)
{
	Creature* local = g_map.getLocalCreature();
	Creature* target = g_map.getCreatureById(g_game.getAttackID());
	if(!m_settings.targetEnabled || shouldPauseForLoot() || !local || !target || !target->isMonster() ||
		!target->isVisible() || target->getHealth() == 0 || target->getCurrentPosition().z != local->getCurrentPosition().z)
	{
		restoreTargetMovement();
		return;
	}

	Uint8 wantedChase = (m_settings.targetMovement == CAVEBOT_TARGET_FOLLOW ? CHASEMODE_FOLLOW : CHASEMODE_STAND);
	if(!m_targetChaseOverride)
	{
		m_previousChaseMode = g_engine.getChaseMode();
		m_targetChaseOverride = true;
		// Cancela imediatamente qualquer caminho que a rota estava executando.
		// O modo escolhido abaixo assume o controle do movimento durante o combate.
		g_game.stopAutoWalk();
	}
	if(g_engine.getChaseMode() != wantedChase)
	{
		g_engine.setChaseMode(wantedChase);
		g_game.sendAttackModes();
	}

	bool diagonalMovement = (m_settings.targetMovement == CAVEBOT_TARGET_DIAGONAL ||
		m_settings.targetMovement == CAVEBOT_TARGET_STAND_DIAGONAL ||
		m_settings.targetMovement == CAVEBOT_TARGET_DIAGONAL_DISTANCE_2 ||
		m_settings.targetMovement == CAVEBOT_TARGET_DIAGONAL_DISTANCE_3 ||
		m_settings.targetMovement == CAVEBOT_TARGET_DIAGONAL_DISTANCE_4 ||
		m_settings.targetMovement == CAVEBOT_TARGET_DIAGONAL_DISTANCE_5);
	if(!diagonalMovement)
	{
		if(m_targetMovementWalking)
			g_game.stopAutoWalk();
		m_targetMovementWalking = false;
		return;
	}
	if(now < m_nextTargetMovement)
		return;

	Position current = local->getCurrentPosition();
	Position targetPosition = target->getCurrentPosition();
	Sint32 deltaX = Position::getDistanceX(current, targetPosition);
	Sint32 deltaY = Position::getDistanceY(current, targetPosition);

	Sint32 wantedDistance = 1;
	if(m_settings.targetMovement == CAVEBOT_TARGET_DIAGONAL_DISTANCE_2)
		wantedDistance = 2;
	else if(m_settings.targetMovement == CAVEBOT_TARGET_DIAGONAL_DISTANCE_3)
		wantedDistance = 3;
	else if(m_settings.targetMovement == CAVEBOT_TARGET_DIAGONAL_DISTANCE_4)
		wantedDistance = 4;
	else if(m_settings.targetMovement == CAVEBOT_TARGET_DIAGONAL_DISTANCE_5)
		wantedDistance = 5;

	if(m_settings.targetMovement == CAVEBOT_TARGET_STAND_DIAGONAL && UTIL_max<Sint32>(deltaX, deltaY) > 1)
	{
		if(m_targetMovementWalking)
			g_game.stopAutoWalk();
		m_targetMovementWalking = false;
		m_nextTargetMovement = now + 150;
		return;
	}
	if(deltaX == wantedDistance && deltaY == wantedDistance)
	{
		if(m_targetMovementWalking)
			g_game.stopAutoWalk();
		m_targetMovementWalking = false;
		m_nextTargetMovement = now + 150;
		return;
	}

	static const Sint32 diagonalOffsets[4][2] = {{-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
	struct MovementCandidate
	{
		Position position;
		Sint32 score;
	};
	Sint32 currentSideX = SDL_static_cast(Sint32, current.x) - SDL_static_cast(Sint32, targetPosition.x);
	Sint32 currentSideY = SDL_static_cast(Sint32, current.y) - SDL_static_cast(Sint32, targetPosition.y);
	auto movementScore = [&](const Position& candidate) -> Sint32 {
		Sint32 candidateSideX = SDL_static_cast(Sint32, candidate.x) - SDL_static_cast(Sint32, targetPosition.x);
		Sint32 candidateSideY = SDL_static_cast(Sint32, candidate.y) - SDL_static_cast(Sint32, targetPosition.y);
		Sint32 score = UTIL_max<Sint32>(Position::getDistanceX(current, candidate), Position::getDistanceY(current, candidate));
		// Crossing either axis through the monster makes the character pass in
		// front of it. Strongly prefer destinations that remain on the same side.
		if(currentSideX != 0 && candidateSideX != 0 && ((currentSideX < 0) != (candidateSideX < 0)))
			score += 10000;
		if(currentSideY != 0 && candidateSideY != 0 && ((currentSideY < 0) != (candidateSideY < 0)))
			score += 10000;
		if(currentSideX * candidateSideX + currentSideY * candidateSideY < 0)
			score += 20000;
		if((currentSideX != 0 && candidateSideX == 0) || (currentSideY != 0 && candidateSideY == 0))
			score += 500;
		return score;
	};
	std::vector<MovementCandidate> candidates;
	for(size_t i = 0; i < 4; ++i)
	{
		Sint32 candidateX = SDL_static_cast(Sint32, targetPosition.x) + diagonalOffsets[i][0] * wantedDistance;
		Sint32 candidateY = SDL_static_cast(Sint32, targetPosition.y) + diagonalOffsets[i][1] * wantedDistance;
		if(candidateX < 0 || candidateX > 65535 || candidateY < 0 || candidateY > 65535)
			continue;
		Position candidate(SDL_static_cast(Uint16, candidateX), SDL_static_cast(Uint16, candidateY), targetPosition.z);
		Tile* tile = g_map.getTile(candidate);
		if(!tile || !tile->isWalkable() || tile->hasFloorChangeRisk() || tile->getTopCreature())
			continue;
		MovementCandidate movementCandidate;
		movementCandidate.position = candidate;
		movementCandidate.score = movementScore(candidate);
		candidates.push_back(movementCandidate);
	}
	std::sort(candidates.begin(), candidates.end(), [](const MovementCandidate& left, const MovementCandidate& right) {
		return left.score < right.score;
	});
	for(const MovementCandidate& candidate : candidates)
	{
		if(g_game.startAutoWalk(candidate.position, true, false))
		{
			m_targetMovementWalking = true;
			m_nextTargetMovement = now + 300;
			return;
		}
	}

	// Sem diagonal alcancavel, nao procure outro canto. Apenas preserve a
	// distancia escolhida em qualquer direcao ao redor do alvo.
	if(UTIL_max<Sint32>(deltaX, deltaY) == wantedDistance)
	{
		if(m_targetMovementWalking)
			g_game.stopAutoWalk();
		m_targetMovementWalking = false;
		m_nextTargetMovement = now + 150;
		return;
	}

	candidates.clear();
	for(Sint32 offsetX = -wantedDistance; offsetX <= wantedDistance; ++offsetX)
	{
		for(Sint32 offsetY = -wantedDistance; offsetY <= wantedDistance; ++offsetY)
		{
			if(UTIL_max<Sint32>(std::abs(offsetX), std::abs(offsetY)) != wantedDistance)
				continue;
			Sint32 candidateX = SDL_static_cast(Sint32, targetPosition.x) + offsetX;
			Sint32 candidateY = SDL_static_cast(Sint32, targetPosition.y) + offsetY;
			if(candidateX < 0 || candidateX > 65535 || candidateY < 0 || candidateY > 65535)
				continue;
			Position candidate(SDL_static_cast(Uint16, candidateX), SDL_static_cast(Uint16, candidateY), targetPosition.z);
			Tile* tile = g_map.getTile(candidate);
			if(!tile || !tile->isWalkable() || tile->hasFloorChangeRisk() || tile->getTopCreature())
				continue;
			MovementCandidate movementCandidate;
			movementCandidate.position = candidate;
			movementCandidate.score = movementScore(candidate);
			candidates.push_back(movementCandidate);
		}
	}
	std::sort(candidates.begin(), candidates.end(), [](const MovementCandidate& left, const MovementCandidate& right) {
		return left.score < right.score;
	});
	m_targetMovementWalking = false;
	for(const MovementCandidate& candidate : candidates)
	{
		if(g_game.startAutoWalk(candidate.position, true, false))
		{
			m_targetMovementWalking = true;
			break;
		}
	}
	m_nextTargetMovement = now + 300;
}

bool CaveBot::useCorpseGold(Container* source)
{
	if(!m_settings.lootGold || !source)
		return false;

	for(ItemUI* item : source->getItems())
	{
		if(!item || !isCurrencyCoin(item->getID()))
			continue;

		double capacity = g_game.getPlayerCapacity();
		bool dropOnGround = (m_settings.dropGoldNoCapacity && capacity < m_settings.dropGoldCapacity);
		if(dropOnGround)
		{
			Creature* local = g_map.getLocalCreature();
			if(!local)
				return false;
			g_game.sendMove(item->getCurrentPosition(), item->getID(), 0,
				local->getCurrentPosition(), item->getItemCount());
			return true;
		}
		if(capacity <= 0.01)
			return false;

		Position position = item->getCurrentPosition();
		g_game.sendUseItem(position, item->getID(), 0, source->getCid());
		return true;
	}
	return false;
}

bool CaveBot::lootOneItem(Container* source)
{
	if(!source)
		return false;

	Container* destination = g_game.findContainer(m_settings.lootDestinationContainer);
	double capacity = g_game.getPlayerCapacity();
	bool noCapacity = (capacity <= 0.01);
	bool dropOnGround = (m_settings.dropLootNoCapacity && capacity < m_settings.dropLootCapacity);
	if(noCapacity && !dropOnGround)
		return false;
	if(!dropOnGround && (!destination || destination->getItems().size() >= SDL_static_cast(size_t, destination->getCapacity())))
		return false;

	for(ItemUI* item : source->getItems())
	{
		if(!item || !idInList(item->getID(), m_settings.lootItemIds))
			continue;

		Position destinationPosition;
		if(dropOnGround)
		{
			Creature* local = g_map.getLocalCreature();
			if(!local)
				return false;
			destinationPosition = local->getCurrentPosition();
		}
		else
			destinationPosition = Position(0xFFFF, SDL_static_cast(Uint16, m_settings.lootDestinationContainer) | 0x40,
				SDL_static_cast(Uint8, destination->getItems().size()));
		g_game.sendMove(item->getCurrentPosition(), item->getID(), 0, destinationPosition, item->getItemCount());
		return true;
	}
	return false;
}

bool CaveBot::openCorpseBag(Container* source, Uint32 now)
{
	if(!source)
		return false;

	for(ItemUI* item : source->getItems())
	{
		if(!item || !item->getThingType() || !item->getThingType()->hasFlag(ThingAttribute_Container))
			continue;

		Uint8 containerId = g_game.findEmptyContainerId();
		if(g_game.findContainer(containerId))
			return false;

		Position position = item->getCurrentPosition();
		g_game.sendUseItem(position, item->getID(), 0, containerId);
		m_lootBagContainer = containerId;
		m_lootContainerRequest = now;
		return true;
	}
	return false;
}

bool CaveBot::hasVisibleLivingMonster() const
{
	Creature* local = g_map.getLocalCreature();
	if(!local)
		return false;

	Position localPosition = local->getCurrentPosition();
	for(auto& known : g_map.getKnownCreatures())
	{
		Creature* creature = known.second;
		if(creature && creature->isMonster() && creature->isVisible() && creature->getHealth() > 0 &&
			creature->getCurrentPosition().z == localPosition.z)
			return true;
	}
	return false;
}

bool CaveBot::shouldPauseForLoot() const
{
	if(!m_settings.lootEnabled)
		return false;
	if(m_lootCorpseContainer != 0xFF || m_lootBagContainer != 0xFF)
		return true;
	if(m_lootTargets.empty())
		return false;
	if(m_settings.lootAfterKill && hasVisibleLivingMonster())
		return false;
	if(m_settings.caveEnabled)
		return true;

	// With route walking disabled, a queued corpse must not pause combat or
	// movement while it is far away. It becomes actionable only when the player
	// manually reaches an adjacent square.
	Creature* local = g_map.getLocalCreature();
	if(!local)
		return false;
	Position center = local->getCurrentPosition();
	for(const LootTarget& target : m_lootTargets)
	{
		if(target.position.z == center.z && Position::getDistanceX(center, target.position) <= 1 &&
			Position::getDistanceY(center, target.position) <= 1)
			return true;
	}
	return false;
}

void CaveBot::finishLootTarget(Uint32 now)
{
	if(!m_lootTargets.empty())
		m_lootTargets.pop_front();
	m_lootCorpseContainer = 0xFF;
	m_lootBagContainer = 0xFF;
	m_lootContainerRequest = 0;
	m_lootOpenAttempts = 0;
	m_lootCorpseBagScanned = false;
	m_lastCorpseItemId = 0;
	m_lastCorpseStack = 0;
	m_lastWalkRequest = 0;
	m_nextLootAction = now + 250;
}

void CaveBot::updateLooter(Uint32 now)
{
	if(!m_settings.lootEnabled || now < m_nextLootAction)
		return;
	if(m_settings.lootAfterKill && hasVisibleLivingMonster())
		return;
	if(g_game.getAttackID() != 0)
	{
		Creature* activeTarget = g_map.getCreatureById(g_game.getAttackID());
		if(activeTarget && activeTarget->getHealth() > 0)
			return;
	}

	// Once a corpse is opened, only its reserved container and its inner bag are
	// inspected. Player backpacks are never scanned as loot sources.
	if(m_lootCorpseContainer != 0xFF)
	{
		Container* corpse = g_game.findContainer(m_lootCorpseContainer);
		if(!corpse)
		{
			if(now < m_lootContainerRequest + 1200)
				return;
			if(m_lootOpenAttempts < 3 && m_lastCorpseItemId != 0)
			{
				g_game.sendUseItem(m_lastCorpsePosition, m_lastCorpseItemId, m_lastCorpseStack, m_lootCorpseContainer);
				++m_lootOpenAttempts;
				m_lootContainerRequest = now;
				m_nextLootAction = now + 350;
				return;
			}

			finishLootTarget(now);
			return;
		}

		if(m_lootBagContainer != 0xFF)
		{
			Container* bag = g_game.findContainer(m_lootBagContainer);
			if(!bag)
			{
				if(now < m_lootContainerRequest + 2000)
					return;
				m_lootBagContainer = 0xFF;
			}
			else if(useCorpseGold(bag) || lootOneItem(bag))
			{
				m_nextLootAction = now + 180;
				return;
			}
			else
			{
				g_game.sendCloseContainer(m_lootBagContainer);
				m_lootBagContainer = 0xFF;
				m_nextLootAction = now + 180;
				return;
			}
		}

		if(useCorpseGold(corpse))
		{
			m_nextLootAction = now + 180;
			return;
		}

		if(!m_lootCorpseBagScanned)
		{
			m_lootCorpseBagScanned = true;
			if(openCorpseBag(corpse, now))
			{
				m_nextLootAction = now + 350;
				return;
			}
		}

		if(lootOneItem(corpse))
		{
			m_nextLootAction = now + 180;
			return;
		}

		g_game.sendCloseContainer(m_lootCorpseContainer);
		finishLootTarget(now);
		return;
	}

	Creature* local = g_map.getLocalCreature();
	if(!local || m_lootTargets.empty())
		return;

	Position center = local->getCurrentPosition();
	if(!m_settings.caveEnabled)
	{
		// Fora da rota automatica, nao caminhe ate um corpo. Use apenas um corpo
		// abatido pelo jogador que esteja ao alcance no momento.
		size_t nearbyTarget = m_lootTargets.size();
		for(size_t index = 0; index < m_lootTargets.size(); ++index)
		{
			const Position& position = m_lootTargets[index].position;
			if(position.z == center.z && Position::getDistanceX(center, position) <= 1 && Position::getDistanceY(center, position) <= 1)
			{
				nearbyTarget = index;
				break;
			}
		}
		if(nearbyTarget == m_lootTargets.size())
		{
			m_nextLootAction = now + 100;
			return;
		}
		if(nearbyTarget != 0)
			std::rotate(m_lootTargets.begin(), m_lootTargets.begin() + nearbyTarget, m_lootTargets.begin() + nearbyTarget + 1);
	}

	LootTarget& target = m_lootTargets.front();
	if(now >= target.queuedAt + 30000 || target.position.z != center.z)
	{
		finishLootTarget(now);
		return;
	}

	Sint32 distanceX = Position::getDistanceX(center, target.position);
	Sint32 distanceY = Position::getDistanceY(center, target.position);
	if(distanceX > 1 || distanceY > 1)
	{
		if(m_settings.caveEnabled && now >= m_lastWalkRequest + 350)
		{
			struct LootApproach
			{
				Position position;
				Sint32 distance;
			};
			std::vector<LootApproach> approaches;
			for(Sint32 offsetX = -1; offsetX <= 1; ++offsetX)
			{
				for(Sint32 offsetY = -1; offsetY <= 1; ++offsetY)
				{
					if(offsetX == 0 && offsetY == 0)
						continue;
					Sint32 approachX = SDL_static_cast(Sint32, target.position.x) + offsetX;
					Sint32 approachY = SDL_static_cast(Sint32, target.position.y) + offsetY;
					if(approachX < 0 || approachX > 65535 || approachY < 0 || approachY > 65535)
						continue;
					Position approachPosition(SDL_static_cast(Uint16, approachX), SDL_static_cast(Uint16, approachY), target.position.z);
					Tile* approachTile = g_map.getTile(approachPosition);
					if(!approachTile || !approachTile->isWalkable() || approachTile->hasFloorChangeRisk() || approachTile->getTopCreature())
						continue;
					LootApproach approach;
					approach.position = approachPosition;
					approach.distance = UTIL_max<Sint32>(Position::getDistanceX(center, approachPosition), Position::getDistanceY(center, approachPosition));
					approaches.push_back(approach);
				}
			}
			std::sort(approaches.begin(), approaches.end(), [](const LootApproach& left, const LootApproach& right) {
				return left.distance < right.distance;
			});
			for(const LootApproach& approach : approaches)
			{
				if(g_game.startAutoWalk(approach.position, true, false))
					break;
			}
			m_lastWalkRequest = now;
		}
		m_nextLootAction = now + 100;
		return;
	}

	if(m_settings.caveEnabled)
		g_game.stopAutoWalk();

	Tile* tile = g_map.getTile(target.position);
	Thing* thing = (tile ? tile->getTopContainerThing() : NULL);
	Item* item = (thing ? thing->getItem() : NULL);
	if(!item)
	{
		// O pacote de morte pode chegar um pouco antes da troca da criatura pelo corpo.
		if(now >= target.queuedAt + 5000)
			finishLootTarget(now);
		else
			m_nextLootAction = now + 100;
		return;
	}

	Uint8 containerId = g_game.findEmptyContainerId();
	if(g_game.findContainer(containerId))
	{
		m_nextLootAction = now + 500;
		return;
	}

	Uint8 stackpos = SDL_static_cast(Uint8, tile->getUseStackPos(thing));
	g_game.sendUseItem(target.position, item->getID(), stackpos, containerId);
	m_lastCorpsePosition = target.position;
	m_lastCorpseUse = now;
	m_lastCorpseItemId = item->getID();
	m_lastCorpseStack = stackpos;
	m_lootCorpseContainer = containerId;
	m_lootContainerRequest = now;
	m_lootOpenAttempts = 1;
	m_lootCorpseBagScanned = false;
	m_nextLootAction = now + 350;
}

bool CaveBot::executeWaypoint(const CaveBotWaypoint& waypoint)
{
	if(waypoint.action == CAVEBOT_WALK || waypoint.action == CAVEBOT_STAND)
		return true;
	if(waypoint.action == CAVEBOT_EXANI)
	{
		g_game.sendSay(MessageSay, 0, std::string(), "exani tera");
		return true;
	}
	if(waypoint.action == CAVEBOT_SAY)
	{
		if(!waypoint.text.empty())
			g_game.sendSay(MessageSay, 0, std::string(), waypoint.text);
		return true;
	}

	Tile* tile = g_map.getTile(waypoint.position);
	Thing* targetThing = (tile ? tile->getTopUseThing() : NULL);
	Item* targetItem = (targetThing ? targetThing->getItem() : NULL);
	if(!tile || !targetItem)
		return false;
	Uint8 targetStack = SDL_static_cast(Uint8, tile->getUseStackPos(targetThing));
	if(waypoint.itemId == 0)
	{
		// Use the current tile item id when a door/lever has transformed since it
		// was recorded. targetItemId remains the recorded identity shown to users.
		g_game.sendUseItem(waypoint.position, targetItem->getID(), targetStack, g_game.findEmptyContainerId());
		return true;
	}

	Position sourcePosition;
	Uint8 sourceStack = waypoint.itemSubtype;
	if(!g_assistant.findItem(waypoint.itemId, waypoint.itemSubtype, sourcePosition, sourceStack))
		return false;
	g_game.sendUseItemEx(sourcePosition, waypoint.itemId, sourceStack, waypoint.position, targetItem->getID(), targetStack);
	return true;
}

void CaveBot::updateWalker(Uint32 now)
{
	if(!m_settings.caveEnabled || m_waypoints.empty() || now < m_nextWalkAction)
	{
		m_walkPositionValid = false;
		return;
	}
	Creature* activeTarget = g_map.getCreatureById(g_game.getAttackID());
	bool fighting = (activeTarget && activeTarget->getHealth() > 0);
	bool pausedForCombat = (fighting && (m_settings.pauseWhileTarget || m_settings.targetMovement != CAVEBOT_TARGET_STAND));
	if((m_settings.pauseOnPlayer && m_playerDetected) ||
		pausedForCombat || shouldPauseForLoot())
	{
		if(pausedForCombat)
			m_reselectWaypointAfterCombat = true;
		m_walkPositionValid = false;
		return;
	}
	Creature* local = g_map.getLocalCreature();
	if(!local)
		return;
	Position current = local->getCurrentPosition();
	if(m_reselectWaypointAfterCombat)
	{
		selectNearestWaypoint(current);
		m_reselectWaypointAfterCombat = false;
	}
	if(m_currentWaypoint >= m_waypoints.size())
		m_currentWaypoint = 0;

	CaveBotWaypoint& waypoint = m_waypoints[m_currentWaypoint];

	// A recorded use followed by another floor means that the used square is
	// the opened hole/stair. Wait for the server to transform it and then step
	// onto that exact square before trying the waypoint on the other floor.
	if(m_floorTransitionPending)
	{
		if(current.z != m_floorTransitionSourceZ)
		{
			m_floorTransitionPending = false;
			m_lastWalkRequest = 0;
			m_walkPositionValid = false;
		}
		else
		{
			if(now >= m_floorTransitionSince + 5000)
			{
				m_floorTransitionPending = false;
				g_game.processTextMessage(MessageFailure, "CaveBot: nao conseguiu entrar no buraco aberto.");
			}
			else if(now >= m_nextWalkAction && now >= m_lastWalkRequest + 250)
			{
				g_game.startAutoWalk(m_floorTransitionPosition, false, false);
				m_lastWalkRequest = now;
			}
			return;
		}
	}

	size_t nextWaypoint = m_currentWaypoint + 1;
	if(nextWaypoint >= m_waypoints.size() && m_settings.loop)
		nextWaypoint = 0;
	bool hasNextWaypoint = (nextWaypoint < m_waypoints.size() && nextWaypoint != m_currentWaypoint);
	bool changesFloorNext = (hasNextWaypoint && m_waypoints[nextWaypoint].position.z != waypoint.position.z &&
		Position::getDistanceX(waypoint.position, m_waypoints[nextWaypoint].position) <= 1 &&
		Position::getDistanceY(waypoint.position, m_waypoints[nextWaypoint].position) <= 1);

	Uint8 reachDistance = (waypoint.action == CAVEBOT_EXANI ? 0 : m_settings.reachDistance);
	if(waypoint.action == CAVEBOT_USE)
		reachDistance = UTIL_max<Uint8>(1, reachDistance);
	bool reached = (current.z == waypoint.position.z &&
		Position::getDistanceX(current, waypoint.position) <= reachDistance &&
		Position::getDistanceY(current, waypoint.position) <= reachDistance);
	if(!reached)
	{
		if(!m_walkPositionValid || current != m_lastWalkPosition)
		{
			m_lastWalkPosition = current;
			m_walkNoProgressSince = now;
			m_walkPositionValid = true;
		}
		else if(now >= m_walkNoProgressSince + 5000)
		{
			g_game.stopAutoWalk();
			g_game.processTextMessage(MessageFailure, "CaveBot: caminho bloqueado, pulando para o proximo waypoint.");
			m_lastWalkRequest = 0;
			m_walkNoProgressSince = now;
			m_walkPositionValid = false;
			m_nextWalkAction = now + 200;
			++m_currentWaypoint;
			if(m_currentWaypoint >= m_waypoints.size())
			{
				if(m_settings.loop)
					m_currentWaypoint = 0;
				else
				{
					m_settings.caveEnabled = false;
					m_currentWaypoint = 0;
					g_game.processTextMessage(MessageLogin, "CaveBot: rota concluida.");
					saveConfig();
				}
			}
			return;
		}
		if(now >= m_lastWalkRequest + m_settings.retryDelay)
		{
			// A risky tile is allowed only when the very next recorded waypoint is
			// on the neighboring floor. This preserves protection against accidental
			// holes while allowing intentional stairs/holes in recorded routes.
			g_game.startAutoWalk(waypoint.position, !changesFloorNext, false);
			m_lastWalkRequest = now;
		}
		return;
	}

	if(!executeWaypoint(waypoint))
	{
		if(now >= m_lastWalkRequest + m_settings.retryDelay)
		{
			g_game.processTextMessage(MessageFailure, "CaveBot: nao foi possivel executar o waypoint.");
			m_lastWalkRequest = now;
		}
		return;
	}
	if(waypoint.action == CAVEBOT_USE && changesFloorNext && current.z == waypoint.position.z)
	{
		m_floorTransitionPending = true;
		m_floorTransitionPosition = waypoint.position;
		m_floorTransitionSourceZ = current.z;
		m_floorTransitionSince = now;
	}
	m_nextWalkAction = now + waypoint.delay;
	m_lastWalkRequest = 0;
	m_walkPositionValid = false;
	++m_currentWaypoint;
	if(m_currentWaypoint >= m_waypoints.size())
	{
		if(m_settings.loop)
			m_currentWaypoint = 0;
		else
		{
			m_settings.caveEnabled = false;
			m_currentWaypoint = 0;
			g_game.processTextMessage(MessageLogin, "CaveBot: rota concluida.");
			saveConfig();
		}
	}
}

void CaveBot::selectNearestWaypoint(const Position& current)
{
	if(m_waypoints.empty())
		return;

	size_t routeAnchor = m_currentWaypoint < m_waypoints.size() ? m_currentWaypoint : 0;
	size_t bestIndex = routeAnchor;
	Uint32 bestDistance = SDL_MAX_UINT32;
	size_t bestForwardDistance = m_waypoints.size();
	for(size_t index = 0; index < m_waypoints.size(); ++index)
	{
		const Position& position = m_waypoints[index].position;
		if(position.z != current.z)
			continue;
		Uint32 distance = UTIL_max<Uint32>(Position::getDistanceX(current, position), Position::getDistanceY(current, position));
		size_t forwardDistance = (index + m_waypoints.size() - routeAnchor) % m_waypoints.size();
		if(distance < bestDistance || (distance == bestDistance && forwardDistance < bestForwardDistance))
		{
			bestDistance = distance;
			bestForwardDistance = forwardDistance;
			bestIndex = index;
		}
	}

	m_currentWaypoint = bestIndex;
	m_lastWalkRequest = 0;
	m_walkNoProgressSince = 0;
	m_walkPositionValid = false;
}

void CaveBot::updateBurstArrows(Uint32 now)
{
	static const Uint16 BURST_ARROW_ID = 3449;
	if(!m_settings.burstArrowEnabled || g_game.getInventoryItem(SLOT_AMMO) || !SDL_TICKS_PASSED(now, m_nextBurstArrowRetry))
		return;

	for(Uint8 containerId = 0; containerId < GAME_MAX_CONTAINERS; ++containerId)
	{
		Container* container = g_game.findContainer(containerId);
		if(!container)
			continue;
		for(ItemUI* item : container->getItems())
		{
			if(!item || item->getID() != BURST_ARROW_ID)
				continue;

			Position destination(0xFFFF, SDL_static_cast(Uint16, SLOT_AMMO) + 1, 0);
			Uint16 count = UTIL_min<Uint16>(100, item->getItemCount());
			g_game.sendMove(item->getCurrentPosition(), BURST_ARROW_ID, 0, destination, count);
			m_nextBurstArrowRetry = now + 150;
			return;
		}
	}
	m_nextBurstArrowRetry = now + 1000;
}

void CaveBot::tick()
{
	ensureCharacterProfile();
	if(!m_configLoaded || !g_engine.isIngame() || g_game.getPlayerID() == 0)
	{
		m_playerDetected = false;
		return;
	}
	Uint32 now = SDL_GetTicks();
	Creature* attacked = g_map.getCreatureById(g_game.getAttackID());
	if(attacked && attacked->isMonster() && attacked->getHealth() > 0)
	{
		m_recentAttackTarget = attacked->getId();
		m_recentAttackAt = now;
	}
	updateRecorder(now);
	if(!m_settings.enabled)
	{
		m_playerDetected = false;
		return;
	}
	updatePlayerAlert(now);
	updateTargetBot(now);
	updateTargetMovement(now);
	updateBurstArrows(now);
	updateLooter(now);
	updateWalker(now);
}

void CaveBot::ensureCharacterProfile()
{
	if(!g_engine.isIngame() || g_game.getPlayerID() == 0)
		return;
	Creature* local = g_map.getLocalCreature();
	if(!local || local->getName().empty() || local->getName() == m_profileCharacter)
		return;
	loadCharacterConfig(local->getName());
}
