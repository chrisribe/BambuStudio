# Bambu Local Print Queue (MVP)

## Goal
Queue multi-plate exports from Bambu Studio so prints can be dispatched in order later, with manual confirmation between plates.

## Scope (MVP)
- Ingest exported plate jobs from a watched folder.
- Group jobs into a project batch (`project_id`).
- Persist queue state in SQLite.
- CLI controls: list, next, complete, hold, resume.
- Optional watch mode for automatic ingest.

## Non-goals (MVP)
- Deep integration in Bambu Studio UI.
- Fully automatic unattended chaining without user confirmation.
- Printer API upload/dispatch (stubbed for now).

## Ingest Contract
Preferred export layout from Studio-side adapter:

```text
exports/
  my_project_2026-09-03/
    manifest.json
    plate_1.gcode.3mf
    plate_2.gcode.3mf
```

`manifest.json` example:

```json
{
  "project_name": "my_project",
  "project_id": "my_project-2026-09-03T22-00-12",
  "plate_count": 2,
  "plates": [
    {"plate_index": 1, "gcode_path": "plate_1.gcode.3mf", "est_minutes": 350, "filament": "PLA Basic"},
    {"plate_index": 2, "gcode_path": "plate_2.gcode.3mf", "est_minutes": 290, "filament": "PLA Basic"}
  ]
}
```

## Queue States
- `queued`
- `held`
- `sent`
- `printing`
- `done`
- `failed`

MVP transitions:
- ingest -> `queued`
- `next` picks first `queued` and marks `sent`
- `complete <job_id>` marks `done`
- `hold <job_id>` / `resume <job_id>` toggles held/queued

## CLI surface
- `watch --inbox <dir> --db <path>`
- `ingest --manifest <manifest.json> --db <path>`
- `list --db <path>`
- `next --db <path>`
- `complete --db <path> --job-id <id>`
- `hold --db <path> --job-id <id>`
- `resume --db <path> --job-id <id>`

## Next steps after MVP
1. Add dispatch adapter to X1 (LAN auth/upload/print start).
2. Add Discord/Hermes bridge (`/queue next`, `/queue status`).
3. Add safety checks (AMS, clear-bed acknowledgment, printer error gate).
4. Add retries, reorder, and per-project pause.
