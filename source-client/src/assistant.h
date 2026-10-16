/*
  Native assistant additions for The Forgotten Client.
  This module uses the client's public game state and protocol methods only.
*/

#ifndef __FILE_ASSISTANT_h_
#define __FILE_ASSISTANT_h_

#include "defines.h"

struct Position;

struct AssistantSettings
{
	bool enabled = false;
	bool fullLight = false;
	bool healingEnabled = false;
	bool healingUseRune = false;
	Uint8 healingPercent = 50;
	std::string healingSpell = "exura gran";
	Uint16 healingRuneId = 0;
	Uint8 healingRuneSubtype = 0;
	bool hasteEnabled = false;
	std::string hasteSpell = "utani hur";
	bool manaEnabled = false;
	Uint8 manaPercent = 40;
	Uint16 manaFluidId = 0;
	Uint8 manaFluidSubtype = 7;
	bool energyRingEnabled = false;
	Uint8 energyRingEquipPercent = 40;
	Uint8 energyRingUnequipPercent = 70;
	Uint16 energyRingId = 0;
	Uint8 energyRingSubtype = 0;
	bool amuletEnabled = false;
	Uint8 amuletSwitchPercent = 40;
	Uint16 lowHealthAmuletId = 0;
	Uint8 lowHealthAmuletSubtype = 0;
	Uint16 highHealthAmuletId = 0;
	Uint8 highHealthAmuletSubtype = 0;
};

class Assistant
{
	public:
		Assistant() = default;

		void loadConfig();
		void saveConfig();
		void tick();
		void resetRuntime();
		void prepareShutdown();
		bool findItem(Uint16 itemId, Uint8 subtype, Position& position, Uint8& stackpos);

		const AssistantSettings& getSettings() const {return m_settings;}
		void applySettings(const AssistantSettings& settings);

	private:
		void applyFullLight(bool enabled);
		void castSpell(const std::string& spell);
		bool useOnSelf(Uint16 itemId, Uint8 subtype);
		bool isEquipped(Uint8 slot, Uint16 itemId);
		bool equipItem(Uint8 slot, Uint16 itemId, Uint8 subtype);
		bool unequipItem(Uint8 slot, Uint16 itemId);
		bool findFreeContainerPosition(Position& position);

		AssistantSettings m_settings;
		Uint32 m_nextAction = 0;
		Uint32 m_nextHaste = 0;
		Uint32 m_nextEnergyRingRetry = 0;
		Uint32 m_nextAmuletRetry = 0;
		bool m_energyRingActive = false;
		Uint8 m_previousLightMode = 1;
		bool m_configLoaded = false;
};

extern Assistant g_assistant;

#endif /* __FILE_ASSISTANT_h_ */
