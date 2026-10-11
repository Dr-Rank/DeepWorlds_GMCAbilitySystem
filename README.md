# GMAS fork for UE 5.8

This fork incorporates DeepWorlds' `dev` branch through
[`946dac57`](https://github.com/DeepWorldsSA/DeepWorlds_GMCAbilitySystem/commit/946dac57ea7fae0d9b2d2276efd1c63a83afba3f),
using upstream's current queue and operation-consumption implementations. It builds with Unreal Engine 5.8.

The refresh brings operation-consumption and cache fixes, effect/tag safety, task heartbeat changes,
external attribute values, percentage-of-base modifiers, Blueprint ability-cost access and modifier editor support.
The fork also includes the spawn-clock guard described below and corrected automation fixtures from CLEAR,
extended to the remaining attribute stress tests. Production clamp defaults and cooldown behavior are unchanged.

**Editor regression result: 1,290/1,290 passed in GMCv2_Demo on 11 October 2026.**
GMAS passed 428/428, Traverse Anywhere passed 861/861 and one additional engine test passed.
Before correction, GMCLocomotion reproduced 150 failing GMAS tests and CLEAR reproduced 119.
Most failures came from arithmetic fixtures inheriting an active [0, 0] clamp; three cooldown tests
advanced DeltaTime instead of the prediction clock. The Traversal collision failure came from preview
geometry being queried before its physics body existed. The corrected fixture finishes mesh compilation
and creates collision before tracing; traversal gameplay behavior is unchanged.

**Immediate activation after spawn:** `QueueAbility` retains early requests until the first finite,
positive GMC prediction timestamp. Repeated held input for the same tag is coalesced while preserving
request order. No activation IDs, cooldowns or ability instances are created during that wait.
Direct `TryActivateAbility` calls return `false` before readiness, so callers should retry or use `QueueAbility`.
Actor-owned startup regression tests and cooldown replay checks pass. A native listen-server/client
first-spawn check is still required before treating that network edge case as fully qualified.

The editor results do not certify Traverse Anywhere's unfinished GMC setup pipeline, native multiplayer,
visible contact or packaged gameplay. Back up and test your own project.
See [the verification record](Docs/UE58_VALIDATION.json) and [preserved pre-fix failures](Docs/UE58_VALIDATION_BEFORE_FIXTURES.json).
The original patch and issue documents remain historical evidence:

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
