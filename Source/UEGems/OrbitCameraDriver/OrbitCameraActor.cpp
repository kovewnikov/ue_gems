// Fill out your copyright notice in the Description page of Project Settings.


#include "OrbitCameraActor.h"

#include "OrbitCameraDebugDrawComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/ArrowComponent.h"
#include "Misc/DataValidation.h"

static FVector SphericalToCartesian(const float AzimuthalAngleRads, const float PolarAngleRads, const FVector& EllipsoidSemiAxesLengths)
{
	float SinPolar, CosPolar, SinAzimuthal, CosAzimuthal;
	FMath::SinCos(&SinPolar, &CosPolar, PolarAngleRads);
	FMath::SinCos(&SinAzimuthal, &CosAzimuthal, AzimuthalAngleRads);

	return FVector(
		EllipsoidSemiAxesLengths.X * SinPolar * CosAzimuthal,
		EllipsoidSemiAxesLengths.Y * SinPolar * SinAzimuthal,
		EllipsoidSemiAxesLengths.Z * CosPolar);
}

AOrbitCameraActor::AOrbitCameraActor()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));;
	
	// Setup camera defaults
	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComponent"));
	CameraComponent->FieldOfView = 90.0f;
	CameraComponent->bConstrainAspectRatio = true;
	CameraComponent->AspectRatio = 1.777778f;
	CameraComponent->PostProcessBlendWeight = 1.0f;
	
	CameraComponent->SetupAttachment(RootComponent);
	
#if WITH_EDITORONLY_DATA
	DebugDrawComponent = CreateEditorOnlyDefaultSubobject<UOrbitCameraDebugDrawComponent>(TEXT("DebugDrawComponent"));
	if (DebugDrawComponent)
	{
		DebugDrawComponent->SetupAttachment(RootComponent);
	}
#endif
}

void AOrbitCameraActor::BeginPlay()
{
	Super::BeginPlay();

	RecalculateOrbitPosition();
}



void AOrbitCameraActor::SetCameraOrbitPosition(const float InHorizontalAngle, const float InVerticalAngle)
{
	HorizontalAngle = InHorizontalAngle;
	VerticalAngle = FMath::Clamp(InVerticalAngle, VerticalRotationLimit.X, VerticalRotationLimit.Y);
	
	RecalculateOrbitPosition();
}

void AOrbitCameraActor::IncrementCameraOrbitPosition(const float InHorizontalAngleDelta, const float InVerticalAngleDelta)
{
	HorizontalAngle += InHorizontalAngleDelta;
	VerticalAngle = FMath::Clamp(VerticalAngle + InVerticalAngleDelta, VerticalRotationLimit.X, VerticalRotationLimit.Y);
	
	RecalculateOrbitPosition();
}

void AOrbitCameraActor::RecalculateOrbitPosition()
{
	if (!ensureAlwaysMsgf(EllipsoidSize.X > 0.f && EllipsoidSize.Y > 0.f && EllipsoidSize.Z > 0.f, TEXT("Ellipsoid is degenerated")))
	{
		return;
	}

	const FVector EllipsoidSemiAxesLengths = EllipsoidSize / 2.f; 
	const FVector CameraPosition = SphericalToCartesian(HorizontalAngle, VerticalAngle, EllipsoidSemiAxesLengths);

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
	DebugDrawComponent->MarkRenderStateDirty();
	RecalculateOrbitPosition();
}

void AOrbitCameraActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

#if WITH_EDITORONLY_DATA
	if (const FProperty* Prop = PropertyChangedEvent.MemberProperty)
	{
		if (Prop->GetFName() == GET_MEMBER_NAME_CHECKED(AOrbitCameraActor, EllipsoidSize))
		{
			RecalculateOrbitPosition();
			DebugDrawComponent->MarkRenderStateDirty();
		}
	}
	if (const FProperty* Prop = PropertyChangedEvent.Property)
	{
		if (Prop->GetFName() == GET_MEMBER_NAME_CHECKED(AOrbitCameraActor, HorizontalAngle) || PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(AOrbitCameraActor, VerticalAngle))
		{
			SetCameraOrbitPosition(HorizontalAngle, VerticalAngle);
		}
	}
#endif
}

void AOrbitCameraActor::PostLoad()
{
	Super::PostLoad();

	RecalculateOrbitPosition();
}

EDataValidationResult AOrbitCameraActor::IsDataValid(FDataValidationContext& Context)
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	bool bIsDataInvalid = false;
	if (EllipsoidSize.X <= 0.f || EllipsoidSize.Y <= 0.f || EllipsoidSize.Z <= 0.f)
	{
		Context.AddError(INVTEXT("Ellipsoid is degenerated"));
		bIsDataInvalid = true;
	}
	
	Result = CombineDataValidationResults(Result, bIsDataInvalid ? EDataValidationResult::Invalid : EDataValidationResult::Valid);

	return Result;
}

#endif
