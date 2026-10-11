using UnrealBuildTool;
public class GMASNativeSpawnLabEditor:ModuleRules
{
 public GMASNativeSpawnLabEditor(ReadOnlyTargetRules Target):base(Target)
 {
 PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs;
 PrivateDependencyModuleNames.AddRange(new[]{"Core","CoreUObject","Engine","GameplayTags","UnrealEd","GMCCore","GMCMotion","GMCAbilitySystem"});
 }
}
