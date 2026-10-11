# GMAS fork for UE 5.8

This fork incorporates DeepWorlds' `dev` branch through
[`946dac57`](https://github.com/DeepWorldsSA/DeepWorlds_GMCAbilitySystem/commit/946dac57ea7fae0d9b2d2276efd1c63a83afba3f).
The source from the four original fork patches is now covered by upstream. The refresh
keeps the fork's history and documentation; the runtime and editor source match that upstream
revision. Upstream now includes version-aware InstancedStruct headers, and the merged source
built successfully with Unreal Engine 5.8 in GMCv2_Demo on 11 October 2026.

The refresh also brings upstream's operation-consumption and cache fixes, effect/tag safety,
task heartbeat changes, external attribute values, percentage-of-base modifiers, Blueprint
ability-cost access and modifier editor support. See the upstream commit history for details.

**Qualification is still in progress.** In GMCv2_Demo, the complete editor suite ran 1,287 tests: GMAS passed 275 of 425; Traverse Anywhere passed 860 of 861; one additional engine test passed. The 150 GMAS failures and single traversal failure were already present before the refresh. All 19 focused GMC/first-person bridge checks passed. See [the validation record](Docs/UE58_VALIDATION.json) for the failed test names. A successful build does not certify native multiplayer, visible contact or packaged gameplay. Back up and test your own project.
The old patch and known-issue documents below are retained as historical evidence, not a
current assertion that upstream is missing those four fixes.

- [Original fork changes](Docs/FORK_CHANGES.md)
- [Recorded known issues](Docs/KNOWN_ISSUES.md)

Everything below is the original upstream README.

---

<p align="center">
  <a href="" rel="noopener">
 <img width=200px height=200px src="https://i.imgur.com/WlQIK41.png" alt="Project logo"></a>
</p>

<h3 align="center">GMC Ability System (GMAS)</h3>

<div align="center">

  [![Status](https://img.shields.io/badge/status-active-success.svg)]() 
  [![GitHub Issues](https://img.shields.io/github/issues/reznok/GMCAbilitySystem.svg)](https://github.com/reznok/GMCAbilitySystem/issues)
  [![GitHub Pull Requests](https://img.shields.io/github/issues-pr/reznok/GMCAbilitySystem.svg)](https://github.com/reznok/GMCAbilitySystem/pulls)
  [![License](https://img.shields.io/badge/license-MIT-blue.svg)](/LICENSE)

</div>

---

<p align="center"> An ability system for use with the <a href="https://www.unrealengine.com/marketplace/en-US/product/general-movement-component">General Movement Component </a> <br>
  See the <a href="https://github.com/reznok/GMCAbilitySystem/wiki">Wiki</a> for more details
    <br> 
</p>
