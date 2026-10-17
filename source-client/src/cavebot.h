/*
  Native CaveBot for the Old FasterOT client.
  It only uses state and protocol operations already exposed by the client.
*/

#ifndef __FILE_CAVEBOT_h_
#define __FILE_CAVEBOT_h_

#include "defines.h"
#include "position.h"

#include <deque>

class Container;

enum CaveBotWaypointAction : Uint8
{
	CAVEBOT_WALK = 0,
	CAVEBOT_STAND,
	CAVEBOT_USE,
	CAVEBOT_SAY,
	CAVEBOT_EXANI
};

enum CaveBotTargetMovement : Uint8
{
	CAVEBOT_TARGET_STAND = 0,
	CAVEBOT_TARGET_FOLLOW,
	CAVEBOT_TARGET_DIAGONAL,
	CAVEBOT_TARGET_STAND_DIAGONAL,
	CAVEBOT_TARGET_DIAGONAL_DISTANCE_3,
	CAVEBOT_TARGET_DIAGONAL_DISTANCE_4,
	CAVEBOT_TARGET_DIAGONAL_DISTANCE_5
};

struct CaveBotWaypoint
{
	Position position;
	CaveBotWaypointAction action = CAVEBOT_WALK;
	// itemId is the source item for "use with". targetItemId identifies the
	// door/object on the recorded map square (and is also kept for direct use).
	Uint16 itemId = 0;
	Uint16 targetItemId = 0;
	Uint8 itemSubtype = 0;
	Uint32 delay = 300;
	std::string text;
};

struct CaveBotSettings
{
	bool enabled = false;
	bool caveEnabled = true;
	bool recordEnabled = false;
	bool loop = true;
	bool pauseWhileTarget = true;
	Uint8 reachDistance = 0;
	Uint32 retryDelay = 1400;

	bool targetEnabled = false;
	std::string targetNames = "*";
	Uint16 attackRuneId = 0;
	Uint8 attackRuneSubtype = 0;
	Uint32 attackDelay = 900;
	CaveBotTargetMovement targetMovement = CAVEBOT_TARGET_STAND;

	bool lootEnabled = false;
	bool lootGold = false;
	bool lootAfterKill = false;
	bool dropLootNoCapacity = false;
	std::string lootItemIds;
	Uint8 lootDestinationContainer = 0;

	bool playerAlertEnabled = true;
	bool pauseOnPlayer = true;
	std::string ignoredPlayers;
	bool burstArrowEnabled = false;
};

class CaveBot
{
	public:
		CaveBot() = default;

		void loadConfig();
		void saveConfig();
		void tick();
		void resetRuntime();
		void notifyCreatureDeath(Uint32 creatureId, const Position& position);
		void notifyManualUse(const Position& targetPosition, Uint16 targetItemId);
		void notifyManualUseWith(const Position& sourcePosition, Uint16 sourceItemId,
			const Position& targetPosition, Uint16 targetItemId);

		const CaveBotSettings& getSettings() const {return m_settings;}
		const std::vector<CaveBotWaypoint>& getWaypoints() const {return m_waypoints;}
		void applySettings(const CaveBotSettings& settings);
		void setEnabled(bool enabled);
		void setCurrentWaypoint(size_t index);
		void notifyUserInput();
		bool isEnabled() const {return m_settings.enabled;}
		bool isPlayerDetected() const {return m_playerDetected;}
		size_t getCurrentWaypoint() const {return m_currentWaypoint;}

		void addWaypoint(const CaveBotWaypoint& waypoint);
		void removeWaypoint(size_t index);
		void moveWaypoint(size_t index, Sint32 direction);
		void shiftWaypoint(size_t index, Sint32 offsetX, Sint32 offsetY);
		void clearWaypoints();

		std::string waypointDescription(size_t index) const;

	private:
		bool matchesNameList(const std::string& name, const std::string& list, bool wildcard) const;
		void updatePlayerAlert(Uint32 now);
		void updateRecorder(Uint32 now);
		void updateTargetBot(Uint32 now);
		void updateTargetMovement(Uint32 now);
		void restoreTargetMovement();
		void updateLooter(Uint32 now);
		void updateBurstArrows(Uint32 now);
		void updateWalker(Uint32 now);
		bool hasVisibleLivingMonster() const;
		bool shouldPauseForLoot() const;
		void finishLootTarget(Uint32 now);
		bool executeWaypoint(const CaveBotWaypoint& waypoint);
		bool lootOneItem(Container* source);
		bool useCorpseGold(Container* source);
		bool openCorpseBag(Container* source, Uint32 now);

		CaveBotSettings m_settings;
		std::vector<CaveBotWaypoint> m_waypoints;
		size_t m_currentWaypoint = 0;
		Uint32 m_nextWalkAction = 0;
		Uint32 m_lastWalkRequest = 0;
		Uint32 m_walkNoProgressSince = 0;
		Position m_lastWalkPosition;
		bool m_walkPositionValid = false;
		bool m_floorTransitionPending = false;
		Position m_floorTransitionPosition;
		Uint8 m_floorTransitionSourceZ = 0;
		Uint32 m_floorTransitionSince = 0;
		Uint32 m_nextTargetAction = 0;
		Uint32 m_nextTargetMovement = 0;
		Uint32 m_nextLootAction = 0;
		Uint32 m_nextPlayerAlert = 0;
		Uint32 m_nextPlayerAlertBeep = 0;
		Uint32 m_playerAlertSoundUntil = 0;
		Uint32 m_playerAlertMutedUntil = 0;
		bool m_playerAlertWasDetected = false;
		Uint32 m_nextBurstArrowRetry = 0;
		Uint32 m_lastCorpseUse = 0;
		Position m_lastCorpsePosition;
		Uint16 m_lastCorpseItemId = 0;
		Uint8 m_lastCorpseStack = 0;
		Uint8 m_lootCorpseContainer = 0xFF;
		Uint8 m_lootBagContainer = 0xFF;
		Uint32 m_lootContainerRequest = 0;
		bool m_lootCorpseBagScanned = false;
		bool m_targetMovementWalking = false;
		bool m_targetChaseOverride = false;
		Uint8 m_previousChaseMode = CHASEMODE_STAND;
		struct LootTarget
		{
			Uint32 creatureId = 0;
			Position position;
			Uint32 queuedAt = 0;
		};
		std::deque<LootTarget> m_lootTargets;
		bool m_playerDetected = false;
		bool m_recordPositionValid = false;
		bool m_recordDirty = false;
		Position m_lastRecordPosition;
		Uint32 m_nextRecordSave = 0;
		bool m_configLoaded = false;
};

extern CaveBot g_caveBot;

#endif /* __FILE_CAVEBOT_h_ */
