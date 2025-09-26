#include "PrintHistoryManager.hpp"
#include <wx/stdpaths.h>
#include <wx/log.h>
#include <wx/textfile.h>
#include <wx/stream.h>
#include <wx/wfstream.h>
#include <sqlite3.h>

PrintHistoryManager::PrintHistoryManager() : m_db(nullptr) {
    m_db_path = GetDatabasePath();
}

PrintHistoryManager::~PrintHistoryManager() {
    if (m_db) {
        sqlite3_close(m_db);
    }
}

bool PrintHistoryManager::Initialize() {
    int rc = sqlite3_open(m_db_path.mb_str(), &m_db);
    if (rc != SQLITE_OK) {
        wxLogError("Cannot open database: %s", sqlite3_errmsg(m_db));
        return false;
    }
    
    return CreateTables();
}

wxString PrintHistoryManager::GetDatabasePath() const {
    wxStandardPaths& paths = wxStandardPaths::Get();
    wxString app_data_dir = paths.GetUserDataDir();
    
    // Ensure directory exists
    if (!wxFileName::DirExists(app_data_dir)) {
        wxFileName::Mkdir(app_data_dir, wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
    }
    
    return wxFileName(app_data_dir, "print_history.db").GetFullPath();
}

bool PrintHistoryManager::CreateTables() {
    const char* create_table_sql = R"(
        CREATE TABLE IF NOT EXISTS print_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            filename TEXT NOT NULL,
            device_name TEXT,
            print_time DATETIME NOT NULL,
            status TEXT NOT NULL,
            duration_seconds INTEGER DEFAULT 0,
            user_notes TEXT,
            gcode_path TEXT,
            cover_image_path TEXT,
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP
        );
        
        CREATE INDEX IF NOT EXISTS idx_print_time ON print_history(print_time DESC);
        CREATE INDEX IF NOT EXISTS idx_status ON print_history(status);
    )";
    
    char* err_msg = nullptr;
    int rc = sqlite3_exec(m_db, create_table_sql, nullptr, nullptr, &err_msg);
    
    if (rc != SQLITE_OK) {
        wxLogError("SQL error: %s", err_msg);
        sqlite3_free(err_msg);
        return false;
    }
    
    return true;
}

bool PrintHistoryManager::AddPrintJob(const PrintHistoryEntry& entry) {
    if (!m_db) return false;
    
    const char* insert_sql = R"(
        INSERT INTO print_history 
        (filename, device_name, print_time, status, duration_seconds, user_notes, gcode_path, cover_image_path)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, insert_sql, -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        wxLogError("Failed to prepare statement: %s", sqlite3_errmsg(m_db));
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, entry.filename.mb_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, entry.device_name.mb_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, entry.print_time.Format("%Y-%m-%d %H:%M:%S").mb_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, entry.status.mb_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, entry.duration_seconds);
    sqlite3_bind_text(stmt, 6, entry.user_notes.mb_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, entry.gcode_path.mb_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, entry.cover_image_path.mb_str(), -1, SQLITE_STATIC);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (rc != SQLITE_DONE) {
        wxLogError("Failed to insert print job: %s", sqlite3_errmsg(m_db));
        return false;
    }
    
    return true;
}

std::vector<PrintHistoryEntry> PrintHistoryManager::GetPrintHistory(int limit) const {
    std::vector<PrintHistoryEntry> entries;
    if (!m_db) return entries;
    
    wxString query = "SELECT id, filename, device_name, print_time, status, duration_seconds, user_notes, gcode_path, cover_image_path FROM print_history ORDER BY print_time DESC";
    if (limit > 0) {
        query += wxString::Format(" LIMIT %d", limit);
    }
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, query.mb_str(), -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        wxLogError("Failed to prepare select statement: %s", sqlite3_errmsg(m_db));
        return entries;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        PrintHistoryEntry entry;
        entry.id = sqlite3_column_int(stmt, 0);
        entry.filename = wxString::FromUTF8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
        entry.device_name = wxString::FromUTF8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2)));
        
        const char* time_str = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        if (time_str) {
            entry.print_time.ParseFormat(time_str, "%Y-%m-%d %H:%M:%S");
        }
        
        entry.status = wxString::FromUTF8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)));
        entry.duration_seconds = sqlite3_column_int(stmt, 5);
        entry.user_notes = wxString::FromUTF8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
        entry.gcode_path = wxString::FromUTF8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7)));
        entry.cover_image_path = wxString::FromUTF8(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8)));
        
        entries.push_back(entry);
    }
    
    sqlite3_finalize(stmt);
    return entries;
}

