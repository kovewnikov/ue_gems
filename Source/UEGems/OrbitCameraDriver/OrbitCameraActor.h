// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreTypes.h"
#include "GameFramework/Actor.h"
#include "OrbitCameraActor.generated.h"

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
	void SetCameraOrbitPosition(const float InHorizontalAngle, const float InVerticalAngle);

	UFUNCTION(BlueprintCallable)
	void IncrementCameraOrbitPosition(const float InHorizontalAngleDelta, const float InVerticalAngleDelta);

	const FVector& GetEllipsoidSize() const { return EllipsoidSize; };

	const FVector2D& GetVerticalRotationLimit() const { return VerticalRotationLimit; }

private:
	void RecalculateOrbitPosition();
	
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

	// Polar angle range of allowed values
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera", Meta = (ExposeOnSpawn = true))
	FVector2D VerticalRotationLimit = FVector2D(0.17f, PI - 0.17f);

	// Azimuthal angle in terms of spherical coordinate system [0, ..., 2PI]
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, meta=(UIMin = "0.0", UIMax = "6.2831"))
	float HorizontalAngle = PI;

	// Polar angle in terms of spherical coordinate system [0, ..., PI]
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, meta=(UIMin = "0.0", UIMax = "3.14159"))
	float VerticalAngle = HALF_PI;
	
	UPROPERTY(Category = CameraActor, VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<class UCameraComponent> CameraComponent;
	
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	TObjectPtr<class UOrbitCameraDebugDrawComponent> DebugDrawComponent;
#endif
};
