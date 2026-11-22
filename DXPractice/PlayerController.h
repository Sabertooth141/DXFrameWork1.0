#pragma once
#include "MonoBehavior.h"

struct ActionDef;
class HitVolumeComponent;
class SkeletalAnimatorComponent;
class HealthComponent;
class LocomotionComponent;
class Camera3D;

// TODO: temp
enum class PlayerState
{
	LOCOMOTION,
	ATTACK,
	ROLL,
	KNOCKDOWN
};

// input buffer temp
enum class BufferedInput
{
	NONE,
	ATTACK,
	ATTACK2,
	ROLL,
};


class PlayerController : public MonoBehavior
{
public:
	void Awake() override;
	void Start() override;
	void Update(float deltaTime) override;

private:
	DirectX::XMFLOAT3 ReadMoveInput() const;
	bool ReadAttackTriggered();

	void UpdateLocomotion(const DirectX::XMFLOAT3& dir);

	void UpdateAction(const DirectX::XMFLOAT3& dir, BufferedInput pressedInput, float deltaTime);
	void UpdateInvuln();
	void StartFromInput(BufferedInput input, const DirectX::XMFLOAT3& dir, bool isLateChain);
	void StartAction(const char* actionName, const DirectX::XMFLOAT3& dir, PlayerState nextState, float minFadeSec = 0.f);

	void StartKnockdown();
	void UpdateKnockdown(const DirectX::XMFLOAT3& dir, BufferedInput pressedInput, float deltaTime);

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
	float bufferSec = 0.4f;

	// TODO: TEMP
	const char* idleClip = "IdleCombat";
	const char* runClip = "RunLoop";
	const char* sprintClip = "RunFastLoop";
	const char* attackAction = "Combo1Attk1";
	const char* attack2Action = "Combo3Attk1";
	const char* rollAction = "RollF";
	const char* knockdownClip = "Death_RM";
	const char* getUpClip = "GetUpB";

	float knockdownStartSec = 1.f;      // skip Death_RM's stumble, start at the fall
	float knockdownFadeSec = 0.1f;
	float knockdownAirSec = 0.45f;      // Death_RM's fall takes about this long
	float knockdownAirHeight = 0.4f;
	float knockdownPushDistance = 1.7f; // added to the clip's own ~0.8 m

private:
	const Camera3D* cam = nullptr;
	HealthComponent* health = nullptr;
	LocomotionComponent* locomotion = nullptr;
	SkeletalAnimatorComponent* animator = nullptr;
	HitVolumeComponent* hitVolume = nullptr;
	BufferedInput bufferedInput = BufferedInput::NONE;

	bool isComboOpen = false;
	bool isCancelOpen = false;

	PlayerState state = PlayerState::LOCOMOTION;
	const ActionDef* currAction = nullptr; // null while in locomotion
	bool wasLeftDown = false;

	bool isKnockdownPending = false;
	DirectX::XMFLOAT3 hitDir{};
	DirectX::XMFLOAT3 knockdownBackDir{};
	float knockdownTimer = 0.f;
	float knockdownGroundY = 0.f;
	bool isGettingUp = false;

	float bufferAgeSec = 0.f;
};