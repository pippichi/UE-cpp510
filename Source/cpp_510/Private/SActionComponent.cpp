// Fill out your copyright notice in the Description page of Project Settings.


#include "SActionComponent.h"
#include "SAction.h"
#include "..\cpp_510.h"
#include "Net\UnrealNetwork.h"
#include "Engine/ActorChannel.h"

// 第一个参数是该Stat显示的名称；第二个参数是Stat名；第三个参数是组（在cpp_510.h中声明的）
DECLARE_CYCLE_STAT(TEXT("StartActionByName"), STAT_StartActionByName, STATGROUP_QYF);

// Sets default values for this component's properties
USActionComponent::USActionComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
	SetIsReplicatedByDefault(true);
}


// Called when the game starts
void USActionComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner()->HasAuthority()) {
		for (TSubclassOf<USAction> ActionClass : DefaultActions) {
			AddAction(GetOwner(), ActionClass);
		}
	}

}

void USActionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 为什么这里还需要特地StopAction？ 如果角色死亡，绑定的ActionComp也应该被垃圾回收掉，此时如果还有Action在跑，就不会被回收干净，可能会出现一系列问题。
	// Stop all
	// 为什么这里要复制一下Actions：因为c++不允许在遍历的过程中删除数组中的元素
	TArray<USAction*> ActionCopy = Actions;
	for (USAction* Action : ActionCopy) {
		if (Action && Action->IsRunning()) {
			Action->StopAction(GetOwner());
		}
	}

	Super::EndPlay(EndPlayReason);
}


// Called every frame
void USActionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
	//FString DebugMsg = GetNameSafe(GetOwner()) + " : " + ActiveGameplayTags.ToStringSimple();
	//GEngine->AddOnScreenDebugMessage(-1, 0.0f, FColor::White, DebugMsg);

	// Draw All Actions
	for (USAction* Action : Actions) {
		FColor TextColor = Action->IsRunning() ? FColor::Blue : FColor::White;

		FString ActionMsg = FString::Printf(TEXT("[%s] Action: %s : IsRuning: %s : Outer: %s"),
			*GetNameSafe(GetOwner()),
			/**Action->ActionName.ToString(),*/
			*GetNameSafe(Action),
			Action->IsRunning() ? TEXT("true") : TEXT("false"),
			*GetNameSafe(Action->GetOuter()));

		LogOnScreen(this, ActionMsg, TextColor, 0.0f);
	}
}


void USActionComponent::AddAction(AActor* Instigator, TSubclassOf<USAction> ActionClass)
{
	if (!ensureAlways(ActionClass)) {
		return;
	}
	// Skip for clients
	if (!GetOwner()->HasAuthority()) {
		UE_LOG(LogTemp, Warning, TEXT("Client attemping to AddAction. [Class: %s]"), *GetNameSafe(ActionClass));
		return;
	}
	USAction* NewAction = NewObject<USAction>(GetOwner(), ActionClass);
	if (ensure(NewAction)) {
		NewAction->Initialize(this);

		Actions.Add(NewAction);

		if (NewAction->bAutoStart && ensure(NewAction->CanStart(Instigator))) {
			NewAction->StartAction(Instigator);
		}
	}
}

void USActionComponent::RemoveAction(USAction* ActionToRemove)
{
	if (!ensure(ActionToRemove && !ActionToRemove->IsRunning())) {
		return;
	}
	Actions.Remove(ActionToRemove);
}

USAction* USActionComponent::GetAction(TSubclassOf<USAction> ActionClass) const
{
	if (ActionClass == nullptr) {
		return nullptr;
	}
	for (USAction* Action : Actions) {
		if (Action && Action->IsA(ActionClass)) {
			return Action;
		}
	}
	return nullptr;
}

bool USActionComponent::StartActionByName(AActor* Instigator, FName ActionName)
{
	SCOPE_CYCLE_COUNTER(STAT_StartActionByName);
	for (USAction* Action : Actions) {
		if (Action && Action->ActionName == ActionName) {
			if (!Action->CanStart(Instigator)) {
				FString FailedMsg = FString::Printf(TEXT("Failed to run: %s"), *ActionName.ToString());
				GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Red, FailedMsg);
				continue;
			}
			// Is Client?
			if (!GetOwner()->HasAuthority()) {
				ServerStartAction(Instigator, ActionName);
			}

			//{
			//	SCOPE_CYCLE_COUNTER(STAT_StartActionByName);
			//	Action->StartAction(Instigator);
			//}

			TRACE_BOOKMARK(TEXT("StartAction: %s"), *GetNameSafe(Action));

			Action->StartAction(Instigator);

			return true;
		}
	}
	return false;
}

bool USActionComponent::StopActionByName(AActor* Instigator, FName ActionName)
{
	for (USAction* Action : Actions) {
		if (Action && Action->ActionName == ActionName) {
			if (Action->IsRunning()) {
				if (!GetOwner()->HasAuthority()) {
					ServerStopAction(Instigator, ActionName);
				}
				Action->StopAction(Instigator);
				return true;
			}
		}
	}
	return false;
}

void USActionComponent::ServerStartAction_Implementation(AActor* Instigator, FName ActionName)
{
	StartActionByName(Instigator, ActionName);
}

void USActionComponent::ServerStopAction_Implementation(AActor* Instigator, FName ActionName)
{
	StopActionByName(Instigator, ActionName);
}

bool USActionComponent::ReplicateSubobjects(class UActorChannel* Channel, class FOutBunch* Bunch, FReplicationFlags* RepFlags)
{
	bool WroteSomething = Super::ReplicateSubobjects(Channel, Bunch, RepFlags);
	for (USAction* Action : Actions) {
		if (Action) {
			WroteSomething |= Channel->ReplicateSubobject(Action, *Bunch, *RepFlags);
		}
	}
	return WroteSomething;
}

void USActionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USActionComponent, Actions);
}