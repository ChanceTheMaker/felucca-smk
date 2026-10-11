# Felucca SMK repository instructions

Read [docs/UPSTREAM_WORKFLOW.md](docs/UPSTREAM_WORKFLOW.md) before importing
upstream changes, preparing contributions, changing firmware protocols, or
publishing a release. These are Salt's operating rules, not upstream policy.

- Our writable repository is `ChanceTheMaker/felucca-smk`. Treat
  `hugelton/Felucca` as read-only. In the legacy linked worktree, **origin points
  at upstream**; in the standalone SMK checkout it points at the SMK repository.
  Inspect remote URLs; never assume `origin` is the fork. Use an
  explicit fork URL for pushes unless a verified fork remote is configured.
- Fetch, compare, test, and prepare focused changes autonomously within the
  current task. Keep unrelated features in separate branches and PRs.
- Never submit the Salt branch wholesale upstream. Prepare each proposed
  contribution from current upstream main and include only that feature.
  Sending an upstream PR or message requires the owner's authorization for
  that contribution; this does not block local preparation or authorized fork PRs.
- Preserve upstream command IDs, storage migrations, author credits and licenses.
  Negotiate new protocol extensions; do not commandeer an existing command.
- Keep experimental Bluetooth work excluded unless the owner explicitly changes
  its scope. Do not copy build outputs or firmware from that worktree.
- Keep website publication separate from firmware release. A host-tested
  candidate does not replace the published firmware before hardware validation.
- Record the upstream revision, retained Salt differences, test evidence,
  skipped checks, and contribution status in the tracking documents. When
  upstream adopts a feature, reconcile our implementation and retire duplication.

The owner's explicit instructions in the current session take precedence.

Deferred requests are recorded in [docs/WORK_QUEUE.md](docs/WORK_QUEUE.md).
Do not silently include queued features in an unrelated active round.
