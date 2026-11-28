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

// controller passes requests directly to action layer
struct ActionRequest
{
	ActionButton button = ActionButton::NONE;
	DirectX::XMFLOAT3 dir{};
	bool isLateChain = false;
};

class PlayerController : public MonoBehavior
{
public:
	void Awake() override;
	void Start() override;
	void Update(float deltaTime) override;

	// public for manipulation from state machine
	void RequestAction(ActionButton button, const DirectX::XMFLOAT3& dir, bool isLateChain);

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

	const ActionRequest& GetActionRequest() const
	{
		return actionRequest;
	}

private:
	void RegisterStates();
	PlayerState* GetState(PlayerStateID id) const;
	void ApplyPendingTransition(const PlayerIntent& intent, float deltaTime);

	void ApplyRootMotion();

private:
	const Camera3D* cam = nullptr;
	HealthComponent* health = nullptr;
	LocomotionComponent* locomotion = nullptr;
	SkeletalAnimatorComponent* animator = nullptr;
	HitVolumeComponent* hitVolume = nullptr;
	PlayerInputReader inputReader;

	static constexpr auto noState = PlayerStateID::COUNT;
	std::array<std::unique_ptr<PlayerState>, static_cast<size_t>(PlayerStateID::COUNT)> states;
	PlayerStateID currStateID = PlayerStateID::LOCOMOTION;
	PlayerStateID pendingStateID = noState;
	bool isPendingRunNow = false;

	ActionController* actionController = nullptr;
	ActionRequest actionRequest;

	const ActionDef* requestedAction = nullptr;
	float requestedMinFadeSec = 0.f;

	bool isKnockdownPending = false;
	DirectX::XMFLOAT3 hitDir{};
};
