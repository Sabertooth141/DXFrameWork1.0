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
	HIT
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

private:
	const Camera3D* cam = nullptr;
	HealthComponent* health = nullptr;
	LocomotionComponent* locomotion = nullptr;
	SkeletalAnimatorComponent* animator = nullptr;
	HitVolumeComponent* hitVolume = nullptr;
	BufferedInput bufferedInput = BufferedInput::NONE;
	bool isWindowOpen = false;

	PlayerState state = PlayerState::LOCOMOTION;
	const ActionDef* currAction = nullptr; // null while in locomotion
	bool wasLeftDown = false;

	float bufferAgeSec = 0.f;
};