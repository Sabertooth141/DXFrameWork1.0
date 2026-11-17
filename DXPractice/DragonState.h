#pragma once

#define DRAGON_STATE_LIST(X)	\
	X(CALM)						\
	X(TURN)						\
	X(ROAR)						\
	X(IDLE)						\
	X(APPROACH)					\
	X(REPOSITION)				\
	X(ATTACK)					\
	X(RECOVER)					\
	X(HIT_REACT)				\
	X(TAKE_OFF)					\
	X(AIR_AIM)					\
	X(AIR_ATTACK)				\
	X(LAND)						\
	X(DEAD)
#include <cstdint>

class DebugRenderer;
class DragonAI;

enum class StateID : uint8_t
{
#define X(name) name,
	DRAGON_STATE_LIST(X)
#undef X
	COUNT
};

inline constexpr const char* stateIDNames[] =
{
#define X(name) #name,
	DRAGON_STATE_LIST(X)
#undef X
};

inline constexpr size_t ToIndex(StateID id)
{
	return static_cast<size_t>(id);
}

inline const char* ToString(StateID id)
{
	if (id < StateID::COUNT)
	{
		return stateIDNames[ToIndex(id)];
	}

	return "NONE";
}

enum class InterruptStateKind : uint8_t
{
	STAGGER,
	PHASE_CHANGE
};

class DragonState
{
public:
	virtual ~DragonState() = default;

	virtual void Enter(DragonAI& ai)
	{
	}

	virtual void Update(DragonAI& ai, float deltaTime)
	{
	}

	virtual void Exit(DragonAI& ai)
	{
	}

	// default to be true - change per state
	virtual bool CanBeInterrupted(InterruptStateKind kind) const
	{
		return true;
	}

#ifndef NDEBUG
	virtual void DrawDebug(const DragonAI& ai, DebugRenderer& debugRenderer) const
	{
	}
#endif
};
