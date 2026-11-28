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
class TransformComponent;
class Camera3D;

class PlayerController : public MonoBehavior
{
public:
	void Awake() override;
	void Start() override;
	void Update(float deltaTime) override;

	// public for manipulation from state machine
	void StartFromInput(ActionButton input, const DirectX::XMFLOAT3& dir, bool isLateChain);

	bool RequestState(PlayerStateID nextID, bool isRunNow = false);
	void ForceState(PlayerStateID nextID, bool isRunNow = false);

	TransformComponent& GetTransform() const;

	SkeletalAnimatorComponent& GetAnimator() const
	{
		return *animator;
	}

	LocomotionComponent& GetLocomotion() const
	{
		return *locomotion;
	}

	ActionController& GetActionController() const
	{
		return *actionController;
	}

	HitVolumeComponent* GetHitVolume() const
	{
		return hitVolume;
	}

	const DirectX::XMFLOAT3& GetHitDir() const
	{
		return hitDir;
	}

	// set by StartAction, played by PlayerActionState::Enter
	const ActionDef* GetRequestedAction() const
	{
		return requestedAction;
	}

	float GetRequestedMinFade() const
	{
		return requestedMinFadeSec;
	}

private:
	void StartAction(const char* actionName, const DirectX::XMFLOAT3& dir, float minFadeSec = 0.f);

	void RegisterStates();
	PlayerState* GetState(PlayerStateID id) const;
	void ApplyPendingTransition(const PlayerIntent& intent, float deltaTime);

	void ApplyRootMotion();

#ifndef NDEBUG
	void ValidateActionData() const;
#endif

public:
	float lateChainFadeSec = 0.1f;

	// TODO: TEMP
	const char* attackAction = "Combo1Attk1";
	const char* attack2Action = "Combo3Attk1";
	const char* rollAction = "RollF";

private:
	const Camera3D* cam = nullptr;
	HealthComponent* health = nullptr;
	LocomotionComponent* locomotion = nullptr;
	SkeletalAnimatorComponent* animator = nullptr;
	HitVolumeComponent* hitVolume = nullptr;
	PlayerInputReader inputReader;

	static constexpr PlayerStateID noState = PlayerStateID::COUNT;
	std::array<std::unique_ptr<PlayerState>, static_cast<size_t>(PlayerStateID::COUNT)> states;
	PlayerStateID currStateID = PlayerStateID::LOCOMOTION;
	PlayerStateID pendingStateID = noState;
	bool isPendingRunNow = false;

	ActionController* actionController = nullptr;
	const ActionDef* requestedAction = nullptr;
	float requestedMinFadeSec = 0.f;

	bool isKnockdownPending = false;
	DirectX::XMFLOAT3 hitDir{};
};
