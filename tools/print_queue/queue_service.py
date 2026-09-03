#!/usr/bin/env python3
import argparse
import json
import sqlite3
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

SCHEMA = """
CREATE TABLE IF NOT EXISTS jobs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    project_id TEXT NOT NULL,
    project_name TEXT NOT NULL,
    plate_index INTEGER NOT NULL,
    plate_count INTEGER NOT NULL,
    gcode_path TEXT NOT NULL,
    est_minutes INTEGER,
    filament TEXT,
    status TEXT NOT NULL DEFAULT 'queued',
    created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(project_id, plate_index)
);

CREATE INDEX IF NOT EXISTS idx_jobs_status_id ON jobs(status, id);
"""


@dataclass
class Plate:
    plate_index: int
    gcode_path: str
    est_minutes: int | None = None
    filament: str | None = None


class QueueDB:
    def __init__(self, db_path: Path):
        self.db_path = db_path
        self.db_path.parent.mkdir(parents=True, exist_ok=True)
        self.conn = sqlite3.connect(self.db_path)
        self.conn.row_factory = sqlite3.Row
        self.conn.executescript(SCHEMA)

    def close(self):
        self.conn.close()

    def ingest_batch(self, project_id: str, project_name: str, plate_count: int, plates: Iterable[Plate]) -> int:
        inserted = 0
        with self.conn:
            for p in plates:
                cur = self.conn.execute(
                    """
                    INSERT OR IGNORE INTO jobs(project_id, project_name, plate_index, plate_count, gcode_path, est_minutes, filament, status)
                    VALUES(?, ?, ?, ?, ?, ?, ?, 'queued')
                    """,
                    (project_id, project_name, p.plate_index, plate_count, p.gcode_path, p.est_minutes, p.filament),
                )
                inserted += cur.rowcount
        return inserted

    def list_jobs(self):
        return self.conn.execute(
            "SELECT id, project_name, project_id, plate_index, plate_count, gcode_path, status, est_minutes, filament FROM jobs ORDER BY id ASC"
        ).fetchall()

    def next_job(self):
        row = self.conn.execute(
            "SELECT * FROM jobs WHERE status='queued' ORDER BY id ASC LIMIT 1"
        ).fetchone()
        if not row:
            return None
        with self.conn:
            self.conn.execute(
                "UPDATE jobs SET status='sent', updated_at=CURRENT_TIMESTAMP WHERE id=?",
                (row["id"],),
            )
        return self.conn.execute("SELECT * FROM jobs WHERE id=?", (row["id"],)).fetchone()

    def update_status(self, job_id: int, status: str) -> bool:
        with self.conn:
            cur = self.conn.execute(
                "UPDATE jobs SET status=?, updated_at=CURRENT_TIMESTAMP WHERE id=?",
                (status, job_id),
            )
        return cur.rowcount > 0


def parse_manifest(manifest_path: Path):
    data = json.loads(manifest_path.read_text())
    required = ["project_name", "project_id", "plate_count", "plates"]
    missing = [k for k in required if k not in data]
    if missing:
        raise ValueError(f"Manifest missing keys: {', '.join(missing)}")

    base = manifest_path.parent
    plates = []
    for p in data["plates"]:
        abs_path = (base / p["gcode_path"]).resolve()
        plates.append(
            Plate(
                plate_index=int(p["plate_index"]),
                gcode_path=str(abs_path),
                est_minutes=int(p["est_minutes"]) if p.get("est_minutes") is not None else None,
                filament=p.get("filament"),
            )
        )

    return data["project_id"], data["project_name"], int(data["plate_count"]), plates


def dispatch_job_stub(row: sqlite3.Row):
    print(
        f"DISPATCH_STUB job={row['id']} project={row['project_name']} "
        f"plate={row['plate_index']}/{row['plate_count']} gcode={row['gcode_path']}"
    )


