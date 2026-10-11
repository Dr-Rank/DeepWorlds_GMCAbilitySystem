> **Historical record retained on 11 October 2026.** The refresh incorporates DeepWorlds dev through 946dac57. Upstream now covers the four original fork patches; review current test and gameplay evidence before relying on the older conclusions below.

# Known issues and measurement traps

Things found alongside the [fork changes](FORK_CHANGES.md) and deliberately **not** fixed, plus the
ways this area of the plugin is easy to measure wrongly.

Nothing here is a regression introduced by this fork. Two of the entries are pre-existing upstream
behaviour, two are limits of the patches themselves, and two are traps that produced convincing
false conclusions during the work and cost real time.

## Contents

**Not fixed**
- [Spawn-time `[BatchOp] … payload missing from cache`](#spawn-time-batchop--payload-missing-from-cache)
- [The queue is only robust because callers are well-behaved](#the-queue-is-only-robust-because-callers-are-well-behaved)
- [`MoveHistoryMaxSize` can invert against the grace period](#movehistorymaxsize-can-invert-against-the-grace-period)

**Measurement traps**
- [A post-fix count of 0 may mean the log category is off](#a-post-fix-count-of-0-may-mean-the-log-category-is-off)
- [`Forcing Operation On Server` rising is expected](#forcing-operation-on-server-rising-is-expected)
- [Log counts are only comparable between sessions of similar length](#log-counts-are-only-comparable-between-sessions-of-similar-length)

---

# Not fixed

## Spawn-time `[BatchOp] … payload missing from cache`

**Status:** pre-existing upstream behaviour, benign, logged at Error. Not fixed here.

A handful per session, all within roughly 1.6 s of level start and always on the first few operation
IDs. Distinct from the fire-time occurrences that
[patch 2](FORK_CHANGES.md#2-a-client-rpc-bound-to-a-server-broadcast-runs-on-the-server-double-applying-operations)
eliminates — if you see these during sustained weapon fire, that is the other bug, not this one.

**Cause:** the grace countdown is `It.Value() -= DeltaTime` against a 1.0 s budget, so the long
frames during PIE startup burn the entire grace before the client's acknowledgement can round-trip.
The server force-applies, so **nothing is lost.** The client's later batch then references payloads
the server has already consumed, and that is what logs at Error.

**Why it was left alone:** it is cosmetic, and the current code genuinely cannot tell the two
outcomes apart — "already applied" and "never delivered" reach the same line with the same
information.

**The clean fix, if someone wants it:** distinguish the outcomes rather than clamping `DeltaTime` or
converting the grace to move counts. Have `ServerAcknowledgeOperation` and the force drain record
recently-consumed IDs in a small bounded set; then log **Verbose "already applied"** for a consumed
ID and reserve **Error** for one that was never delivered.

Clamping `DeltaTime` would hide the symptom without making the two cases distinguishable, which is
worse than the current state.

---

## The queue is only robust because callers are well-behaved

**Status:** structural, understood, not fixed. Read this before "fixing" the queue.

[Patch 1](FORK_CHANGES.md#1-bpreventconcurrentactivation-is-declared-documented-blueprint-exposed--and-never-read)
removes the *producer* that was starving the client operation queue. It does **not** make the queue
robust. Any future caller that activates an ability every frame without setting
`bPreventConcurrentActivation` reproduces the starvation exactly.

### Do not "fix" this by switching `Pop()` to `RemoveAt(0)`

That was the first instinct here, and it is wrong on inspection.

**The one-op-per-move limit is a wire constraint, not an ordering policy.** Per the comment at
`GMASBoundQueueV2.cpp:110-125`, `OperationData` is a single `FInstancedStruct` bound into the move,
so exactly one can travel per move. Server-broadcast operations can be batched because they ship only
**IDs** — both sides already hold the payload in `OperationPayloads`. Client-initiated operations
must ship the **full payload**, because only the client has it, so they cannot go into the ID-list
wrapper and only one can be sent per move. That ceiling is structural, and no ordering change touches
it.

Given that, **FIFO does not fix the failure mode — it relabels it.** Under a sustained per-frame
producer the queue still grows without bound; instead of one operation starving forever, every
operation executes progressively later. A reload firing four seconds after the input is a
correctness problem too, just a less visible one than a reload that never fires.

### The LIFO ordering itself is defensible

`Pop()` is the natural O(1) choice for a queue assumed to hold 0–1 entries, where ordering is
unobservable. Upstream's note at `:153` calling it *"an existing quirk, kept as-is for compat"* reads
as declining an unrelated behavioural change while adding the batching path, not as a design being
defended.

**The actual weakness is that "the queue is always short" is a load-bearing assumption that nothing
enforces.**

### Useful directions, in increasing cost

1. **Stop duplicates entering the queue.** This is what patch 1 does, and it is already the
   documented contract. Cheapest, and it removes the dominant producer.
2. **Age out stale client-initiated operations.** A backlog then cannot persist even if some future
   caller floods the queue without setting the flag. This is the defensive fix: it makes the queue
   robust rather than relying on every caller being well-behaved. **This is the one to reach for if
   the problem recurs.**
3. **A batch wrapper able to carry multiple full payloads.** The thorough fix — it removes the
   one-per-move ceiling for client operations entirely — but it is a wire-format change with real
   cost and compatibility implications, and is probably only worth it if (1) and (2) prove
   insufficient in practice.

---

## `MoveHistoryMaxSize` can invert against the grace period

**Status:** latent, guarded against, not fully resolved.

[Patch 3c](FORK_CHANGES.md#3c-a-grace-period-skip-needed-because-of-3a) skips pruning any payload
still sitting in `ServerQueuedBoundOperationsGracePeriods`, so a still-pending operation cannot be
yanked out from under the grace-timeout drain.

That guard holds. What it does not do is make the two windows independent of each other:

| | Value | Approx. at 60 Hz |
|---|---|---|
| `MoveHistoryMaxSize` (prune window) | 256 default | ~4.3 s |
| Grace period | 1.0 s default | 1.0 s |
| `MoveHistoryMaxSize` **minimum** | `ClampMin 8` | **~0.13 s** |

`MoveHistoryMaxSize` is `EditAnywhere`. A project that lowers it toward the clamp minimum inverts the
relationship — the prune window becomes far shorter than the grace period. The 3c guard means such an
operation is *skipped rather than lost*, which is the safe direction, but the cache then retains
entries specifically in the configuration where the project was trying to bound memory.

**If you lower `MoveHistoryMaxSize`, check it against your grace period.** The principled fix is
probably to express the grace in move counts rather than seconds, so the two cannot diverge — that is
an upstream design question and is not attempted here.

---

# Measurement traps

## A post-fix count of 0 may mean the log category is off

**This produced a convincing false positive during the work on patch 1. Expect to hit it.**

`Ability Activation for %s Stopped (Already Instanced)` is logged at **`VeryVerbose`**. So is the new
skip line added by patch 1.

A post-fix session that has not raised the category shows that count at **0**, down from 294 — which
looks exactly like the flood having stopped, when it only means the line is suppressed. **Do not cite
294 → 0 as evidence.** The two explanations are indistinguishable from that data.

**To measure it properly**, run with:

```
Log LogGMCAbilitySystem VeryVerbose
```

and confirm **both** of the following:

- the patch-1 skip line fires roughly once per frame while the input is held, **and**
- `Already Instanced` stays at **0**

The first without the second means the flood moved rather than stopped. The second without the first
means the category is still suppressed and you have learned nothing.

This is why patch 1's status in [FORK_CHANGES.md](FORK_CHANGES.md#verification-status) reads
*"verified by behaviour, numeric confirmation outstanding"* rather than claiming the counts.

---

## `Forcing Operation On Server` rising is expected

`Forcing Operation On Server` **rises** as a direct consequence of
[patch 2](FORK_CHANGES.md#2-a-client-rpc-bound-to-a-server-broadcast-runs-on-the-server-double-applying-operations).
Host-pawn ServerAuth operations now travel the forced drain **by design** — that is the mechanism of
the fix, not a side effect of it.

It is not a leak. A pre-patch log already sat at 1,186 occurrences.

---

## Log counts are only comparable between sessions of similar length

Related to the above, and worth stating separately because it caused a wasted investigation.

A short weapon-only session showed `Forcing Operation On Server` at **8**; a 31-minute multi-session
log showed **7,040**. Read side by side, that looks like a 900× regression. It is a 900× difference
in session length.

The same applies to `[AbilityCut]` and `[ReplayBurst]` counts, which appear in these sessions and are
**not** addressed by any patch in this fork. Do not read a `[ReplayBurst]` count against a
differently-sized run as a regression.

**Normalise by session duration, or compare only runs of comparable length.**
