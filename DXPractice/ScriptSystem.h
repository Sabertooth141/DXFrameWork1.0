#pragma once
#include <vector>

#include "MonoBehavior.h"

class ScriptSystem
{
public:
	void RegisterScript(MonoBehavior* script)
	{
		script->Awake();
		pendingScripts.push_back(script);
	}

	void Update(float deltaTime)
	{
		for (auto* script : pendingScripts)
		{
			if (script->IsEnabled())
			{
				script->Start();
				script->SetStarted(true);
			}
			activeScripts.push_back(script);
		}
		pendingScripts.clear();

		for (auto* script : activeScripts)
		{
			if (script->IsEnabled())
			{
				script->Update(deltaTime);
			}
		}

		FlushDestroyed();
	}

	void UnregisterScript(MonoBehavior* script)
	{
		script->OnDestroy();
		destroyQueue.push_back(script);
	}

private:
	void FlushDestroyed()
	{
		for (auto* destroyed : destroyQueue)
		{
			std::erase(activeScripts, destroyed);
		}
		destroyQueue.clear();
	}

private:
	std::vector<MonoBehavior*> pendingScripts;
	std::vector<MonoBehavior*> activeScripts;
	std::vector<MonoBehavior*> destroyQueue;
};
