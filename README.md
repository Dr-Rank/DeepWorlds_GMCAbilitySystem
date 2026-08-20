> ## This is a fork
>
> A fork of [DeepWorlds' GMAS](https://github.com/DeepWorldsSA/DeepWorlds_GMCAbilitySystem) `dev`
> branch, itself a fork of [reznok/GMCAbilitySystem](https://github.com/reznok/GMCAbilitySystem).
> It carries **four patches** on top of upstream — three networking fixes found while integrating
> GMAS into a listen-server + client project, and one build fix.
>
> | | |
> |---|---|
> | **Upstream baseline** | [`7d836b20`](https://github.com/DeepWorldsSA/DeepWorlds_GMCAbilitySystem/commit/7d836b20d7a0c5ca87244bd300a33d6847b75cd6) on DeepWorlds `dev` (26 June 2026) |
> | **Engine** | UE **5.5+**. Built and run on 5.8. |
> | **What changed, and why** | **[Docs/FORK_CHANGES.md](Docs/FORK_CHANGES.md)** |
> | **What is knowingly still broken** | **[Docs/KNOWN_ISSUES.md](Docs/KNOWN_ISSUES.md)** |
>
> The four patches, most to least severe:
>
> 1. **`bPreventConcurrentActivation` was declared, documented, Blueprint-exposed — and never read.**
>    Callers who ticked the box got nothing. On a client this starves the operation queue: a
>    hold-to-sprint input measured 294 activations in 3.4 s, during which a reload never processed.
> 2. **A Client RPC bound to a server-side broadcast runs on the listen-server host**, double-applying
>    operations. An exactly-once violation that presents only as log noise.
> 3. **The operation-payload cache never prunes on a server**, and leaks an entry for every consumed
>    payload.
> 4. **`InstancedStruct` include paths** updated for UE 5.5+, where `StructUtils` moved into
>    `CoreUObject`.
>
> Each is a separate commit, with the full reasoning in the commit body, so upstream can take them
> piecemeal. Every patch in `FORK_CHANGES.md` also carries an *"is it fixed upstream yet?"* test, so
> a future vendor drop can drop the hunks that are no longer needed.
>
> Issues 1–3 have **not** been reported upstream yet.
>
> Everything below this line is upstream's README, unchanged.

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
