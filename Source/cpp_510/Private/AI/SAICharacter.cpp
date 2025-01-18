// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/SAICharacter.h"
#include "AIModule/Classes/Perception/PawnSensingComponent.h"
#include "AIModule\Classes\AIController.h"
#include "AIModule/Classes/BehaviorTree/BlackboardComponent.h"
#include "SAttributeComponent.h"
#include "AIModule\Classes\BrainComponent.h"
#include "SWorldUserWidget.h"
#include "Components\CapsuleComponent.h"
#include "GameFramework\CharacterMovementComponent.h"
#include "Blueprint\UserWidget.h"
#include "SActionComponent.h"

// Sets default values
ASAICharacter::ASAICharacter()
{
    PawnSensingComp = CreateDefaultSubobject<UPawnSensingComponent>("PawnSensingComp");
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AttributeComp = CreateDefaultSubobject<USAttributeComponent>("AttributeComp");

    ActionComp = CreateDefaultSubobject<USActionComponent>("ActionComp");

    // Disabled on capsule to let projectiles pass through capsule and hit mesh instead
    //GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Ignore);
    // Enabled on mesh to react to incoming projectiles
    GetMesh()->SetGenerateOverlapEvents(true);

    TimeToHitParamName = "TimeToHit";
    TargetActorKey = "TargetActor";
}

void ASAICharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();

    PawnSensingComp->OnSeePawn.AddDynamic(this, &ASAICharacter::OnPawnSeen);
    AttributeComp->OnHealthChanged.AddDynamic(this, &ASAICharacter::OnHealthChanged);
}

void ASAICharacter::OnHealthChanged(AActor* InstigatorActor, USAttributeComponent* OwningComp, float NewHealth, float Delta)
{
    if (Delta < 0.0f) {
        if (InstigatorActor != this) {
            SetTargetActor(InstigatorActor);
        }
        if (ActiveHealthBar == nullptr) {
            ActiveHealthBar = CreateWidget<USWorldUserWidget>(GetWorld(), HealthBarWidgetClass);
            if (ActiveHealthBar) {
                ActiveHealthBar->AttachedActor = this;
                // 这一步会调用Widget蓝图中的Event Construct
                ActiveHealthBar->AddToViewport();
            }
        }
        GetMesh()->SetScalarParameterValueOnMaterials(TimeToHitParamName, GetWorld()->TimeSeconds);
        if (NewHealth <= 0.0f) {
            // stop BT
            AAIController* AIC = Cast<AAIController>(GetController());
            if (AIC) {
                AIC->GetBrainComponent()->StopLogic("Killed");
            }
            // ragdoll
            GetMesh()->SetAllBodiesSimulatePhysics(true);
            GetMesh()->SetCollisionProfileName("Ragdoll");

            GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            GetCharacterMovement()->DisableMovement();

            // set lifespan
            SetLifeSpan(10.0f);
        }
    }
}

void ASAICharacter::SetTargetActor(AActor* NewTarget)
{
    AAIController* AIC = Cast<AAIController>(GetController());
    if (AIC) {
        AIC->GetBlackboardComponent()->SetValueAsObject(TargetActorKey, NewTarget);
    }
}

AActor* ASAICharacter::GetTargetActor() const
{
    AAIController* AIC = Cast<AAIController>(GetController());
    if (AIC) {
        return Cast<AActor>(AIC->GetBlackboardComponent()->GetValueAsObject(TargetActorKey));
    }
    return nullptr;
}

void ASAICharacter::OnPawnSeen(APawn* Pawn)
{
    // Ignore if target already set
    if (GetTargetActor() != Pawn) {
        
        SetTargetActor(Pawn);

        MulticastPawnSeen();

    }
    // DrawDebugString(GetWorld(), GetActorLocation(), "PLAYER SPOTTED", nullptr, FColor::White, 4.0f, true);
}

void ASAICharacter::MulticastPawnSeen_Implementation()
{
    USWorldUserWidget* NewWidget = CreateWidget<USWorldUserWidget>(GetWorld(), SpottedWidgetClass);
    if (NewWidget) {
        NewWidget->AttachedActor = this;
        // Index of 10 (or anything higher than default of 0) places this on top of ant other widget
        // May end up begind the minion health bar otherwise.
        NewWidget->AddToViewport(10);
    }
}

