#include "DragonAI.h"

#include "DragonStates.h"
#include "GameObject.h"
#include "Scene.h"
#include "TextRenderer.h"
#include "SkeletalAnimatorComponent.h"


void DragonAI::Start()
{
	MonoBehavior::Start();

	animator = owner->GetComponent<SkeletalAnimatorComponent>();
	transform = owner->GetTransform();
	if (!animator || !transform)
	{
#ifndef NDEBUG
		textRenderer->Log("[DragonAI] %s: no animator / transform, AI disabled", owner->GetName().c_str());
#endif
		SetEnabled(false);
		return;
	}

	RegisterStates();

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

	if (pendingStateID == noState)
	{
		if (DragonState* state = GetState(currStateID))
		{
			state->Update(*this, deltaTime);
		}
	}

	ApplyPendingTransition();
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
		textRenderer->Log("[DragonAI] %s has no state object yet, ignored", ToString(nextStateID));
#endif
		return;
	}

	pendingStateID = nextStateID;
}

void DragonAI::RegisterStates()
{
	states[ToIndex(StateID::CALM)] = std::make_unique<DragonCalmState>();
}

void DragonAI::ApplyPendingTransition()
{
	if (pendingStateID == noState)
	{
		return;
	}

	const StateID nextStateID = pendingStateID;
	pendingStateID = noState;

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
