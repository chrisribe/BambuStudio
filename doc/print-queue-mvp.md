# Bambu Local Print Queue (moved to separate repo)

The queue service prototype has been moved out of this BambuStudio fork to keep Studio changes minimal and reduce upstream merge friction.

## New home
- Repo: https://github.com/chrisribe/bambu-local-print-queue

## BambuStudio responsibility (thin adapter only)
This repository should only handle export-side integration points, for example:
- Export selected plate(s) into a batch folder.
- Emit a `manifest.json` that describes `project_id`, `plate_count`, and plate files.
- Optionally call external queue CLI (`bambu_queue.py ingest ...`) after export.

## Recommended export contract
```text
exports/
  my_project_2026-09-03/
    manifest.json
    plate_1.gcode.3mf
    plate_2.gcode.3mf
```

`manifest.json` schema:
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
