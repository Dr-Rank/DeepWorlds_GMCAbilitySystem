# Native first-spawn ability regression (lab only)

This optional test plugin runs a real GMC listen server and owning client in two PIE worlds inside one Unreal Editor process. It does not inject timestamps or replace GMC's move loop. It requires UE 5.8, GMC, GMCMotion and this GMAS fork. No Traverse Anywhere plugin or demo assets are required by the harness.

Copy this folder into a disposable project's `Plugins` directory, enable `GMASNativeSpawnLab` in its `.uproject`, and build the editor. Run `Automation RunTests GMASNative.ListenServer.SpawnClock`. Use an empty editor session: the test creates an unsaved blank map and ends its own PIE session. Do not run it while editing an unsaved level.

The two cases are `Normal` and `Lag100msLoss1pct` (50 ms inbound and outbound latency, 1% loss in both directions on the client's connection). Each creates a fresh session with a native `AGMC_Pawn`, `UGMCMotion`, `UGMC_AbilitySystemComponent` and `AGMC_PlayerController`. The movement component forwards the same GMAS bind, prediction, simulation, ancillary and local-input callbacks that a GMC integration must forward.

The first local move requests the ability 32 times **before GMAS receives its first prediction tick**, with `ActionTimer == 0`. Assertions check one coalesced deferred request, positive-clock activation, recovery for a second request at GMC timestamp 2 seconds, matching client/server operation IDs, both abilities ending, and no active ability or remaining cooldown after 12 seconds. The native test ability uses a 0.75-second cooldown and ends after 0.25 seconds. Observations cover the listen host, the owning client and its authoritative server copy; simulated proxies do not execute predicted abilities.

This qualifies the startup-clock/input-queue edge case for the tested integration. It does not certify every customer ability graph, reconnect/respawn policy, task chain, packaged server, or Traverse Anywhere's unfinished GMC gameplay setup.

**Exclude this plugin from release builds.** Its native pawn, ability, game mode and gameplay tag exist solely for regression testing. The GMAS runtime guard itself is in the normal `Source/GMCAbilitySystem` module; users do not need this test plugin to get the fix.

Evidence and hashes: [native spawn record](../../Docs/UE58_NATIVE_SPAWN_VALIDATION.json).
