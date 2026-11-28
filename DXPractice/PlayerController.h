#pragma once
#include "MonoBehavior.h"
#include "PlayerInput.h"

class ActionController;
enum class ActionButton : uint8_t;
struct PlayerIntent;
struct ActionDef;
class HitVolumeComponent;
class SkeletalAnimatorComponent;
class HealthComponent;
class LocomotionComponent;
class Camera3D;

enum class PlayerState
{
	LOCOMOTION,
	ATTACK,
	ROLL,
	KNOCKDOWN
};

class PlayerController : public MonoBehavior
{
public:
	void Awake() override;
	void Start() override;
	void Update(float deltaTime) override;

private:
	void UpdateLocomotion(const PlayerIntent& intent);

	void UpdateAction(const PlayerIntent& intent, float deltaTime);
	void StartFromInput(ActionButton input, const DirectX::XMFLOAT3& dir, bool isLateChain);
	void StartAction(const char* actionName, const DirectX::XMFLOAT3& dir, PlayerState nextState, float minFadeSec = 0.f);

	void StartKnockdown();
	void UpdateKnockdown(const PlayerIntent& intent, float deltaTime);

	void ChangeState(PlayerState nextState);
	void PlayLocomotionClip(const char* clipName);
	void ApplyRootMotion();

public:
	// intent amounts, applied through LocomotionComponent
	float runSpeed = 3.7f;
	float sprintSpeed = 6.2f;
	float turnSpeed = 12.f;
	float locomotionFadeSec = 0.15f;
	float lateChainFadeSec = 0.1f;

	// TODO: TEMP
	const char* idleClip = "IdleCombat";
	const char* runClip = "RunLoop";
	const char* sprintClip = "RunFastLoop";
	const char* attackAction = "Combo1Attk1";
	const char* attack2Action = "Combo3Attk1";
	const char* rollAction = "RollF";
	const char* knockdownClip = "Death_RM";
	const char* getUpClip = "GetUpB";

	float knockdownStartSec = 1.f; // skip Death_RM's stumble, start at the fall
	float knockdownFadeSec = 0.1f;
	float knockdownAirSec = 0.45f; // Death_RM's fall takes about this long
	float knockdownAirHeight = 0.4f;
	float knockdownPushDistance = 1.7f; // added to the clip's own ~0.8 m

private:
	const Camera3D* cam = nullptr;
	HealthComponent* health = nullptr;
	LocomotionComponent* locomotion = nullptr;
	SkeletalAnimatorComponent* animator = nullptr;
	HitVolumeComponent* hitVolume = nullptr;
	PlayerInputReader inputReader;
	InputBuffer inputBuffer;

	PlayerState state = PlayerState::LOCOMOTION;
	ActionController* actionController = nullptr;

	bool isKnockdownPending = false;
	DirectX::XMFLOAT3 hitDir{};
	DirectX::XMFLOAT3 knockdownBackDir{};
	float knockdownTimer = 0.f;
	float knockdownGroundY = 0.f;

	bool isGettingUp = false;
	bool isGetUpCancelOpen = false;
};
