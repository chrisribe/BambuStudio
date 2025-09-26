#ifndef PRINTHISTORYMANAGER_HPP
#define PRINTHISTORYMANAGER_HPP

#include <wx/wx.h>
#include <wx/filename.h>
#include <vector>
#include <memory>

struct PrintHistoryEntry {
    int id;
    wxString filename;
    wxString device_name;
    wxDateTime print_time;
    wxString status;
    int duration_seconds;
    wxString user_notes;
    wxString gcode_path;
    wxString cover_image_path;
    
    PrintHistoryEntry() : id(-1), duration_seconds(0) {}
};

class PrintHistoryManager {
public:
    PrintHistoryManager();
    ~PrintHistoryManager();
    
    bool Initialize();
    bool AddPrintJob(const PrintHistoryEntry& entry);
    std::vector<PrintHistoryEntry> GetPrintHistory(int limit = 0) const;
    bool ExportToCSV(const wxString& filename) const;
    bool ExportToJSON(const wxString& filename) const;
    bool DeleteEntry(int id);
    bool UpdateEntry(const PrintHistoryEntry& entry);
    
    // For testing/demonstration
    bool AddSampleData();
    
    // Statistics and debugging
    int GetEntryCount() const;
    wxString GetDatabaseInfo() const;
    
private:
    struct sqlite3* m_db;
    wxString m_db_path;
    
    bool CreateTables();
    wxString GetDatabasePath() const;
    bool ExecuteSQL(const wxString& sql) const;
};

#endif // PRINTHISTORYMANAGER_HPP