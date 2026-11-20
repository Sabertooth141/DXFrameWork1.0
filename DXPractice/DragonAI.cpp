#include "DragonAI.h"

#include <imgui.h>

#include "DragonStates.h"
#include "GameObject.h"
#include "HealthComponent.h"
#include "Keyboard.h"
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

	hitVolume = owner->GetComponent<HitVolumeComponent>();

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
		perception.Update(transform->GetPosition(), transform->GetRotation().y, player->GetTransform()->GetPosition(), deltaTime);
	}
	else
	{
		perception.ClearTarget();
	}

	attackSelector.TickCooldowns(deltaTime);
	attackSelector.Evaluate(perception, currPhase);

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

	if (keyboard->KeyIsTriggered(VK_F5))
	{
		owner->GetComponent<HealthComponent>()->ApplyDamage(100, 0);
	}
}

bool DragonAI::RequestStateTransition(StateID nextStateID)
{
	if (pendingStateID != noState)
	{
#ifndef NDEBUG
		textRenderer->Log("[DragonAI] %s ignored, %s already pending", ToString(nextStateID), ToString(pendingStateID));
#endif
		return false;
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
		return false;
	}

	pendingStateID = nextStateID;
	return true;
}

bool DragonAI::RequestAttack(int attkIndex)
{
	if (attkIndex < 0 || attkIndex >= attackSelector.GetAttkCount())
	{
		return false;
	}

	if (!RequestStateTransition(StateID::ATTACK))
	{
		return false;
	}

	currAttkIndex = attkIndex;
	return true;
}

void DragonAI::OnHitDealt(const HitEvent& event)
{
	if (currStateID != StateID::ATTACK && currStateID != StateID::AIR_ATTACK)
	{
		return;
	}

	hasAttackHit = true;

#ifndef NDEBUG
	textRenderer->Log("[DragonAI] %s hit %s (%s)", event.hitboxGroup.c_str(),
	                  event.victim->GetName().c_str(), event.hurtboxName.c_str());
#endif
}

void DragonAI::EnterPhase2()
{
	currPhase = 2;
	isEnraging = false;
	currRecoverScale = phase2RecoverScale;
	animator->SetTimeScale(phase2TimeScale);
}

#ifndef NDEBUG
void DragonAI::DrawDebugUI()
{
	const std::string title = "DragonAI##" + owner->GetName();
	ImGui::Begin(title.c_str());

	ImGui::SeparatorText("State");
	ImGui::Text("state    %s", ToString(currStateID));
	ImGui::Text("time     %.2f s", timeInState);
	ImGui::Text("phase    %d %s", currPhase, isPhaseChangePending ? "(change PENDING)" : "");
	if (health)
	{
		ImGui::Text("hp floor %.0f", health->minHp);
	}
	ImGui::Text("strafe   %s %.1f m/s",
	            strafeMotion.GetPhase() == StrafeMotion::Phase::ACCEL ? "accel" : strafeMotion.GetPhase() == StrafeMotion::Phase::CRUISE ? "cruise" : strafeMotion.GetPhase() == StrafeMotion::Phase::SLOWDOWN ? "slowdown" : "-",
	            strafeMotion.GetSpeed());
	ImGui::Text("combat   %s", isInCombat ? "yes" : "no (intro pending)");
	ImGui::Text("enraging %s", isEnraging ? "yes" : "-");
	if (currAttkIndex >= 0)
	{
		ImGui::Text("attack   %s (%s)", attackSelector.GetOption(currAttkIndex).name.c_str(), hasAttackHit ? "hit" : "-");
	}

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
		ImGui::Text("velocity %.1f m/s", sqrtf(perception.targetVel.x * perception.targetVel.x +
			            perception.targetVel.z * perception.targetVel.z));
	}
	else
	{
		ImGui::TextDisabled("no target");
	}

	ImGui::SeparatorText("Attacks");
	if (ImGui::BeginTable("attacks", 3, ImGuiTableFlags_SizingFixedFit))
	{
		const ImVec4 pickColor{0.3f, 1.f, 0.4f, 1.f};
		const ImVec4 validColor{1.f, 1.f, 1.f, 1.f};
		const ImVec4 rejectColor{0.55f, 0.55f, 0.55f, 1.f};

		for (int i = 0; i < attackSelector.GetAttkCount(); i++)
		{
			const AttackOption& option = attackSelector.GetOption(i);
			const AttackReject reject = attackSelector.GetReject(i);
			const bool isBest = i == attackSelector.GetBestAttkIndex();
			const ImVec4 color = isBest ? pickColor : reject == AttackReject::NONE ? validColor : rejectColor;

			ImGui::TableNextRow();
			ImGui::TableNextColumn();
			ImGui::TextColored(color, "%s", option.name.c_str());
			ImGui::TableNextColumn();
			ImGui::TextColored(color, "p%d", option.priority);
			ImGui::TableNextColumn();

			switch (reject)
			{
			case AttackReject::NONE:
				ImGui::TextColored(color, isBest ? "<- pick" : "valid (lower priority)");
				break;
			case AttackReject::TARGET:
				ImGui::TextColored(color, "no target");
				break;
			case AttackReject::PHASE:
				ImGui::TextColored(color, "phase %d", currPhase);
				break;
			case AttackReject::COOLDOWN:
				ImGui::TextColored(color, "cooldown %.1f s", attackSelector.GetCooldownLeft(i));
				break;
			case AttackReject::RANGE:
				ImGui::TextColored(color, "range %.1f (%.0f-%.0f)", perception.distance, option.minRange, option.maxRange);
				break;
			case AttackReject::SECTOR:
				ImGui::TextColored(color, "sector %s", ToString(perception.sector));
				break;
			}
		}
		ImGui::EndTable();
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
	states[ToIndex(StateID::ROAR)] = std::make_unique<DragonRoarState>();
	states[ToIndex(StateID::APPROACH)] = std::make_unique<DragonApproachState>();
	states[ToIndex(StateID::IDLE)] = std::make_unique<DragonIdleState>();
	states[ToIndex(StateID::ATTACK)] = std::make_unique<DragonAttackState>();
	states[ToIndex(StateID::RECOVER)] = std::make_unique<DragonRecoverState>();
	states[ToIndex(StateID::TAKE_OFF)] = std::make_unique<DragonSequenceClipState>("TakeOff", StateID::AIR_AIM);
	states[ToIndex(StateID::AIR_AIM)] = std::make_unique<DragonAirAimState>();
	states[ToIndex(StateID::LAND)] = std::make_unique<DragonSequenceClipState>("Land", StateID::TURN);
	states[ToIndex(StateID::AIR_ATTACK)] = std::make_unique<DragonAirAttackState>();
	states[ToIndex(StateID::AIR_RECOVER)] = std::make_unique<DragonAirRecoverState>();
	states[ToIndex(StateID::DEAD)] = std::make_unique<DragonDeadState>();
}

void DragonAI::BindHealthCallbacks()
{
	health = owner->GetComponent<HealthComponent>();

	if (!health)
	{
		return;
	}

	health->minHp = health->GetMaxHp() * health->thresholdRatio;

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
	if (isPhaseChangePending && isInCombat && CanCurrentBeInterrupted(InterruptStateKind::PHASE_CHANGE))
	{
		if (ForceStateTransition(StateID::TAKE_OFF))
		{
			isPhaseChangePending = false;
			BeginPhaseChange();
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

void DragonAI::BeginPhaseChange()
{
	isEnraging = true;

	if (health)
	{
		health->minHp = 0.f;
	}
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
