// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreTypes.h"
#include "GameFramework/Actor.h"
#include "OrbitCameraActor.generated.h"

class UCineCameraComponent;
class UOrbitCameraDebugDrawComponent;

/*
 * Wrapper around the camera which exposes handy API for orbiting the camera
 * References:  https://en.wikipedia.org/wiki/Spherical_coordinate_system
 */
UCLASS()
class UEGEMS_API AOrbitCameraActor : public AActor
{
	GENERATED_BODY()

public:
	AOrbitCameraActor();

private:

	virtual void BeginPlay() override;

public:

	UFUNCTION(BlueprintCallable)
	void SetCameraAngles(const float InHorizontalAngleDeg, const float InVerticalAngleDeg);

	UFUNCTION(BlueprintCallable)
	void IncrementCameraAngles(const float InHorizontalAngleDelta, const float InVerticalAngleDelta);

	UFUNCTION(BlueprintCallable)
	void SetEllipsoidSize(const FVector& InEllipsoidSize);

	const FVector& GetEllipsoidSize() const { return EllipsoidSize; };

	const FVector2D& GetVerticalRotationLimitDegrees() const { return VerticalAngleLimitDeg; }

private:
	void RecalculateCameraPosition();
	
#if WITH_EDITOR
	virtual void PostEditMove(bool bFinished) override;
	
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual void PostLoad() override;

	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) override;
#endif

protected:
	// Ellipsoid width, length and height
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", Meta = (ExposeOnSpawn = true))
	FVector EllipsoidSize = FVector(100.f, 100.f, 100.f);

	// Polar angle range of allowed values in degrees
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", Meta = (ExposeOnSpawn = true))
	FVector2D VerticalAngleLimitDeg = FVector2D(6.f, 174.f);

	// Azimuthal angle in degrees within the spherical coordinate system [0, ..., 360]
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Camera", meta=(UIMin = "0.0", UIMax = "360"))
	float HorizontalAngleDeg = 180.f;

	// Polar angle in degrees within the spherical coordinate system [0, ..., 180]
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Camera", meta=(UIMin = "0.0", UIMax = "180"))
	float VerticalAngleDeg = 90.f;
	
	UPROPERTY(Category = CameraActor, VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<UCineCameraComponent> CameraComponent;
	
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<UOrbitCameraDebugDrawComponent> DebugDrawComponent;
#endif
};
