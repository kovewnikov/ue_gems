// Fill out your copyright notice in the Description page of Project Settings.


#include "OrbitCameraDebugDrawComponent.h"

#include "OrbitCameraActor.h"
#include "Materials/MaterialRenderProxy.h"

static void DrawEllipse(const FVector& Center, const FVector& ASemiAxis, const FVector& BSemiAxis, int32 Segments, const FColor& Color, float Thickness, TArray<FDebugRenderSceneProxy::FDebugLine>& OutEllipse)
{
	Segments = FMath::Max(4, Segments);
	const float AngleStep = (2.f * PI) / Segments;
	
	OutEllipse.Reserve(OutEllipse.Num() + Segments);
	
	float Angle = 0.f;
	while(Segments--)
	{
		float Sin, Cos;
		FMath::SinCos(&Sin, &Cos, Angle);
		const FVector Start = Center + ASemiAxis * Cos + BSemiAxis * Sin;
		Angle += AngleStep;
		FMath::SinCos(&Sin, &Cos, Angle);
		const FVector End = Center + ASemiAxis * Cos + BSemiAxis * Sin;
		OutEllipse.Add(FDebugRenderSceneProxy::FDebugLine(Start, End, Color, Thickness));
	}
}

static void DrawEllipsoidPolygon(FMeshElementCollector& Collector, const FMaterialRenderProxy* MaterialRenderProxy, int32 NumSides, int32 NumRings, float StartAngle, float EndAngle, const FVector& EllipsoidRadius, const FVector& Center, const FRotator& Orientation, int32 ViewIndex)
{
	// Use a mesh builder to draw the sphere.
	FDynamicMeshBuilder MeshBuilder(Collector.GetFeatureLevel());
	{
		// The first/last arc are on top of each other.
		int32 NumVerts = (NumSides + 1) * (NumRings + 1);
		FDynamicMeshVertex* Verts = (FDynamicMeshVertex*)FMemory::Malloc(NumVerts * sizeof(FDynamicMeshVertex));

		// Calculate verts for one arc
		FDynamicMeshVertex* ArcVerts = (FDynamicMeshVertex*)FMemory::Malloc((NumRings + 1) * sizeof(FDynamicMeshVertex));

		for (int32 i = 0; i<NumRings + 1; i++)
		{
			FDynamicMeshVertex* ArcVert = &ArcVerts[i];

			float angle = StartAngle + ((float)i / NumRings) * (EndAngle-StartAngle);

			// Note- unit sphere, so position always has mag of one. We can just use it for normal!			
			ArcVert->Position.X = 0.0f;
			ArcVert->Position.Y = FMath::Sin(angle);
			ArcVert->Position.Z = FMath::Cos(angle);

			ArcVert->SetTangents(
				FVector3f(1, 0, 0),
				FVector3f(0.0f, -ArcVert->Position.Z, ArcVert->Position.Y),
				ArcVert->Position
				);

			ArcVert->TextureCoordinate[0].X = 0.0f;
			ArcVert->TextureCoordinate[0].Y = ((float)i / NumRings);
		}

		// Then rotate this arc NumSides+1 times.
		for (int32 s = 0; s<NumSides + 1; s++)
		{
			FRotator3f ArcRotator(0, 360.f * (float)s / NumSides, 0);
			FRotationMatrix44f ArcRot(ArcRotator);
			float XTexCoord = ((float)s / NumSides);

			for (int32 v = 0; v<NumRings + 1; v++)
			{
				int32 VIx = (NumRings + 1)*s + v;

				Verts[VIx].Position = ArcRot.TransformPosition(ArcVerts[v].Position);

				Verts[VIx].SetTangents(
					ArcRot.TransformVector(ArcVerts[v].TangentX.ToFVector3f()),
					ArcRot.TransformVector(ArcVerts[v].GetTangentY()),
					ArcRot.TransformVector(ArcVerts[v].TangentZ.ToFVector3f())
					);

				Verts[VIx].TextureCoordinate[0].X = XTexCoord;
				Verts[VIx].TextureCoordinate[0].Y = ArcVerts[v].TextureCoordinate[0].Y;
			}
		}

		// Add all of the vertices we generated to the mesh builder.
		for (int32 VertIdx = 0; VertIdx < NumVerts; VertIdx++)
		{
			MeshBuilder.AddVertex(Verts[VertIdx]);
		}

		// Add all of the triangles we generated to the mesh builder.
		for (int32 s = 0; s<NumSides; s++)
		{
			int32 a0start = (s + 0) * (NumRings + 1);
			int32 a1start = (s + 1) * (NumRings + 1);

			for (int32 r = 0; r<NumRings; r++)
			{
				MeshBuilder.AddTriangle(a0start + r + 0, a1start + r + 0, a0start + r + 1);
				MeshBuilder.AddTriangle(a1start + r + 0, a1start + r + 1, a0start + r + 1);
			}
		}

		// Free our local copy of verts and arc verts
		FMemory::Free(Verts);
		FMemory::Free(ArcVerts);
	}
	MeshBuilder.GetMesh(FScaleMatrix(EllipsoidRadius) * FRotationMatrix(Orientation) * FTranslationMatrix(Center), MaterialRenderProxy, SDPG_World, false, false, true, ViewIndex, Collector, nullptr);
}

UOrbitCameraDebugDrawComponent::UOrbitCameraDebugDrawComponent()
{
	//to disable drawing in game view
	bHiddenInGame = true;
}

