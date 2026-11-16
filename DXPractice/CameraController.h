#pragma once
#include "MonoBehavior.h"

class Camera3D;

class CameraController : public MonoBehavior
{
public:
	void Awake() override;
	void Start() override;
	void LateUpdate(float deltaTime) override;

private:
	void HandleOrbitInput();
	void HandleZoomInput();
	DirectX::XMFLOAT3 GetTargetPoint() const;
	void ApplyToCamera() const;

#ifndef NDEBUG
	void FrameDragon();
#endif

public:
	// target
	float heightOffset = 1.5f;
	float followSpeed = 10.f;

	// orbit
	float lookSens = 0.004f;
	float minPitchDeg = -10.f;
	float maxPitchDeg = 60.f;

	// zoom
	float distance = 6.f;
	float minDistance = 3.f;
	float maxDistance = 12.f;
	float zoomStep = 1.f;

#ifndef NDEBUG
	float framePitchDeg = 15.f;
#endif

private:
	Camera3D* cam = nullptr;

	float yaw = 0.f;
	float orbitPitch = 0.f;
	DirectX::XMFLOAT3 smoothedTarget{};
};
