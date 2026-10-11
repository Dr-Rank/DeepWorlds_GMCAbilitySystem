using UnrealBuildTool;
public class GMASNativeSpawnLabRuntime:ModuleRules
{
 public GMASNativeSpawnLabRuntime(ReadOnlyTargetRules Target):base(Target)
 {
 PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs;
 PrivateDependencyModuleNames.AddRange(new[]{"Core","CoreUObject","Engine","GameplayTags"});
 }
}
