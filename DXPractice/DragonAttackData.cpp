#include "DragonAttackData.h"

const std::vector<AttackOption>& DragonAttackData()
{
	static const std::vector<AttackOption> attacks = []
	{
		std::vector<AttackOption> attkList;

		// behind: the answer to a player hugging the tail
		attkList.push_back({
			.name = "tailSwipe", .clip = "Attk2", .priority = 5, .cooldownSec = 7.f,
			.minRange = 3.f, .maxRange = 9.f, .sectors = sectorBack,
			.trackSpeedDeg = 0.f, .recoverSec = 0.8f, .whiffRecoverSec = 1.6f, .hitboxGroup = "tail"
			});

		// close, front or its left (the slam uses the left wings)
		attkList.push_back({
			.name = "wingSlam", .clip = "Attk5", .priority = 4, .cooldownSec = 8.f,
			.minRange = 2.f, .maxRange = 8.f, .sectors = sectorFront,
			.trackSpeedDeg = 60.f, .recoverSec = 0.8f, .whiffRecoverSec = 1.5f, .hitboxGroup = "wingL"
			});

		attkList.push_back({
			.name = "headBlade", .clip = "Attk3", .priority = 3, .cooldownSec = 6.f,
			.minRange = 0.f, .maxRange = 7.f, .sectors = sectorFront | sectorLeft,
			.trackSpeedDeg = 90.f, .recoverSec = 1.f, .whiffRecoverSec = 1.8f, .hitboxGroup = "headBlade"
			});

		// mid range
		attkList.push_back({
			.name = "breathLine", .clip = "Attk1", .priority = 2, .cooldownSec = 10.f,
			.minRange = 6.f, .maxRange = 15.f, .sectors = sectorFront,
			.trackSpeedDeg = 60.f, .recoverSec = 1.2f, .whiffRecoverSec = 2.f, .hitboxGroup = "breath"
			});

		attkList.push_back({
			.name = "breathFan", .clip = "Attk4", .priority = 1, .cooldownSec = 12.f,
			.minRange = 4.f, .maxRange = 12.f, .sectors = sectorFront,
			.trackSpeedDeg = 30.f, .recoverSec = 1.5f, .whiffRecoverSec = 2.2f, .hitboxGroup = "breath"
			});

		return attkList;
	}();

	return attacks;
}
