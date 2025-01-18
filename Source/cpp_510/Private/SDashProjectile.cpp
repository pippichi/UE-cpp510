// Fill out your copyright notice in the Description page of Project Settings.


#include "SDashProjectile.h"

#include "Kismet/GameplayStatics.h"
#include "GameFramework\ProjectileMovementComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Sound\SoundCue.h"

ASDashProjectile::ASDashProjectile()
{
	TeleportDelay = 0.2f;
	DetonateDelay = 0.2f;

	MoveComp->InitialSpeed = 6000.f;
}

void ASDashProjectile::BeginPlay()
{
	Super::BeginPlay();

	GetWorldTimerManager().SetTimer(TimerHandle_DelayedDetonate, this, &ASDashProjectile::Explode, DetonateDelay);
}

void ASDashProjectile::Explode_Implementation()
{
	// Skip base implementation as it will destroy actor (we need to stay alive a bit longer to finish the 2nd timer)
	// Super::Explode_Implementation();

	// Clear timer if the Explode was already called through another source like OnActorHit
	GetWorldTimerManager().ClearTimer(TimerHandle_DelayedDetonate);
	UGameplayStatics::SpawnEmitterAtLocation(this, ImpactVFX, GetActorLocation(), GetActorRotation());

	UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());

	EffectComp->DeactivateSystem();

	MoveComp->StopMovementImmediately();
	SetActorEnableCollision(false);

	FTimerHandle TimerHandle_DelayedTeleport;
	GetWorldTimerManager().SetTimer(TimerHandle_DelayedTeleport, this, &ASDashProjectile::TeleportInstigator, TeleportDelay);
}

void ASDashProjectile::TeleportInstigator()
{
	AActor* ActorToTeleport = GetInstigator();
	if (ensureAlways(ActorToTeleport)) {
		// Keep instigator rotation or it may end up jarring
		FVector OriginLocation = ActorToTeleport->GetActorLocation();
		FVector TargetLocation = GetActorLocation();
		TargetLocation.Z = OriginLocation.Z + 50.0f;
		UE_LOG(LogTemp, Warning, TEXT("Transform from %s to %s !!"), *ActorToTeleport->GetActorLocation().ToString(), *TargetLocation.ToString());
		ActorToTeleport->TeleportTo(TargetLocation, ActorToTeleport->GetActorRotation(), false, false);
	
		// Play shake on the player we teleported
		APawn* InstigatorPawn = Cast<APawn>(ActorToTeleport);
		APlayerController* PC = Cast<APlayerController>(InstigatorPawn->GetController());
		if (PC && PC->IsLocalController()) {
			PC->ClientStartCameraShake(ImpactShake);
		}

		Destroy();
	}
}
