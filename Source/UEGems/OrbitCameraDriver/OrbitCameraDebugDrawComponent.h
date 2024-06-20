// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Debug/DebugDrawComponent.h"
#include "OrbitCameraDebugDrawComponent.generated.h"

class FDebugRenderSceneProxy;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UOrbitCameraDebugDrawComponent : public UDebugDrawComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UOrbitCameraDebugDrawComponent();

private:
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	

#if UE_ENABLE_DEBUG_DRAWING
	virtual FDebugRenderSceneProxy* CreateDebugSceneProxy() override;
#endif
	
};
