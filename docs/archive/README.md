# Archived documentation

Archived on 2026-10-18 while merging the two roadmaps. These documents are
historical references, not active task assignments or current agent
instructions. Their implementation notes and measured results are retained;
unresolved work is carried into the [current roadmap](../../ROADMAP.md).

| Document | Why archived / where to look now |
| --- | --- |
| [Engine roadmap](ROADMAP.md) | Replaced by the shared roadmap; contains completed Milestones 0–6 and the superseded dual-look renderer plan |
| [Black Plague/Requiem roadmap](PORTING-ROADMAP.md) | Merged into the shared roadmap; retains detailed porting checks and experiments |
| [Colour-space audit](colour-space-audit.md) | Pre-linear-lighting audit; the linear-only renderer has replaced its described pipeline |
| [GPU profiler, run 1](gpu-profiler-run1.md) | 2026-10-02 timings before later AO/renderer changes; useful historical measurements, not a current baseline |
| [AO review](codex-ao-review.md) | Review preceding the implemented AO optimization pass; remaining profiling candidates are in the roadmap |
| [Legacy engine review](legacy-engine-review.md) | Findings addressed by the subsequent engine fixes; remaining manual checks and profiling are in the roadmap |
| [Legacy engine fixes](legacy-engine-fixes.md) | Completed branch report and validation history; current testing commands are in the repository README and AGENTS |
| [Milestone 6 step 9 handoff](milestone6-step9-handoff.md) | Backend split completed; old worktree and assignment are obsolete |
| [Renderer cleanup plan](renderer-cleanup-plan.md) | Step 11 implemented; retains state/ordering decisions and final validation limits |

The snapshots retain historical wording, including task instructions. Do not
execute those instructions as new work. Build directories, `/tmp` logs,
Windows kit locations and experiment scripts mentioned there may be absent.
Current behaviour and commands must be checked against the source tree.
