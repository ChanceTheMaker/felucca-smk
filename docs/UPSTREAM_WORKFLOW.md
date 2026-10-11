# Working with upstream Felucca

These rules govern how **ChanceTheMaker/felucca-smk** receives updates from
**hugelton/Felucca** and offers selected improvements back. Keep Salt current,
preserve a small, understandable set of differences, and send upstream changes
that can be reviewed independently. This is our fork's policy; it does not claim
to be the upstream maintainers' contribution policy.

## Repository boundaries

| Repository or branch | Purpose | Write policy |
| --- | --- | --- |
| `hugelton/Felucca` | Upstream source and release reference | Fetch and inspect; no direct pushes |
| `ChanceTheMaker/felucca-smk` | SMK development, Studio and simulator | Push authorized SMK work here |
| `ChanceTheMaker/Felucca` | Separate FM-1 Salt project and existing upstream PR branches | Do not publish SMK work here |
| Fork `main` | Integrated, reviewable Salt source | PRs with linear history; no force-pushes |
| Fork `gh-pages` | Published website and selected firmware package | Publish the verified site; preserve the intended firmware version |
| `integrate/<version>-<feature>` or `feat/<version>-<feature>` | Incoming update and dependent Salt features | One feature per PR; document dependencies |
| `upstream/<feature>` in the fork | A selected contribution based on upstream main | Only the relevant change, tests, and documentation |

Legacy linked worktrees use **origin for upstream**. The standalone SMK clone
uses **origin for ChanceTheMaker/felucca-smk** and **upstream for hugelton/Felucca**.
The initial SMK migration creates `main` and preserves `feat/smk-studio-simulator`
at the same source checkpoint; later work uses focused PRs. Never use a bare `git push` or
assume a remote name identifies its owner. Check `git remote -v` and use explicit
repository URLs until names have been verified. Routine work never rewrites
either repository's main branch or release tags. Rebasing a feature branch is
allowed when needed; update it with an explicit `--force-with-lease` tied to the
observed remote commit, preserving other contributors' work.

## Receive an upstream update

1. Fetch upstream main and tags without merging into the working branch. Record
   the exact commit, tag, and comparison baseline in
   [UPSTREAM_STATUS.md](UPSTREAM_STATUS.md). Compare source, generated assets,
   build dependencies, licenses, package format, stored data, and web protocol.
2. Read upstream release notes, README, contribution instructions if present,
   and relevant issues or PRs. Check whether a Salt feature is already adopted,
   replaced, or incompatible. Do not treat similar names as equivalent behavior.
3. Create an isolated integration branch or worktree from the fork. Divide the
   work into a foundation PR and discrete dependent feature PRs. Pin the upstream
   revision throughout that round; review later upstream changes in another round.
4. Prefer merging upstream history when it cleanly preserves reviewability. If a
   source import or selective port is necessary, identify the source SHA and
   paths in the PR and tracking file. Do not invent merge ancestry or call a
   source import an upstream merge. Preserve copyright and license files.
5. Reapply only the Salt differences that remain useful. Resolve conflicts by
   behavior, not by choosing every file from one side. Preserve new upstream
   functionality and migrations while testing the retained Salt behavior.
6. Update the editor, installer, browser DSP and generated assets whenever their
   firmware interfaces change. Regenerate translations from their JSON catalogs;
   do not hand-merge the generated `locales.js`. Keep all eight languages usable.
7. Validate the combined source and build a distinctly numbered candidate.
   Update the tracking record with tests, limitations, artifact identity, and PRs.

For the current 1.0 round, upstream's SONG command **33** remains SONG and device
preferences/favorites use **34–38**. Detect capabilities from the connected
firmware and clear them on reconnect. The experimental Salt command-33 layout
was never flashed and requires no compatibility layer. That decision applies
only to that unshipped layout; preserve compatibility for versions actually used.

## Select changes to offer upstream

Good candidates solve an upstream problem with a bounded change: a reproducible
MIDI bug, protocol robustness, useful accessibility, or a separately testable
display enhancement. Prefer a small fix over an inseparable product redesign.

Salt branding, analytics identifiers and preferences, website presentation,
deployment configuration, and experimental Bluetooth work stay in the fork
unless a specific contribution is agreed. Check current upstream first: parts
of Salt's MIDI, palette and favorites work are already credited in v1.0.

