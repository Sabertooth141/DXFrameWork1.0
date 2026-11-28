#pragma once
#include <memory>

#include "MonoBehavior.h"
#include "PlayerInput.h"
#include "PlayerState.h"

class ActionController;
enum class ActionButton : uint8_t;
struct PlayerIntent;
struct ActionDef;
class HitVolumeComponent;
class SkeletalAnimatorComponent;
class HealthComponent;
class LocomotionComponent;
class Camera3D;

class PlayerController : public MonoBehavior
{
public:
	void Awake() override;
	void Start() override;
	void Update(float deltaTime) override;

	// public for manipulation from state machine
	void StartFromInput(ActionButton input, const DirectX::XMFLOAT3& dir, bool isLateChain);
	void UpdateLocomotion(const PlayerIntent& intent);
	void UpdateAction(const PlayerIntent& intent, float deltaTime);

	void StartKnockdown();
	void UpdateKnockdown(const PlayerIntent& intent, float deltaTime);

	bool RequestState(PlayerStateID nextID, bool isRunNow = false);
	void ForceState(PlayerStateID nextID, bool isRunNow = false);
private:
	void StartAction(const char* actionName, const DirectX::XMFLOAT3& dir, float minFadeSec = 0.f);	

	void RegisterStates();
	PlayerState* GetState(PlayerStateID id) const;
	void ApplyPendingTransition(const PlayerIntent& intent, float deltaTime);
	void RunTransitionCleanup(PlayerStateID fromID, PlayerStateID toID);

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

	static constexpr PlayerStateID noState = PlayerStateID::COUNT;
	std::array<std::unique_ptr<PlayerState>, static_cast<size_t>(PlayerStateID::COUNT)> states;
	PlayerStateID currStateID = PlayerStateID::LOCOMOTION;
	PlayerStateID pendingStateID = noState;
	bool isPendingRunNow = false;

	ActionController* actionController = nullptr;

	bool isKnockdownPending = false;
	DirectX::XMFLOAT3 hitDir{};
	DirectX::XMFLOAT3 knockdownBackDir{};
	float knockdownTimer = 0.f;
	float knockdownGroundY = 0.f;

	bool isGettingUp = false;
	bool isGetUpCancelOpen = false;
};
