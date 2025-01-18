// Fill out your copyright notice in the Description page of Project Settings.


#include "SInteractionComponent.h"
#include "SGamePlayInterface.h"
#include "SWorldUserWidget.h"

static TAutoConsoleVariable<bool> CVarDebugDrawInteraction(TEXT("su.InteractionDebugDraw"), false, TEXT("Enable Debug Lines for Interact Component."), ECVF_Cheat);

// Sets default values for this component's properties
USInteractionComponent::USInteractionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...

	TraceRadius = 30.0f;
	TraceDistance = 500.0f;
	CollisionChannel = ECC_WorldDynamic;
}

// Called when the game starts
void USInteractionComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

// Called every frame
void USInteractionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
	APawn* MyPawn = Cast<APawn>(GetOwner());
	if (MyPawn->IsLocallyControlled()) {
		FindBestInteractable();
	}
}



void USInteractionComponent::FindBestInteractable()
{
	bool bDrawDebug = CVarDebugDrawInteraction.GetValueOnGameThread();

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(CollisionChannel);

	AActor* MyActor = GetOwner();
	FVector EyeLocation;
	FRotator EyeRotator;
	MyActor->GetActorEyesViewPoint(EyeLocation, EyeRotator);

	FVector End = EyeLocation + (EyeRotator.Vector() * TraceDistance);

	TArray<FHitResult> HitResults;
	FCollisionShape Shape;
	Shape.SetSphere(TraceRadius);
	bool bHitSomething = GetWorld()->SweepMultiByObjectType(HitResults, EyeLocation, End, FQuat::Identity, ObjectQueryParams, Shape);

	/*FHitResult OutHit;
	bool bHitSomething = GetWorld()->LineTraceSingleByObjectType(OutHit, EyeLocation, End, ObjectQueryParams);*/

	/*if (AActor* HitActor = OutHit.GetActor()) {
		if (HitActor->Implements<USGamePlayInterface>()) {
			APawn* MyPawn = Cast<APawn>(MyActor);
			ISGamePlayInterface::Execute_Interact(HitActor, MyPawn);
		}
	}*/

	FColor Color = bHitSomething ? FColor::Green : FColor::Red;

	// Clear ref before trying to fill
	FocusActor = nullptr;

	for (FHitResult OutHit : HitResults) {
		if (bDrawDebug) {
			DrawDebugSphere(GetWorld(), OutHit.ImpactPoint, TraceRadius, 32, Color, false, 2.0f);
		}
		if (AActor* HitActor = OutHit.GetActor()) {
			if (HitActor->Implements<USGamePlayInterface>()) {
				FocusActor = HitActor;
				break;
			}
		}
	}

	if (FocusActor) {
		if (DefaultWidgetInstance == nullptr && ensure(DefaultWidgetClass)) {
			DefaultWidgetInstance = CreateWidget<USWorldUserWidget>(GetWorld(), DefaultWidgetClass);
		}

		if (DefaultWidgetInstance) {
			DefaultWidgetInstance->AttachedActor = FocusActor;
			if (!DefaultWidgetInstance->IsInViewport()) {
				DefaultWidgetInstance->AddToViewport();
			}
		}
	}
	else {
		if (DefaultWidgetInstance) {
			DefaultWidgetInstance->RemoveFromParent();
		}
	}

	if (bDrawDebug) {
		DrawDebugLine(GetWorld(), EyeLocation, End, Color, false, 2.f, 0, 2.f);
	}
}

void USInteractionComponent::PrimaryInteract() {
	ServerInteract(FocusActor);
}

void USInteractionComponent::ServerInteract_Implementation(AActor* InFocus)
{
	if (InFocus == nullptr) {
		GEngine->AddOnScreenDebugMessage(-1, 1.0f, FColor::Red, "No Focus Actor to interact.");
		return;
	}

	APawn* MyPawn = Cast<APawn>(GetOwner());
	ISGamePlayInterface::Execute_Interact(InFocus, MyPawn);

}