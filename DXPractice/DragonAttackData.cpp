#include "DragonAttackData.h"

const std::vector<AttackOption>& DragonAttackData()
{
	static const std::vector<AttackOption> attacks = []
	{
		std::vector<AttackOption> attkList;

		// behind: the answer to a player hugging the tail
		// back whole sector
		attkList.push_back({
			.name = "tailSwipe", .action = "Attk2", .priority = 5, .cooldownSec = 8.f,
			.minRange = 0.f, .maxRange = 8.f, .sectors = sectorBack,
			.trackSpeedDeg = 0.f, .recoverSec = 0.8f, .whiffRecoverSec = 1.6f, .hitboxGroup = "tail"
			});

		// close range
		// front whole sector
		attkList.push_back({
			.name = "wingSlam", .action = "Attk5", .priority = 4, .cooldownSec = 8.f,
			.minRange = 2.f, .maxRange = 8.f, .sectors = sectorFront,
			.trackSpeedDeg = 60.f, .recoverSec = 0.8f, .whiffRecoverSec = 1.f, .hitboxGroup = "wingL"
			});

		// close
		// front && left whole sector
		attkList.push_back({
			.name = "headBlade", .action = "Attk3", .priority = 3, .cooldownSec = 6.f,
			.minRange = 0.f, .maxRange = 7.f, .sectors = sectorFront | sectorLeft,
			.trackSpeedDeg = 90.f, .recoverSec = 0.6f, .whiffRecoverSec = 1.f, .hitboxGroup = "headBlade"
			});

		// close
		// front sector
		attkList.push_back({
			.name = "breathFan", .action = "Attk4", .priority = 1, .cooldownSec = 12.f,
			.minRange = 4.f, .maxRange = 7.f, .sectors = sectorFront,
			.trackSpeedDeg = 30.f, .recoverSec = 1.5f, .whiffRecoverSec = 2.2f, .hitboxGroup = "breath"
			});

		// mid range
		// dead ahead
		attkList.push_back({
			.name = "breathLine", .action = "Attk1", .priority = 2, .cooldownSec = 12.f,
			.minRange = 5.5f, .maxRange = 15.f, .sectors = sectorFront,
			.trackSpeedDeg = 60.f, .recoverSec = 0.2f, .whiffRecoverSec = 0.6f, .hitboxGroup = "breath"
			});

		return attkList;
	}();

	return attacks;
}
