#include "GMASNativeSpawnFixture.h"
#include "Actors/GMCPlayerController.h"
#include "Components/CapsuleComponent.h"
#include "NativeGameplayTags.h"
static FGameplayTag SpawnTag(){return FGameplayTag::RequestGameplayTag(TEXT("GMAS.Native.SpawnCheck"));}
UGMASNativeSpawnAbility::UGMASNativeSpawnAbility(){CooldownTime=0.75f;bAllowMultipleInstances=false;bActivateOnMovementTick=true;}
void UGMASNativeSpawnAbility::BeginAbilityEvent_Implementation()
{
 Started=OwnerAbilityComponent->ActionTimer;
 auto* P=CastChecked<AGMASNativeSpawnPawn>(OwnerAbilityComponent->GetOwner());
 P->Begun.Add(GetAbilityID());P->BeginTimes.Add(Started);
 UE_LOG(LogTemp,Display,TEXT("[NativeSpawn] begin pawn=%s role=%d local=%d id=%d clock=%.6f"),*P->GetName(),int(P->GetLocalRole()),P->IsLocallyControlled(),GetAbilityID(),Started);
}
void UGMASNativeSpawnAbility::TickEvent_Implementation(float){if(OwnerAbilityComponent->ActionTimer-Started>=0.25)EndAbility();}
void UGMASNativeSpawnAbility::EndAbilityEvent_Implementation(){CastChecked<AGMASNativeSpawnPawn>(OwnerAbilityComponent->GetOwner())->Ended.Add(GetAbilityID());}
AGMASNativeSpawnPawn::AGMASNativeSpawnPawn()
{
 Capsule=CreateDefaultSubobject<UCapsuleComponent>(TEXT("Capsule"));Capsule->InitCapsuleSize(42,95);Capsule->SetCollisionProfileName(TEXT("Pawn"));SetRootComponent(Capsule);
 ASC=CreateDefaultSubobject<UGMC_AbilitySystemComponent>(TEXT("Abilities"));
 Movement=CreateDefaultSubobject<UGMASNativeSpawnMovement>(TEXT("Movement"));Movement->SetUpdatedComponent(Capsule);ASC->GMCMovementComponent=Movement;
 SetReplicates(true);
}
void AGMASNativeSpawnPawn::BeginPlay(){Super::BeginPlay();FAbilityMapData Map;Map.InputTag=SpawnTag();Map.Abilities.Add(UGMASNativeSpawnAbility::StaticClass());ASC->AddAbilityMapData(Map);}
void UGMASNativeSpawnMovement::BindReplicationData_Implementation(){Super::BindReplicationData_Implementation();CastChecked<AGMASNativeSpawnPawn>(GetOwner())->ASC->BindReplicationData();}
void UGMASNativeSpawnMovement::PreLocalMoveExecution_Implementation(const FGMC_Move& Move)
{
 Super::PreLocalMoveExecution_Implementation(Move);auto* P=CastChecked<AGMASNativeSpawnPawn>(GetOwner());
 if(P->RequestClock<0){P->RequestClock=P->ASC->ActionTimer;for(int32 I=0;I<32;++I)P->ASC->QueueAbility(SpawnTag());P->DeferredCount=P->ASC->GetStartupAbilityRequestCountForTest();UE_LOG(LogTemp,Display,TEXT("[NativeSpawn] early pawn=%s role=%d clock=%.6f deferred=%d"),*P->GetName(),int(P->GetLocalRole()),P->RequestClock,P->DeferredCount);}
 if(!P->RequestedAgain&&P->ASC->ActionTimer>=2.0){P->RequestedAgain=true;P->ASC->QueueAbility(SpawnTag());}
 P->ASC->PreLocalMoveExecution();
}
void UGMASNativeSpawnMovement::GenPredictionTick_Implementation(float D){Super::GenPredictionTick_Implementation(D);CastChecked<AGMASNativeSpawnPawn>(GetOwner())->ASC->GenPredictionTick(D);}
void UGMASNativeSpawnMovement::GenSimulationTick_Implementation(float D){Super::GenSimulationTick_Implementation(D);CastChecked<AGMASNativeSpawnPawn>(GetOwner())->ASC->GenSimulationTick(D);}
void UGMASNativeSpawnMovement::GenAncillaryTick_Implementation(float D,bool L,bool C){Super::GenAncillaryTick_Implementation(D,L,C);CastChecked<AGMASNativeSpawnPawn>(GetOwner())->ASC->GenAncillaryTick(D,C);}
AGMASNativeSpawnMode::AGMASNativeSpawnMode(){DefaultPawnClass=AGMASNativeSpawnPawn::StaticClass();PlayerControllerClass=AGMC_PlayerController::StaticClass();}

