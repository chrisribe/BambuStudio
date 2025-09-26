#ifndef PRINTHISTORYWIDGET_HPP
#define PRINTHISTORYWIDGET_HPP

#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/splitter.h>
#include <wx/button.h>
#include <wx/stattext.h>
#include <wx/searchctrl.h>
#include <memory>

#include "PrintHistoryManager.hpp"

class PrintHistoryWidget : public wxPanel {
public:
    PrintHistoryWidget(wxWindow* parent, PrintHistoryManager* manager);
    ~PrintHistoryWidget();
    
    void RefreshHistory();
    void SetManager(PrintHistoryManager* manager);
    
private:
    // Event handlers
    void OnExportCSV(wxCommandEvent& event);
    void OnExportJSON(wxCommandEvent& event);
    void OnRefresh(wxCommandEvent& event);
    void OnAddSampleData(wxCommandEvent& event);
    void OnItemSelected(wxListEvent& event);
    void OnItemRightClick(wxListEvent& event);
    void OnDeleteEntry(wxCommandEvent& event);
    void OnSearch(wxCommandEvent& event);
    void OnSearchCancel(wxCommandEvent& event);
    
    // Helper methods
    void SetupUI();
    void PopulateList();
    void UpdateDetailsPanel(const PrintHistoryEntry& entry);
    void ShowContextMenu(const wxPoint& pos);
    wxString FormatDuration(int seconds) const;
    
    // UI components
    wxSplitterWindow* m_splitter;
    wxPanel* m_list_panel;
    wxPanel* m_details_panel;
    wxListCtrl* m_list_ctrl;
    wxSearchCtrl* m_search_ctrl;
    wxButton* m_export_csv_btn;
    wxButton* m_export_json_btn;
    wxButton* m_refresh_btn;
    wxButton* m_add_sample_btn;
    
    // Details panel components
    wxStaticText* m_details_filename;
    wxStaticText* m_details_device;
    wxStaticText* m_details_time;
    wxStaticText* m_details_status;
    wxStaticText* m_details_duration;
    wxStaticText* m_details_notes;
    wxStaticText* m_details_gcode_path;
    
    PrintHistoryManager* m_manager;
    std::vector<PrintHistoryEntry> m_entries;
    int m_selected_entry_id;
    
    enum {
        ID_EXPORT_CSV = wxID_HIGHEST + 1,
        ID_EXPORT_JSON,
        ID_REFRESH,
        ID_ADD_SAMPLE_DATA,
        ID_DELETE_ENTRY,
        ID_SEARCH
    };
    
    wxDECLARE_EVENT_TABLE();
};

#endif // PRINTHISTORYWIDGET_HPP