def cmd_ingest(args):
    db = QueueDB(Path(args.db))
    try:
        project_id, project_name, plate_count, plates = parse_manifest(Path(args.manifest))
        added = db.ingest_batch(project_id, project_name, plate_count, plates)
        print(f"ingested={added} project_id={project_id}")
    finally:
        db.close()


def cmd_watch(args):
    inbox = Path(args.inbox).expanduser().resolve()
    processed = inbox / ".processed"
    processed.mkdir(parents=True, exist_ok=True)

    db = QueueDB(Path(args.db))
    print(f"watching={inbox}")
    try:
        while True:
            manifests = sorted(inbox.glob("**/manifest.json"))
            for manifest in manifests:
                stamp = processed / (manifest.parent.name + ".done")
                if stamp.exists():
                    continue
                try:
                    project_id, project_name, plate_count, plates = parse_manifest(manifest)
                    added = db.ingest_batch(project_id, project_name, plate_count, plates)
                    stamp.write_text(f"project_id={project_id} added={added}\n")
                    print(f"ingested={added} manifest={manifest}")
                except Exception as e:
                    print(f"error ingesting {manifest}: {e}")
            time.sleep(args.poll_seconds)
    finally:
        db.close()


def cmd_list(args):
    db = QueueDB(Path(args.db))
    try:
        rows = db.list_jobs()
        if not rows:
            print("queue is empty")
            return
        for r in rows:
            print(
                f"id={r['id']} status={r['status']} project={r['project_name']} "
                f"plate={r['plate_index']}/{r['plate_count']} est={r['est_minutes']} filament={r['filament']}"
            )
    finally:
        db.close()


def cmd_next(args):
    db = QueueDB(Path(args.db))
    try:
        row = db.next_job()
        if not row:
            print("no queued jobs")
            return
        dispatch_job_stub(row)
    finally:
        db.close()


def cmd_status_update(args, status: str):
    db = QueueDB(Path(args.db))
    try:
        ok = db.update_status(args.job_id, status)
        print(f"updated={ok} id={args.job_id} status={status}")
    finally:
        db.close()


def add_db_arg(p):
    p.add_argument("--db", default="~/.bambu_queue/queue.db", help="SQLite db path")


def build_parser():
    parser = argparse.ArgumentParser(description="Local print queue prototype")
    sub = parser.add_subparsers(dest="cmd", required=True)

    p_ingest = sub.add_parser("ingest", help="Ingest a manifest.json batch")
    add_db_arg(p_ingest)
    p_ingest.add_argument("--manifest", required=True)
    p_ingest.set_defaults(func=cmd_ingest)

    p_watch = sub.add_parser("watch", help="Watch inbox for manifest.json")
    add_db_arg(p_watch)
    p_watch.add_argument("--inbox", required=True)
    p_watch.add_argument("--poll-seconds", type=int, default=8)
    p_watch.set_defaults(func=cmd_watch)

    p_list = sub.add_parser("list", help="List queue jobs")
    add_db_arg(p_list)
    p_list.set_defaults(func=cmd_list)

    p_next = sub.add_parser("next", help="Take next queued job and mark as sent")
    add_db_arg(p_next)
    p_next.set_defaults(func=cmd_next)

    p_complete = sub.add_parser("complete", help="Mark a job done")
    add_db_arg(p_complete)
    p_complete.add_argument("--job-id", type=int, required=True)
    p_complete.set_defaults(func=lambda a: cmd_status_update(a, "done"))

    p_hold = sub.add_parser("hold", help="Hold a job")
    add_db_arg(p_hold)
    p_hold.add_argument("--job-id", type=int, required=True)
    p_hold.set_defaults(func=lambda a: cmd_status_update(a, "held"))

    p_resume = sub.add_parser("resume", help="Resume a held job")
    add_db_arg(p_resume)
    p_resume.add_argument("--job-id", type=int, required=True)
    p_resume.set_defaults(func=lambda a: cmd_status_update(a, "queued"))

    return parser


def main():
    parser = build_parser()
    args = parser.parse_args()
    args.db = str(Path(args.db).expanduser().resolve())
    args.func(args)


if __name__ == "__main__":
    main()
