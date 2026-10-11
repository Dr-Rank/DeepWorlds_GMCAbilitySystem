#include "Misc/AutomationTest.h"
#if WITH_AUTOMATION_WORKER
#include "GMASNativeSpawnFixture.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "FileHelpers.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "PlayInEditorDataTypes.h"
#include "Tests/AutomationCommon.h"
#include "Tests/AutomationEditorCommon.h"
#include "UObject/StrongObjectPtr.h"
#include "HAL/PlatformTime.h"
IMPLEMENT_COMPLEX_AUTOMATION_TEST(FGMASNativeSpawnSpec,"GMASNative.ListenServer.SpawnClock",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
void FGMASNativeSpawnSpec::GetTests(TArray<FString>& Names,TArray<FString>& Commands) const {Names={TEXT("Normal"),TEXT("Lag100msLoss1pct")};Commands=Names;}
bool FGMASNativeSpawnSpec::RunTest(const FString& Parameters)
{
 if(GEditor->PlayWorld){AddError(TEXT("An existing PIE session must be preserved"));return false;}
 GetMutableDefault<UGMASNativeSpawnAbility>()->AbilityTag=FGameplayTag::RequestGameplayTag(TEXT("GMAS.Native.SpawnCheck"));
 auto* World=UEditorLoadingAndSavingUtils::NewBlankMap(false);World->GetWorldSettings()->DefaultGameMode=AGMASNativeSpawnMode::StaticClass();
 auto* Floor=World->SpawnActor<AStaticMeshActor>(FVector(0,0,-50),FRotator::ZeroRotator);Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Floor->SetActorScale3D(FVector(100,100,1));Floor->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
 World->SpawnActor<APlayerStart>(FVector(0,0,150),FRotator::ZeroRotator);World->SpawnActor<APlayerStart>(FVector(300,0,150),FRotator::ZeroRotator);
 auto Settings=MakeShared<TStrongObjectPtr<ULevelEditorPlaySettings>>(NewObject<ULevelEditorPlaySettings>());
 (*Settings)->SetPlayNetMode(EPlayNetMode::PIE_ListenServer);(*Settings)->SetPlayNumberOfClients(2);(*Settings)->SetRunUnderOneProcess(true);(*Settings)->bLaunchSeparateServer=false;
 auto& Net=(*Settings)->NetworkEmulationSettings;Net.bIsNetworkEmulationEnabled=Parameters!=TEXT("Normal");Net.EmulationTarget=NetworkEmulationTarget::Client;Net.CurrentProfile=TEXT("Custom");Net.InPackets.MinLatency=Net.InPackets.MaxLatency=50;Net.OutPackets.MinLatency=Net.OutPackets.MaxLatency=50;Net.InPackets.PacketLossPercentage=Net.OutPackets.PacketLossPercentage=1;
 FRequestPlaySessionParams Params;Params.WorldType=EPlaySessionWorldType::PlayInEditor;Params.SessionDestination=EPlaySessionDestinationType::InProcess;Params.EditorPlaySettings=Settings->Get();
 ADD_LATENT_AUTOMATION_COMMAND(FStartPIEForAutomationCommand(Params));
 auto Started=MakeShared<double>(FPlatformTime::Seconds());
 ADD_LATENT_AUTOMATION_COMMAND(FFunctionLatentCommand([this,Settings,Started]()->bool{
  if(FPlatformTime::Seconds()-*Started<12)return false;
  AGMASNativeSpawnPawn* Host=nullptr;AGMASNativeSpawnPawn* ServerRemote=nullptr;AGMASNativeSpawnPawn* Client=nullptr;int32 Worlds=0;
  for(const auto& Context:GEngine->GetWorldContexts())if(Context.WorldType==EWorldType::PIE){++Worlds;for(TActorIterator<AGMASNativeSpawnPawn> It(Context.World());It;++It){auto* P=*It;if(P->GetLocalRole()==ROLE_AutonomousProxy)Client=P;else if(P->HasAuthority()){if(P->IsLocallyControlled())Host=P;else ServerRemote=P;}}}
  TestEqual(TEXT("Two native PIE worlds"),Worlds,2);
  for(auto* P:{Host,ServerRemote,Client})if(TestNotNull(TEXT("Native network participant"),P)){
   UE_LOG(LogTemp,Display,TEXT("[NativeSpawn] final pawn=%s role=%d local=%d clock=%.6f begun=%d ended=%d active=%d"),*P->GetName(),int(P->GetLocalRole()),P->IsLocallyControlled(),P->ASC->ActionTimer,P->Begun.Num(),P->Ended.Num(),P->ASC->GetActiveAbilityCount(UGMASNativeSpawnAbility::StaticClass()));
   TestEqual(TEXT("Exactly two operation IDs (startup then recovery)"),P->Begun.Num(),2);TestEqual(TEXT("Both abilities ended"),P->Ended.Num(),2);TestTrue(TEXT("Every activated operation ended"),P->Ended.Includes(P->Begun));TestEqual(TEXT("Cooldown cleared"),P->ASC->GetCooldownForAbility(FGameplayTag::RequestGameplayTag(TEXT("GMAS.Native.SpawnCheck"))),0.f);TestEqual(TEXT("No stuck ability"),P->ASC->GetActiveAbilityCount(UGMASNativeSpawnAbility::StaticClass()),0);
   TestTrue(TEXT("Real GMC clock advanced"),P->ASC->ActionTimer>2.0);for(double T:P->BeginTimes)TestTrue(TEXT("No zero-clock activation"),T>0);
   if(P->IsLocallyControlled()){TestEqual(TEXT("Input requested before first prediction tick"),P->RequestClock,0.0);TestEqual(TEXT("Held input coalesced to one deferred request"),P->DeferredCount,1);}
  }
  if(Client&&ServerRemote){TestTrue(TEXT("Client/server operation IDs agree"),Client->Begun.Includes(ServerRemote->Begun)&&ServerRemote->Begun.Includes(Client->Begun));}
  return true;
 }));
 ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;
}
#endif




