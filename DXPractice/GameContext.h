#pragma once

class Renderer;
class PhysicsSystem;
class ScriptSystem;
class AnimationSystem;
class RenderSystem;

struct GameContext
{
    Renderer& renderer;
    PhysicsSystem& physicsSys;
    ScriptSystem& scriptSys;
    AnimationSystem& animationSys;
    RenderSystem& renderSys;
};