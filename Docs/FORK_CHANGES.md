> **Historical record retained on 11 October 2026.** The refresh incorporates DeepWorlds dev through 946dac57. Upstream now covers the four original fork patches; review current test and gameplay evidence before relying on the older conclusions below.

# Fork changes

Every change this fork makes to the upstream plugin, with the evidence that identified it and a
test for deciding whether it is still needed after a future vendor drop.

**Upstream baseline:** [`7d836b20`](https://github.com/DeepWorldsSA/DeepWorlds_GMCAbilitySystem/commit/7d836b20d7a0c5ca87244bd300a33d6847b75cd6)
on DeepWorlds' `dev` branch (26 June 2026) — *"fix(gmas): guard remote-pawn first-frame OOB in
GenAncillaryTick"*. All line references and quoted source are from that commit.

**Engine requirement:** UE **5.5 or newer**. Built and run on 5.8. See [patch 4](#4-instancedstruct-include-paths-for-ue-55).

Everything here was found while integrating GMAS into a **listen-server + client** project. Three
of the four patches are networking bugs whose symptoms only appear in that topology; a dedicated
server or a standalone session will not reproduce them, and in one case will not reproduce them
*because the buggy code never runs there*.

## Contents

| # | Change | Severity | Commit |
|---|---|---|---|
| 1 | [`bPreventConcurrentActivation` was never read](#1-bpreventconcurrentactivation-is-declared-documented-blueprint-exposed--and-never-read) | High — abilities starve indefinitely | `ca1b28b` |
| 2 | [A Client RPC bound to a server broadcast runs on the host](#2-a-client-rpc-bound-to-a-server-broadcast-runs-on-the-server-double-applying-operations) | High — exactly-once violation | `6067fd0` |
| 3 | [The payload cache never prunes on a server](#3-the-operation-payload-cache-never-prunes-on-a-server-and-leaks-entries-for-consumed-payloads) | Medium — unbounded growth | `24773e8` |
| 4 | [`InstancedStruct` include paths for UE 5.5+](#4-instancedstruct-include-paths-for-ue-55) | Build — will not compile otherwise | `cd3544f` |

What is **not** in this fork is as important as what is. The upstream source is heavily annotated in
a style that reads like project-local work — numbered `Bug #3` references, the bilateral
predicted-end defer and `ClientGraceTime`, the `[ReplayBurst]` diagnostics and
`UGMASReplayBurstSettings`, `UGMASNetworkTimingSettings`, the `GMASApplyTrace` CVar, dated
`TEMP TEST` comments, and the whole `GMASBoundQueueV2` rewrite. **All of that is upstream's.** The
four patches below are the entire delta.

Issues observed but deliberately **not** fixed are in [KNOWN_ISSUES.md](KNOWN_ISSUES.md), with the
reasoning for leaving each one alone.

---

## 1. `bPreventConcurrentActivation` is declared, documented, Blueprint-exposed — and never read

**Where:** `Private/Components/GMCAbilityComponent.cpp`,
`UGMC_AbilitySystemComponent::QueueAbility` (line 753)

### The defect

```cpp
UFUNCTION(BlueprintCallable, DisplayName="Activate Ability", Category="GMAS|Abilities")
void QueueAbility(FGameplayTag InputTag, const UInputAction* InputAction = nullptr,
                  bool bPreventConcurrentActivation = false);
```

The header documents the parameter as *"prevents the concurrent activation of abilities already in
progress. The check is made locally reducing server charge, but also required in case of activation
key wait inside the ability."*

The function body never references it. Blueprint authors who tick the box get no protection and no
warning.

### Why it is worse than a no-op

`TryActivateAbility` does reject the duplicate, at the `bAllowMultipleInstances` / "Already
Instanced" check (line 689) — but **after** the operation has been given an ID, cached via
`MakeOperationData` → `CacheOperationPayload`, and pushed into `ClientQueuedOperations`.

On a client that is expensive, because of how the queue drains:

1. `GetNextOperationID` (`GMASBoundQueueV2.h:50`) returns `--NextOperationID` on `NM_Client`, so
   every client-initiated activation has a **negative** ID.
2. In `GenPreLocalMoveExecution` (`GMASBoundQueueV2.cpp:126`), a single negative ID sets
   `bAllServerBroadcast = false`, forcing the single-op path.
3. That path drains **exactly one operation per movement tick**, via `ClientQueuedOperations.Pop()`
   — i.e. **LIFO**, acknowledged in the comment at line 152 as an existing quirk kept for compat.

A caller that queues the same ability every frame therefore keeps a freshly-pushed duplicate
permanently on top of the stack, and **any operation queued once starves indefinitely.**

### How a reasonable project hits this

An Enhanced Input action with no explicit triggers uses the implicit Down trigger: `Started` fires
once, `Triggered` fires **every frame while held**. Binding a hold-style ability such as sprint to
`Triggered` is normal, and often necessary — it is what lets the ability re-evaluate a block
condition and resume mid-hold. Such a project calls `QueueAbility` once per frame, and ticks
`bPreventConcurrentActivation` precisely because the API offers it.

### Measured

Remote client against a listen server, sprint held for ~3.4 s with a reload pressed in the middle:

| Observation | Value |
|---|---|
| `Ability Activation for GA_Sprint_C Stopped (Already Instanced)` | **294** over 3.39 s |
| Reload ability processed during that window | **none** |
| First reload processing | **8 ms after the last sprint activation** |

User-visible symptom: *the client cannot reload while sprinting, and the reload then fires by
itself, with no new input, the instant sprint is released.*

The listen-server host is unaffected, because `QueueAbility` routes authority activations to
`QueueServerOperation` (positive ID, grace period), which never enters `ClientQueuedOperations`.

Upstream's own comment on `UGMCAbility::bAllowMultipleInstances`
(`Public/Ability/GMCAbility.h:146`) already documents the gap: *"If false, the actual activation
(but not the queuing) of an ability will fail if the ability already is active."*

### The change

Inserted in `QueueAbility` immediately after `Candidates` is populated, reusing it rather than
calling `GetGrantedAbilitiesByTag` twice:

```cpp
if (bPreventConcurrentActivation)
{
    for (const TSubclassOf<UGMCAbility>& AbilityClass : Candidates)
    {
        if (!AbilityClass) continue;
        const UGMCAbility* AbilityCDO = AbilityClass->GetDefaultObject<UGMCAbility>();
        if (AbilityCDO && !AbilityCDO->bAllowMultipleInstances && GetActiveAbilityCount(AbilityClass) > 0)
        {
            return;   // already running — do not spend a queue slot
        }
    }
}
```

It mirrors the existing line-689 check, so the accept/reject decision is identical to before; only
the point at which it happens moves. The parameter still defaults to `false`, so no existing caller
changes behaviour, and `bAllowMultipleInstances` is honoured so abilities that legitimately stack
are untouched.

Placed **before** the client-auth branch, which is correct: an already-active single-instance
ability should not be re-activated by that path either.

### Upstream-fixed check

If a future drop reads `bPreventConcurrentActivation` anywhere in `QueueAbility`, drop this hunk
entirely.

If instead they fix it at the *queue* layer — by making `GenPreLocalMoveExecution` drain FIFO, or
drain more than one client-initiated operation per move — **keep this hunk anyway.** It is still the
documented API contract, and the starvation symptom being gone does not make the parameter work.

---

## 2. A Client RPC bound to a server broadcast runs on the server, double-applying operations

**Where:** `Private/Components/GMCAbilityComponent.cpp`,
`RPCOnServerOperationAdded_Implementation` (line ~1111)

### The defect

`RPCOnServerOperationAdded` is a Client RPC, but `QueueServerOperation` **broadcasts**
`OnServerOperationAdded`, to which it is bound — and UE resolves a Client RPC aimed at the host's own
connection to **Local** callspace. So on a listen server, and in standalone, the body executes **on
the server** for a locally-controlled pawn, pushing server-broadcast IDs into
`ClientQueuedOperations`.

From there:

1. `GenPreLocalMoveExecution` — whose guard includes `IsLocallyControlledListenServerPawn` — drains
   them into an `FGMASBoundQueueV2BatchOperation`
2. the batch applies the operations and writes a `BatchAcknowledgeOperation`
3. `ServerProcessAcknowledgedOperation` applies **the same payloads a second time**, then clears them
4. any surviving reference to that batch logs
   `[BatchOp] Sub-operation N payload missing from cache … Authority=1`

### Why it is easy to miss

It presents as log noise — pairs of that Error alongside `ClientQueuedOperations has N pending
operations on server` during host weapon fire. The effects *are* applied at step 2, so nothing looks
broken. But step 3 is a genuine **exactly-once violation**, and is the hazard behind effects with
`bUniqueByEffectTag = false` stacking.

What actively hid it: a source comment near the `GenAncillaryTick` → `ProcessOperation` call
asserted this could not happen —

> *"Server-side ClientQueuedOperations cannot be populated (all 4 callers gated by !HasAuthority(),
> and RPCOnServerOperationAdded is Client-RPC)."*

That claim is false for a listen server. Meanwhile `CheckValidState` had been reporting the
condition correctly the entire time, and its Error was being read as expected noise. **A correct
diagnostic was disbelieved because a comment asserted the state it reported was impossible.**

### The change

A `HasAuthority()` early-return that hands the operation to the **existing** grace-timeout drain by
setting its grace period to `0.f`, so `GenAncillaryTick` force-applies it on the next tick and clears
the payload.

Applying inline was deliberately avoided: `QueueServerOperation` is broadcast from inside
`ApplyAbilityEffect`, so a nested apply could re-enter `ActiveEffects` while a caller is
mid-iteration.

The false comment is replaced with the corrected reasoning — commentary only, no behaviour change,
but worth carrying forward so the wrong conclusion is not re-derived. `CheckValidState`'s Error is
now a genuine canary rather than expected noise.

Also demotes an unconditional per-operation
`UE_LOG(LogTemp, Warning, TEXT("RPCOnServerOperationAdded: %d"), OperationID)` to
`LogGMCAbilitySystem, Verbose`. It fired on every operation in a hot path, and `LogTemp` is a
category many projects suppress wholesale.

### Upstream-fixed check

Drop this patch if a future drop guards `RPCOnServerOperationAdded_Implementation` on authority
itself, **or** stops binding a Client RPC to a delegate that is broadcast server-side.

Note that the comment correction and the log demotion are independently worth keeping even if the
guard lands upstream in a different form.

---

## 3. The operation-payload cache never prunes on a server, and leaks entries for consumed payloads

**Where:** `Private/Utility/GMASBoundQueueV2.cpp`

### 3a. The prune was guarded to clients only

`GMCMoveCounter++` and `ClearStaleOperationData()` sat behind
`if (GMCMovementComponent->GetNetMode() >= NM_Client)`.

`ENetMode` orders Standalone(0), DedicatedServer(1), ListenServer(2), Client(3) — so `>= NM_Client`
means the prune **never ran on any server, or in standalone.** Servers do cache a payload per
operation, through `MakeOperationData` → `CacheOperationPayload`, and each one appends to
`OperationDataCacheExpiration`. That array grew by one entry per operation for the lifetime of the
process, with nothing able to prune it — thousands of entries over a match of sustained weapon fire.

The guard is removed; the prune runs on every role.

### 3b. Consumed payloads leaked their expiration entry

On finding an aged-out entry whose payload was already gone, the body logged a Warning and
`continue`d **without `RemoveCurrent()`**.

But consumed-then-expired is the *normal* path — the payload is cleared by
`ServerAcknowledgeOperation`, or by the grace-timeout force, and neither touches this array. So every
completed operation leaked an entry and then re-warned on every subsequent tick. The entry is now
dropped silently.

### 3c. A grace-period skip, needed because of 3a

Entries whose payload is still in `ServerQueuedBoundOperationsGracePeriods` are now **skipped, not
dropped.**

This becomes load-bearing once 3a makes the prune run server-side: yanking a still-pending payload
would silently **lose** the operation rather than merely delay it, because `GenAncillaryTick`'s
grace-timeout drain tests `OperationPayloads.Contains` before forcing.

Safe at the `MoveHistoryMaxSize` default of 256 (~4 s at 60 Hz, against the 1 s default grace) — but
that property is `EditAnywhere` with `ClampMin 8` (~0.13 s), so the two windows **can invert** on a
project that lowers it. See [KNOWN_ISSUES.md](KNOWN_ISSUES.md#movehistorymaxsize-can-invert-against-the-grace-period).

`MaximumFreshMoveIndex` is also widened from `int` to `int64` to match `GMCMoveCounter`.

### Upstream-fixed check

- If a future drop increments and prunes unconditionally, drop **3a**.
- If it calls `RemoveCurrent()` on the consumed-payload path, drop **3b**.
- **3c** is ours regardless, unless they add an equivalent guard — and it must not be dropped while
  3a is present, or operations will be lost rather than delayed.

---

## 4. `InstancedStruct` include paths for UE 5.5+

**Where:** six headers under `Public/`

`StructUtils` was folded into `CoreUObject` in UE 5.5, moving the header to
`StructUtils/InstancedStruct.h`. Five of the six affected headers included it by its pre-5.5 path
unconditionally, so they do not resolve on a modern engine.

`GMASSyncedEvent.h` was the only file that handled this, via:

```cpp
#if ENGINE_MAJOR_VERSION >= 5 && ENGINE_MINOR_VERSION >= 5
    #include "StructUtils/InstancedStruct.h"
#else
    #include "InstancedStruct.h"
#endif
```

**This fork replaces that guard with the unconditional modern include rather than propagating it to
the other five.** The guard only bought support for engine versions that the other five headers
could not compile on anyway, so spreading it would have implied a portability the plugin did not
actually have.

The trade is explicit and is the reason this fork states 5.5+: **if you need UE 5.4 or earlier,
this patch is wrong for you** — restore the guard in all six files instead. It is a mechanical
change and nothing else in the fork depends on it.

### Upstream-fixed check

Drop this patch as soon as a vendor drop uses the `StructUtils/` path itself. This is the patch most
likely to be fixed upstream and the least interesting to carry.

---

## Verification status

Listen server + one client, in PIE. Compiled clean with the editor closed, and the change confirmed
present in the built DLL rather than inferred from timestamps — UBT will report *"Target is up to
date"* having executed zero actions.

| Patch | Status |
|---|---|
| 1 — `bPreventConcurrentActivation` | **Verified by behaviour** (2026-08-17). A client holding sprint could not reload; after the patch it reloads normally. Numeric confirmation still outstanding — see the [VeryVerbose trap](KNOWN_ISSUES.md#a-post-fix-count-of-0-may-mean-the-log-category-is-off). |
| 2 — authority guard | **Verified** (2026-07-30). `ClientQueuedOperations has N pending operations on server` went from present to **0**; `[BatchOp] … payload missing from cache` no longer occurs during weapon fire. |
| 3 — cache prune | **Verified** (2026-07-30), same session. |
| 4 — include paths | **Verified by compilation** on UE 5.8. |

Four regression canaries stayed at **0** across the 2026-07-30 session:

- `Tried to queue server operation`
- `OperationID … still in cache expiration map`
- `ServerQueuedBoundOperationsGracePeriods has N pending operations on client`
- `OperationPayloads has invalid operation ID N on server`

Exercised in that session: host and client weapon fire; host and client bleedout through to death
and ragdoll; revive with a partial clock; a per-life revive cap; host sprint and reload.