For each selected feature:

1. Search upstream issues and PRs for duplicates or ongoing work. For a new
   protocol, storage change, or large design, prepare a short proposal before
   investing in an upstream-specific implementation.
2. Start a fresh branch at the **current upstream main**, not Salt main. Port or
   cherry-pick only the feature; remove Salt-specific dependencies, branding,
   telemetry and unrelated formatting. Follow the upstream style and tests.
3. Preserve original authorship with cherry-picks and `-x` where practical.
   For a substantial rewrite, credit the original author in the commit/PR.
   Preserve license notices and confirm any new asset can be redistributed.
4. Compare the complete branch to upstream main. Verify every changed path is
   part of this contribution and that it builds without the rest of Salt.
5. Prepare a concrete PR description: the trigger and before/after behavior,
   scope, compatibility and migration effects, test commands/results, and any
   untested hardware behavior. Link the original issue or contribution.
6. Push the branch **to our fork**. Opening an upstream PR or posting to upstream
   requires the owner's authorization for that contribution. If already given,
   proceed without asking again. Permission to sync or publish Salt does not by
   itself authorize messages or PRs in someone else's project. Never push
   directly to upstream main.
7. Respond to review with focused changes. Do not claim an upstream maintainer
   has accepted a proposal until it is accepted. Record the upstream PR and its
   state: proposed, submitted, changes requested, merged, declined, or superseded.

Once a contribution lands upstream, record its merge commit. On the next sync,
use the upstream implementation and remove the duplicate Salt patch where
behavior is equivalent. Re-run its regression test; keep any deliberate Salt
extension separate. Do not repeatedly resubmit a declined change unchanged.

## Validation and publication

Run tests appropriate to each feature, then the combined integration checks:
firmware build and host suite, golden sound renders, target budgets, MIDI/parser
and persistence coverage, editor protocol and updater tests, browser audio,
translations, and desktop/mobile interactions. Do not change golden baselines
merely to make a failure pass. Explain intended sound or budget changes.

Distinguish simulated-device tests, browser tests, and real FM-1 tests. Report
skipped dependencies and absent vendor images. Do not claim successful hardware
installation, physical MIDI reliability, or official recovery from mocks alone.

Website features can be merged and published independently while retaining the
known published firmware package. Candidate firmware has a separate version,
matching source archive, SHA-256 checksums, credits and license files. Hardware
validation comes before promoting it to the default installer. The current
round keeps Salt14 published while `1.0.1-salt1rc1` is validated. Experimental
Bluetooth code is excluded from both the candidate and the published build.

## Practical commands

These commands make repository ownership explicit. Inspect the working tree
first and substitute the chosen branch and reviewed upstream revision.

```sh
git status --short
git remote -v
git fetch https://github.com/hugelton/Felucca.git main:refs/remotes/upstream/main --tags
git fetch https://github.com/ChanceTheMaker/felucca-smk.git main:refs/remotes/smk/main
git log --oneline <previous-upstream-sha>..upstream/main
git diff --stat <previous-upstream-sha> upstream/main

# A contribution gets a clean upstream base in a new worktree.
git worktree add -b upstream/<feature> ../Felucca-upstream-<feature> upstream/main
# Port the selected feature there, test it, and review the full diff.
git diff upstream/main...HEAD
git push https://github.com/ChanceTheMaker/Felucca.git HEAD:refs/heads/upstream/<feature>
```

For an SMK PR, explicitly select `ChanceTheMaker/felucca-smk` and the intended base.
The upstream contribution push example above uses the original FM-1 fork only
for a separately authorized, clean upstream contribution, never the SMK branch.
For an authorized upstream PR, explicitly select `hugelton/Felucca`, base `main`,
and head `ChanceTheMaker:upstream/<feature>`. Prepare the description in a file
or structured tool argument so formatting and literal text are preserved.

## Keep the record current

Update [UPSTREAM_STATUS.md](UPSTREAM_STATUS.md) in each integration or contribution
PR. Record the baseline, new upstream revision, incoming features, retained Salt
differences, contribution links, test evidence, and release state. A later agent
should be able to determine what is upstream, what remains ours, and which
candidate is safe to publish without reconstructing a chat history.
