#include "DragonAI.h"

#include <imgui.h>

#include "DragonStates.h"
#include "GameObject.h"
#include "HealthComponent.h"
#include "LocomotionComponent.h"
#include "Scene.h"
#include "TextRenderer.h"
#include "SkeletalAnimatorComponent.h"


void DragonAI::Start()
{
	MonoBehavior::Start();

	animator = owner->GetComponent<SkeletalAnimatorComponent>();
	transform = owner->GetTransform();
	locomotion = owner->GetComponent<LocomotionComponent>();
	if (!animator || !transform || !locomotion)
	{
#ifndef NDEBUG
		textRenderer->Log("[DragonAI] %s: no animator / transform / locomotion, AI disabled", owner->GetName().c_str());
#endif
		SetEnabled(false);
		return;
	}

	RegisterStates();
	BindHealthCallbacks();

	RequestStateTransition(StateID::CALM);
	ApplyPendingTransition();
}

void DragonAI::Update(float deltaTime)
{
	timeInState += deltaTime;

	if (const GameObject* player = FindPlayer())
	{
		perception.Update(transform->GetPosition(), transform->GetRotation().y, player->GetTransform()->GetPosition());
	}
	else
	{
		perception.ClearTarget();
	}

	CheckAnyStateTransition();

	if (pendingStateID == noState)
	{
		if (DragonState* state = GetState(currStateID))
		{
			state->Update(*this, deltaTime);
		}
	}

	ApplyPendingTransition();

	locomotion->ApplyLocomotion(deltaTime);

	wasDamaged = false;
}

void DragonAI::RequestStateTransition(StateID nextStateID)
{
	if (pendingStateID != noState)
	{
#ifndef NDEBUG
		textRenderer->Log("[DragonAI] %s ignored, %s already pending", ToString(nextStateID), ToString(pendingStateID));
#endif
		return;
	}

	if (!GetState(nextStateID))
	{
#ifndef NDEBUG
		if (nextStateID != lastIgnoredStateID)
		{
			textRenderer->Log("[DragonAI] %s has no state object yet, ignored", ToString(nextStateID));
		}
#endif
		lastIgnoredStateID = nextStateID;
		return;
	}

	pendingStateID = nextStateID;
}

#ifndef NDEBUG
void DragonAI::DrawDebugUI()
{
	const std::string title = "DragonAI##" + owner->GetName();
	ImGui::Begin(title.c_str());

	ImGui::SeparatorText("State");
	ImGui::Text("state    %s", ToString(currStateID));
	ImGui::Text("time     %.2f s", timeInState);
	ImGui::Text("combat   %s", isInCombat ? "yes" : "no (intro pending)");

	ImGui::SeparatorText("Health Flags");
	if (health)
	{
		ImGui::Text("hp       %.0f / %.0f", health->GetCurrHp(), health->GetMaxHp());
		ImGui::Text("dead     %s", isDeathPending ? "PENDING" : "-");
		ImGui::Text("phase    %s", isPhaseChangePending ? "PENDING" : "-");
		ImGui::Text("stagger  %.1f", staggerMeter);
	}
	else
	{
		ImGui::TextDisabled("no HealthComponent");
	}

	ImGui::SeparatorText("Perception");
	if (perception.hasTarget)
	{
		ImGui::Text("distance %.2f m   (%s)", perception.distance, ToString(perception.band));
		ImGui::Text("angle    %+.1f deg (%s)", perception.angleDeg, ToString(perception.sector));
	}
	else
	{
		ImGui::TextDisabled("no target");
	}

	// live tuning: copy values that feel right back into DragonPerception.h
	if (ImGui::CollapsingHeader("Perception tuning"))
	{
		ImGui::DragFloat("close max", &perception.closeMax, 0.1f, 0.f, perception.midMax, "%.1f m");
		ImGui::DragFloat("mid max", &perception.midMax, 0.1f, perception.closeMax, 50.f, "%.1f m");
		ImGui::DragFloat("front half", &perception.frontHalfDeg, 0.5f, 0.f, perception.backStartDeg, "%.0f deg");
		ImGui::DragFloat("back start", &perception.backStartDeg, 0.5f, perception.frontHalfDeg, 180.f, "%.0f deg");
		ImGui::DragFloat("distance hysteresis", &perception.distanceHysteresis, 0.05f, 0.f, 3.f, "%.2f m");
		ImGui::DragFloat("angle hysteresis", &perception.angleHysteresis, 0.5f, 0.f, 30.f, "%.0f deg");
	}

	ImGui::End();
}

void DragonAI::DrawDebugGizmos(DebugRenderer& debugRenderer) const
{
	perception.DrawDebug(debugRenderer, transform->GetPosition(), transform->GetRotation().y);

	if (const DragonState* state = GetState(currStateID))
	{
		state->DrawDebug(*this, debugRenderer);
	}
}
#endif

void DragonAI::RegisterStates()
{
	states[ToIndex(StateID::CALM)] = std::make_unique<DragonCalmState>();
	states[ToIndex(StateID::TURN)] = std::make_unique<DragonTurnState>();
}

void DragonAI::BindHealthCallbacks()
{
	health = owner->GetComponent<HealthComponent>();

	if (!health)
	{
		return;
	}

	health->onDamaged = [this](float damage, float stagger)
	{
		staggerMeter += stagger;
		wasDamaged = true;
	};

	health->onThreasholdCrossed = [this]()
	{
		isPhaseChangePending = true;
	};

	health->onDeath = [this]()
	{
		isDeathPending = true;
	};
}

void DragonAI::CheckAnyStateTransition()
{
	// death check
	if (isDeathPending)
	{
		if (currStateID != StateID::DEAD)
		{
			ForceStateTransition(StateID::DEAD);
		}
		return;
	}

	// phase change check
	if (isPhaseChangePending && CanCurrentBeInterrupted(InterruptStateKind::PHASE_CHANGE))
	{
		if (ForceStateTransition(StateID::TAKE_OFF))
		{
			isPhaseChangePending = false;
		}
		return;
	}

	// TODO: stagger
}

bool DragonAI::ForceStateTransition(StateID nextStateID)
{
	if (!GetState(nextStateID))
	{
		return false;
	}

	pendingStateID = nextStateID;
	return true;
}

bool DragonAI::CanCurrentBeInterrupted(InterruptStateKind kind) const
{
	const DragonState* currState = GetState(currStateID);
	return !currState || currState->CanBeInterrupted(kind);
}

void DragonAI::ApplyPendingTransition()
{
	if (pendingStateID == noState)
	{
		return;
	}

	const StateID nextStateID = pendingStateID;
	pendingStateID = noState;
	lastIgnoredStateID = noState;

	if (DragonState* prevState = GetState(currStateID))
	{
		prevState->Exit(*this);
	}

	currStateID = nextStateID;
	timeInState = 0.f;
	GetState(currStateID)->Enter(*this);
}

DragonState* DragonAI::GetState(StateID id) const
{
	if (id >= StateID::COUNT)
	{
		return nullptr;
	}

	return states[ToIndex(id)].get();
}

const GameObject* DragonAI::FindPlayer() const
{
	const Scene* scene = owner->GetContext().currScene;
	if (!scene)
	{
		return nullptr;
	}

	for (const auto& object : scene->GetObjects())
	{
		if (object->GetTag() == ObjectTag::PLAYER && !scene->IsPendingDestroy(object.get()))
		{
			return object.get();
		}
	}

	return nullptr;
}
