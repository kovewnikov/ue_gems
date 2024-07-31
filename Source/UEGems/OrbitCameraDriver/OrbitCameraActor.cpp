// Fill out your copyright notice in the Description page of Project Settings.


#include "OrbitCameraActor.h"

#include "Camera/CameraComponent.h"
#include "Misc/DataValidation.h"
#include "OrbitCameraDebugDrawComponent.h"
#include "Runtime/CinematicCamera/Public/CineCameraComponent.h"

static FVector SphericalToCartesian(const float AzimuthalAngleDeg, const float PolarAngleDeg, const FVector& EllipsoidSemiAxesLengths)
{
	float SinPolar, CosPolar, SinAzimuthal, CosAzimuthal;
	FMath::SinCos(&SinPolar, &CosPolar, FMath::DegreesToRadians(PolarAngleDeg));
	FMath::SinCos(&SinAzimuthal, &CosAzimuthal, FMath::DegreesToRadians(AzimuthalAngleDeg));

	return FVector(
		EllipsoidSemiAxesLengths.X * SinPolar * CosAzimuthal,
		EllipsoidSemiAxesLengths.Y * SinPolar * SinAzimuthal,
		EllipsoidSemiAxesLengths.Z * CosPolar);
}

AOrbitCameraActor::AOrbitCameraActor()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));;
	
	// Setup camera defaults
	CameraComponent = CreateDefaultSubobject<UCineCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->SetupAttachment(RootComponent);
	
#if WITH_EDITORONLY_DATA
	if (DebugDrawComponent = CreateEditorOnlyDefaultSubobject<UOrbitCameraDebugDrawComponent>(TEXT("DebugDrawComponent")))
	{
		DebugDrawComponent->SetupAttachment(RootComponent);
	}
#endif
}

void AOrbitCameraActor::BeginPlay()
{
	Super::BeginPlay();

	RecalculateCameraPosition();
}

void AOrbitCameraActor::SetCameraAngles(const float InHorizontalAngleDeg, const float InVerticalAngleDeg)
{
	HorizontalAngleDeg = InHorizontalAngleDeg;
	VerticalAngleDeg = FMath::Clamp(InVerticalAngleDeg, VerticalAngleLimitDeg.X, VerticalAngleLimitDeg.Y);
	
	RecalculateCameraPosition();
}

void AOrbitCameraActor::IncrementCameraAngles(const float InHorizontalAngleDelta, const float InVerticalAngleDelta)
{
	HorizontalAngleDeg += InHorizontalAngleDelta;
	VerticalAngleDeg = FMath::Clamp(VerticalAngleDeg + InVerticalAngleDelta, VerticalAngleLimitDeg.X, VerticalAngleLimitDeg.Y);
	
	RecalculateCameraPosition();
}

void AOrbitCameraActor::SetEllipsoidSize(const FVector& InEllipsoidSize)
{
	EllipsoidSize = InEllipsoidSize;

	RecalculateCameraPosition();
	if (DebugDrawComponent)
	{
		DebugDrawComponent->MarkRenderStateDirty();	
	}
}

void AOrbitCameraActor::RecalculateCameraPosition()
{
	if (!ensureAlwaysMsgf(EllipsoidSize.X > 0.f && EllipsoidSize.Y > 0.f && EllipsoidSize.Z > 0.f, TEXT("Ellipsoid is degenerated")))
	{
		return;
	}

	const FVector EllipsoidSemiAxesLengths = EllipsoidSize / 2.f; 
	const FVector CameraPosition = SphericalToCartesian(HorizontalAngleDeg, VerticalAngleDeg, EllipsoidSemiAxesLengths);

	const FVector XAxis = -CameraPosition.GetSafeNormal();
	const FVector YAxis = (FVector::UpVector ^ XAxis).GetSafeNormal();
	const FVector ZAxis = (XAxis ^ YAxis);

	const FMatrix NewCameraMat(XAxis, YAxis, ZAxis, CameraPosition);

	CameraComponent->SetWorldTransform(FTransform(NewCameraMat) * CameraComponent->GetOwner()->GetTransform());
}

#if WITH_EDITOR

void AOrbitCameraActor::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	if (DebugDrawComponent)
	{
		DebugDrawComponent->MarkRenderStateDirty();	
	}
}

void AOrbitCameraActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
#if WITH_EDITORONLY_DATA
	if (PropertyChangedEvent.MemberProperty && PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(AOrbitCameraActor, VerticalAngleLimitDeg))
	{
		if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(FVector, X))
		{
			VerticalAngleLimitDeg.X = FMath::Clamp(VerticalAngleLimitDeg.X, 0.f, VerticalAngleLimitDeg.Y);
		}
		else if(PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(FVector, Y))
		{
			VerticalAngleLimitDeg.Y = FMath::Clamp(VerticalAngleLimitDeg.Y, VerticalAngleLimitDeg.X, 180.f);
		}
		RecalculateCameraPosition();
	}
#endif
	
	Super::PostEditChangeProperty(PropertyChangedEvent);

#if WITH_EDITORONLY_DATA
	if (PropertyChangedEvent.MemberProperty && PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(AOrbitCameraActor, EllipsoidSize))
	{
		RecalculateCameraPosition();
		if (DebugDrawComponent)
		{
			DebugDrawComponent->MarkRenderStateDirty();	
		}
	}
	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(AOrbitCameraActor, HorizontalAngleDeg) || PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(AOrbitCameraActor, VerticalAngleDeg))
	{
		RecalculateCameraPosition();
	}
	
#endif
}

void AOrbitCameraActor::PostLoad()
{
	Super::PostLoad();

	RecalculateCameraPosition();
}

EDataValidationResult AOrbitCameraActor::IsDataValid(FDataValidationContext& Context)
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	bool bIsDataInvalid = false;
	if (EllipsoidSize.X <= 0.f || EllipsoidSize.Y <= 0.f || EllipsoidSize.Z <= 0.f)
	{
		Context.AddError(INVTEXT("Ellipsoid has degenerated"));
		bIsDataInvalid = true;
	}
	
	Result = CombineDataValidationResults(Result, bIsDataInvalid ? EDataValidationResult::Invalid : EDataValidationResult::Valid);

	return Result;
}

#endif