FBoxSphereBounds UOrbitCameraDebugDrawComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	FBoxSphereBounds ResultBounds(ForceInitToZero);
	const AOrbitCameraActor* OrbitCameraActor = GetOwner<AOrbitCameraActor>();
	if (!ensureMsgf(OrbitCameraActor, TEXT("Supposed to exist within AOrbitCameraActor only")))
	{
		return ResultBounds;
	}
	const FVector EllipsoidHalfSize = OrbitCameraActor->GetEllipsoidSize() / 2.f;
	const double SphereSize = FMath::Max(EllipsoidHalfSize.X, FMath::Max(EllipsoidHalfSize.Y, EllipsoidHalfSize.Z));
	
	ResultBounds = FBoxSphereBounds(FVector::Zero(), EllipsoidHalfSize, SphereSize).TransformBy(LocalToWorld);
	return ResultBounds;
}

#if UE_ENABLE_DEBUG_DRAWING
FDebugRenderSceneProxy* UOrbitCameraDebugDrawComponent::CreateDebugSceneProxy()
{
	class FOrbitTraceDebugRenderSceneProxy : public FDebugRenderSceneProxy
	{
	public:
		FOrbitTraceDebugRenderSceneProxy(const UPrimitiveComponent* InComponent)
			: FDebugRenderSceneProxy(InComponent)
		{
			DrawType = SolidMesh;
		}
		
		virtual SIZE_T GetTypeHash() const override 
		{
			static size_t UniquePointer;
			return reinterpret_cast<size_t>(&UniquePointer);
		}

		virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override 
		{
			FPrimitiveViewRelevance Result;
			Result.bDrawRelevance = IsShown(View);
			Result.bDynamicRelevance = true;
			Result.bSeparateTranslucency = Result.bNormalTranslucency = IsShown(View) && GIsEditor;
			Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);
			return Result;
		}

		virtual void GetDynamicMeshElementsForView(const FSceneView* View, const int32 ViewIndex, const FSceneViewFamily& ViewFamily, const uint32 VisibilityMap, FMeshElementCollector& Collector, FMaterialCache& DefaultMaterialCache, FMaterialCache& SolidMeshMaterialCache) const override
		{
			FDebugRenderSceneProxy::GetDynamicMeshElementsForView(View, ViewIndex, ViewFamily, VisibilityMap, Collector, DefaultMaterialCache, SolidMeshMaterialCache);
			
			const FMaterialRenderProxy* SpherePolygonColor = &Collector.AllocateOneFrameResource<FColoredMaterialRenderProxy>(GEngine->DebugMeshMaterial->GetRenderProxy(), FColor::Cyan.WithAlpha(DrawAlpha));

			DrawEllipsoidPolygon(Collector, SpherePolygonColor, 20, 7, VerticalAngleRange.X, VerticalAngleRange.Y, EllipsoidRadius, Location, Orientation, ViewIndex);
		}
		
		FVector EllipsoidRadius;

		FVector2D HorizontalAngleRange;
		FVector2D VerticalAngleRange;
		FVector Location;
		FRotator Orientation;
	};

	FOrbitTraceDebugRenderSceneProxy* DebugProxy = new FOrbitTraceDebugRenderSceneProxy(this);
	check(DebugProxy);

	const AOrbitCameraActor* OrbitCameraActor = GetOwner<AOrbitCameraActor>();
	if (!ensureMsgf(OrbitCameraActor, TEXT("Supposed to exist within AOrbitCameraActor only")))
	{
		return DebugProxy;
	}
	const USceneComponent* OrbitCameraRoot = OrbitCameraActor->GetRootComponent();
	check(OrbitCameraRoot);

	const FVector EllipsoidRadius = OrbitCameraActor->GetEllipsoidSize() / 2.f;
	const FVector XSemiAxis = OrbitCameraRoot->GetForwardVector() * EllipsoidRadius.X;
	const FVector YSemiAxis = OrbitCameraRoot->GetRightVector() * EllipsoidRadius.Y;
	const FVector ZSemiAxis = OrbitCameraRoot->GetUpVector() * EllipsoidRadius.Z;
	
	DebugProxy->EllipsoidRadius = EllipsoidRadius;
	DebugProxy->VerticalAngleRange = OrbitCameraActor->GetVerticalRotationLimit();
	DebugProxy->Location = OrbitCameraRoot->GetComponentLocation();
	DebugProxy->Orientation = OrbitCameraRoot->GetComponentRotation();
	

	//Draw X-Y Ellipse
	DrawEllipse(OrbitCameraRoot->GetComponentLocation(), XSemiAxis, YSemiAxis, 18, FColor::Green, 3, DebugProxy->Lines);

	//Draw Y-Z Ellipse
	DrawEllipse(OrbitCameraRoot->GetComponentLocation(), YSemiAxis, ZSemiAxis, 18, FColor::Green, 3, DebugProxy->Lines);
	
	//Draw X-Z Ellipse
	DrawEllipse(OrbitCameraRoot->GetComponentLocation(), XSemiAxis, ZSemiAxis, 18, FColor::Green, 3, DebugProxy->Lines);
	
	// DebugProxy->Spheres.Add(FDebugRenderSceneProxy::FSphere(400.f, OrbitCameraRoot->GetComponentLocation() + FVector::UnitZ() * 20.f, FColor::Cyan));
	
	return DebugProxy;
}

#endif // UE_ENABLE_DEBUG_DRAWING
