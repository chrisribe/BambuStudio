# Local Print Queue Tools

Prototype CLI/service for ingesting Bambu Studio multi-plate exports and managing a local queue.

## Quick start

```bash
python3 tools/print_queue/queue_service.py --help
python3 tools/print_queue/queue_service.py list --db ~/.bambu_queue/queue.db
```

## Suggested flow
1. Export plates + `manifest.json` into an inbox folder.
2. Run watch mode:
   - `python3 tools/print_queue/queue_service.py watch --inbox ~/bambu-queue-inbox`
3. Control queue with `list`, `next`, `complete`, `hold`, `resume`.

## Notes
- `next` currently marks job `sent` and prints a dispatch stub line.
- Integrate real printer dispatch in `dispatch_job_stub()`.