bool PrintHistoryManager::ExportToCSV(const wxString& filename) const {
    auto entries = GetPrintHistory();
    
    wxTextFile file(filename);
    if (file.Exists()) {
        file.Open();
        file.Clear();
    } else {
        file.Create();
    }
    
    // CSV Header
    file.AddLine("ID,Filename,Device Name,Print Time,Status,Duration (seconds),User Notes,GCode Path,Cover Image Path");
    
    for (const auto& entry : entries) {
        wxString line = wxString::Format("%d,\"%s\",\"%s\",\"%s\",\"%s\",%d,\"%s\",\"%s\",\"%s\"",
            entry.id,
            entry.filename,
            entry.device_name,
            entry.print_time.Format("%Y-%m-%d %H:%M:%S"),
            entry.status,
            entry.duration_seconds,
            entry.user_notes,
            entry.gcode_path,
            entry.cover_image_path
        );
        file.AddLine(line);
    }
    
    return file.Write();
}

bool PrintHistoryManager::ExportToJSON(const wxString& filename) const {
    auto entries = GetPrintHistory();
    
    wxString json_content = "{\n  \"print_history\": [\n";
    
    for (size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        json_content += wxString::Format(
            "    {\n"
            "      \"id\": %d,\n"
            "      \"filename\": \"%s\",\n"
            "      \"device_name\": \"%s\",\n"
            "      \"print_time\": \"%s\",\n"
            "      \"status\": \"%s\",\n"
            "      \"duration_seconds\": %d,\n"
            "      \"user_notes\": \"%s\",\n"
            "      \"gcode_path\": \"%s\",\n"
            "      \"cover_image_path\": \"%s\"\n"
            "    }%s\n",
            entry.id,
            entry.filename,
            entry.device_name,
            entry.print_time.Format("%Y-%m-%d %H:%M:%S"),
            entry.status,
            entry.duration_seconds,
            entry.user_notes,
            entry.gcode_path,
            entry.cover_image_path,
            (i < entries.size() - 1) ? "," : ""
        );
    }
    
    json_content += "  ],\n";
    json_content += wxString::Format("  \"export_time\": \"%s\"\n", wxDateTime::Now().Format("%Y-%m-%d %H:%M:%S"));
    json_content += "}\n";
    
    wxTextFile file(filename);
    if (file.Exists()) {
        file.Open();
        file.Clear();
    } else {
        file.Create();
    }
    
    file.AddLine(json_content);
    return file.Write();
}

bool PrintHistoryManager::DeleteEntry(int id) {
    if (!m_db) return false;
    
    const char* delete_sql = "DELETE FROM print_history WHERE id = ?";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, delete_sql, -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        wxLogError("Failed to prepare delete statement: %s", sqlite3_errmsg(m_db));
        return false;
    }
    
    sqlite3_bind_int(stmt, 1, id);
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}

bool PrintHistoryManager::UpdateEntry(const PrintHistoryEntry& entry) {
    if (!m_db || entry.id < 0) return false;
    
    const char* update_sql = R"(
        UPDATE print_history 
        SET filename = ?, device_name = ?, print_time = ?, status = ?, 
            duration_seconds = ?, user_notes = ?, gcode_path = ?, cover_image_path = ?
        WHERE id = ?
    )";
    
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, update_sql, -1, &stmt, nullptr);
    
    if (rc != SQLITE_OK) {
        wxLogError("Failed to prepare update statement: %s", sqlite3_errmsg(m_db));
        return false;
    }
    
    sqlite3_bind_text(stmt, 1, entry.filename.mb_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, entry.device_name.mb_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, entry.print_time.Format("%Y-%m-%d %H:%M:%S").mb_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 4, entry.status.mb_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 5, entry.duration_seconds);
    sqlite3_bind_text(stmt, 6, entry.user_notes.mb_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 7, entry.gcode_path.mb_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 8, entry.cover_image_path.mb_str(), -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 9, entry.id);
    
    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    return rc == SQLITE_DONE;
}