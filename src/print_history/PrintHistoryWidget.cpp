#include "PrintHistoryWidget.hpp"
#include <wx/filedlg.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/menu.h>
#include <wx/log.h>

wxBEGIN_EVENT_TABLE(PrintHistoryWidget, wxPanel)
    EVT_BUTTON(ID_EXPORT_CSV, PrintHistoryWidget::OnExportCSV)
    EVT_BUTTON(ID_EXPORT_JSON, PrintHistoryWidget::OnExportJSON)
    EVT_BUTTON(ID_REFRESH, PrintHistoryWidget::OnRefresh)
    EVT_BUTTON(ID_ADD_SAMPLE_DATA, PrintHistoryWidget::OnAddSampleData)
    EVT_LIST_ITEM_SELECTED(wxID_ANY, PrintHistoryWidget::OnItemSelected)
    EVT_LIST_ITEM_RIGHT_CLICK(wxID_ANY, PrintHistoryWidget::OnItemRightClick)
    EVT_MENU(ID_DELETE_ENTRY, PrintHistoryWidget::OnDeleteEntry)
    EVT_SEARCHCTRL_SEARCH_BTN(ID_SEARCH, PrintHistoryWidget::OnSearch)
    EVT_SEARCHCTRL_CANCEL_BTN(ID_SEARCH, PrintHistoryWidget::OnSearchCancel)
wxEND_EVENT_TABLE()

PrintHistoryWidget::PrintHistoryWidget(wxWindow* parent, PrintHistoryManager* manager)
    : wxPanel(parent, wxID_ANY), m_manager(manager), m_selected_entry_id(-1) {
    SetupUI();
    RefreshHistory();
}

PrintHistoryWidget::~PrintHistoryWidget() = default;

void PrintHistoryWidget::SetupUI() {
    // Main sizer
    wxBoxSizer* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Top toolbar
    wxBoxSizer* toolbar_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    m_search_ctrl = new wxSearchCtrl(this, ID_SEARCH, wxEmptyString, wxDefaultPosition, wxSize(200, -1));
    m_search_ctrl->SetDescriptiveText("Search print history...");
    
    m_refresh_btn = new wxButton(this, ID_REFRESH, "Refresh");
    m_add_sample_btn = new wxButton(this, ID_ADD_SAMPLE_DATA, "Add Sample Data");
    m_export_csv_btn = new wxButton(this, ID_EXPORT_CSV, "Export CSV");
    m_export_json_btn = new wxButton(this, ID_EXPORT_JSON, "Export JSON");
    
    toolbar_sizer->Add(m_search_ctrl, 1, wxALL | wxEXPAND, 5);
    toolbar_sizer->Add(m_refresh_btn, 0, wxALL, 5);
    toolbar_sizer->Add(m_add_sample_btn, 0, wxALL, 5);
    toolbar_sizer->Add(m_export_csv_btn, 0, wxALL, 5);
    toolbar_sizer->Add(m_export_json_btn, 0, wxALL, 5);
    
    main_sizer->Add(toolbar_sizer, 0, wxEXPAND | wxALL, 5);
    
    // Splitter window
    m_splitter = new wxSplitterWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 
                                      wxSP_3D | wxSP_LIVE_UPDATE);
    
    // List panel
    m_list_panel = new wxPanel(m_splitter, wxID_ANY);
    wxBoxSizer* list_sizer = new wxBoxSizer(wxVERTICAL);
    
    m_list_ctrl = new wxListCtrl(m_list_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                 wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES | wxLC_VRULES);
    
    // Set up columns
    m_list_ctrl->AppendColumn("Filename", wxLIST_FORMAT_LEFT, 200);
    m_list_ctrl->AppendColumn("Device", wxLIST_FORMAT_LEFT, 150);
    m_list_ctrl->AppendColumn("Date/Time", wxLIST_FORMAT_LEFT, 150);
    m_list_ctrl->AppendColumn("Status", wxLIST_FORMAT_CENTER, 100);
    m_list_ctrl->AppendColumn("Duration", wxLIST_FORMAT_CENTER, 100);
    
    list_sizer->Add(m_list_ctrl, 1, wxEXPAND | wxALL, 5);
    m_list_panel->SetSizer(list_sizer);
    
    // Details panel
    m_details_panel = new wxPanel(m_splitter, wxID_ANY);
    SetupDetailsPanel();
    
    // Configure splitter
    m_splitter->SplitVertically(m_list_panel, m_details_panel, 600);
    m_splitter->SetMinimumPaneSize(300);
    
    main_sizer->Add(m_splitter, 1, wxEXPAND | wxALL, 5);
    
    SetSizer(main_sizer);
}

void PrintHistoryWidget::SetupDetailsPanel() {
    wxBoxSizer* details_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Title
    wxStaticText* title = new wxStaticText(m_details_panel, wxID_ANY, "Print Job Details");
    wxFont title_font = title->GetFont();
    title_font.SetWeight(wxFONTWEIGHT_BOLD);
    title_font.SetPointSize(title_font.GetPointSize() + 2);
    title->SetFont(title_font);
    
    details_sizer->Add(title, 0, wxALL | wxEXPAND, 10);
    details_sizer->Add(new wxStaticLine(m_details_panel), 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
    
    // Details fields
    wxFlexGridSizer* grid_sizer = new wxFlexGridSizer(7, 2, 10, 10);
    grid_sizer->AddGrowableCol(1);
    
    // Filename
    grid_sizer->Add(new wxStaticText(m_details_panel, wxID_ANY, "Filename:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_details_filename = new wxStaticText(m_details_panel, wxID_ANY, "-");
    grid_sizer->Add(m_details_filename, 1, wxEXPAND);
    
    // Device
    grid_sizer->Add(new wxStaticText(m_details_panel, wxID_ANY, "Device:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_details_device = new wxStaticText(m_details_panel, wxID_ANY, "-");
    grid_sizer->Add(m_details_device, 1, wxEXPAND);
    
    // Print Time
    grid_sizer->Add(new wxStaticText(m_details_panel, wxID_ANY, "Print Time:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_details_time = new wxStaticText(m_details_panel, wxID_ANY, "-");
    grid_sizer->Add(m_details_time, 1, wxEXPAND);
    
    // Status
    grid_sizer->Add(new wxStaticText(m_details_panel, wxID_ANY, "Status:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_details_status = new wxStaticText(m_details_panel, wxID_ANY, "-");
    grid_sizer->Add(m_details_status, 1, wxEXPAND);
    
    // Duration
    grid_sizer->Add(new wxStaticText(m_details_panel, wxID_ANY, "Duration:"), 0, wxALIGN_RIGHT | wxALIGN_CENTER_VERTICAL);
    m_details_duration = new wxStaticText(m_details_panel, wxID_ANY, "-");
    grid_sizer->Add(m_details_duration, 1, wxEXPAND);
    
    // Notes
    grid_sizer->Add(new wxStaticText(m_details_panel, wxID_ANY, "Notes:"), 0, wxALIGN_RIGHT | wxALIGN_TOP);
    m_details_notes = new wxStaticText(m_details_panel, wxID_ANY, "-");
    m_details_notes->Wrap(200);
    grid_sizer->Add(m_details_notes, 1, wxEXPAND);
    
    // GCode Path
    grid_sizer->Add(new wxStaticText(m_details_panel, wxID_ANY, "GCode Path:"), 0, wxALIGN_RIGHT | wxALIGN_TOP);
    m_details_gcode_path = new wxStaticText(m_details_panel, wxID_ANY, "-");
    m_details_gcode_path->Wrap(200);
    grid_sizer->Add(m_details_gcode_path, 1, wxEXPAND);
    
    details_sizer->Add(grid_sizer, 0, wxALL | wxEXPAND, 10);
    
    m_details_panel->SetSizer(details_sizer);
}

void PrintHistoryWidget::RefreshHistory() {
    if (!m_manager) return;
    
    m_entries = m_manager->GetPrintHistory();
    PopulateList();
    
    // If no entries, suggest adding sample data for testing
    if (m_entries.empty()) {
        wxLogInfo("Print history is empty. You can add sample data for testing.");
        // Optional: Show a message in the details panel
        if (m_details_filename) {
            m_details_filename->SetLabel("No print history entries found");
            m_details_device->SetLabel("Click 'Add Sample Data' to see demo entries");
            m_details_time->SetLabel("");
            m_details_status->SetLabel("");
            m_details_duration->SetLabel("");
            m_details_notes->SetLabel("");
            m_details_gcode_path->SetLabel("");
            m_details_panel->Layout();
        }
    }
}

void PrintHistoryWidget::PopulateList() {
    m_list_ctrl->DeleteAllItems();
    
    for (size_t i = 0; i < m_entries.size(); ++i) {
        const auto& entry = m_entries[i];
        
        long item_index = m_list_ctrl->InsertItem(i, entry.filename);
        m_list_ctrl->SetItem(item_index, 1, entry.device_name);
        m_list_ctrl->SetItem(item_index, 2, entry.print_time.Format("%Y-%m-%d %H:%M"));
        m_list_ctrl->SetItem(item_index, 3, entry.status);
        m_list_ctrl->SetItem(item_index, 4, FormatDuration(entry.duration_seconds));
        
        m_list_ctrl->SetItemData(item_index, entry.id);
    }
}

void PrintHistoryWidget::UpdateDetailsPanel(const PrintHistoryEntry& entry) {
    m_details_filename->SetLabel(entry.filename);
    m_details_device->SetLabel(entry.device_name);
    m_details_time->SetLabel(entry.print_time.Format("%Y-%m-%d %H:%M:%S"));
    m_details_status->SetLabel(entry.status);
    m_details_duration->SetLabel(FormatDuration(entry.duration_seconds));
    m_details_notes->SetLabel(entry.user_notes.IsEmpty() ? "No notes" : entry.user_notes);
    m_details_gcode_path->SetLabel(entry.gcode_path.IsEmpty() ? "No path" : entry.gcode_path);
    
    m_details_panel->Layout();
}

wxString PrintHistoryWidget::FormatDuration(int seconds) const {
    if (seconds < 60) {
        return wxString::Format("%ds", seconds);
    } else if (seconds < 3600) {
        return wxString::Format("%dm %ds", seconds / 60, seconds % 60);
    } else {
        int hours = seconds / 3600;
        int mins = (seconds % 3600) / 60;
        int secs = seconds % 60;
        return wxString::Format("%dh %dm %ds", hours, mins, secs);
    }
}

void PrintHistoryWidget::SetManager(PrintHistoryManager* manager) {
    m_manager = manager;
    RefreshHistory();
}

void PrintHistoryWidget::OnExportCSV(wxCommandEvent& event) {
    wxFileDialog dialog(this, "Export Print History to CSV", "", "print_history.csv", 
                        "CSV files (*.csv)|*.csv", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    if (dialog.ShowModal() == wxID_OK && m_manager) {
        if (m_manager->ExportToCSV(dialog.GetPath())) {
            wxMessageBox("Print history exported successfully!", "Export Complete", wxOK | wxICON_INFORMATION);
        } else {
            wxMessageBox("Failed to export print history.", "Export Error", wxOK | wxICON_ERROR);
        }
    }
}

void PrintHistoryWidget::OnExportJSON(wxCommandEvent& event) {
    wxFileDialog dialog(this, "Export Print History to JSON", "", "print_history.json", 
                        "JSON files (*.json)|*.json", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    if (dialog.ShowModal() == wxID_OK && m_manager) {
        if (m_manager->ExportToJSON(dialog.GetPath())) {
            wxMessageBox("Print history exported successfully!", "Export Complete", wxOK | wxICON_INFORMATION);
        } else {
            wxMessageBox("Failed to export print history.", "Export Error", wxOK | wxICON_ERROR);
        }
    }
}

void PrintHistoryWidget::OnRefresh(wxCommandEvent& event) {
    RefreshHistory();
}

void PrintHistoryWidget::OnAddSampleData(wxCommandEvent& event) {
    if (!m_manager) {
        wxMessageBox("Print history manager not available.", "Error", wxOK | wxICON_ERROR);
        return;
    }
    
    if (m_manager->AddSampleData()) {
        wxMessageBox("Sample data added successfully!", "Success", wxOK | wxICON_INFORMATION);
        RefreshHistory();
    } else {
        wxMessageBox("Failed to add sample data.", "Error", wxOK | wxICON_ERROR);
    }
}

void PrintHistoryWidget::OnItemSelected(wxListEvent& event) {
    long item_index = event.GetIndex();
    if (item_index >= 0 && item_index < static_cast<long>(m_entries.size())) {
        m_selected_entry_id = m_list_ctrl->GetItemData(item_index);
        
        // Find the entry by ID
        for (const auto& entry : m_entries) {
            if (entry.id == m_selected_entry_id) {
                UpdateDetailsPanel(entry);
                break;
            }
        }
    }
}

void PrintHistoryWidget::OnItemRightClick(wxListEvent& event) {
    m_selected_entry_id = m_list_ctrl->GetItemData(event.GetIndex());
    ShowContextMenu(event.GetPoint());
}

void PrintHistoryWidget::ShowContextMenu(const wxPoint& pos) {
    wxMenu menu;
    menu.Append(ID_DELETE_ENTRY, "Delete Entry");
    
    PopupMenu(&menu, pos);
}

void PrintHistoryWidget::OnDeleteEntry(wxCommandEvent& event) {
    if (m_selected_entry_id < 0) return;
    
    if (wxMessageBox("Are you sure you want to delete this print history entry?", 
                     "Confirm Delete", wxYES_NO | wxICON_QUESTION) == wxYES) {
        if (m_manager && m_manager->DeleteEntry(m_selected_entry_id)) {
            RefreshHistory();
            m_selected_entry_id = -1;
            
            // Clear details panel
            PrintHistoryEntry empty_entry;
            UpdateDetailsPanel(empty_entry);
        } else {
            wxMessageBox("Failed to delete the entry.", "Delete Error", wxOK | wxICON_ERROR);
        }
    }
}

void PrintHistoryWidget::OnSearch(wxCommandEvent& event) {
    wxString search_term = m_search_ctrl->GetValue().Lower();
    if (search_term.IsEmpty()) {
        PopulateList();
        return;
    }
    
    m_list_ctrl->DeleteAllItems();
    
    size_t displayed_index = 0;
    for (const auto& entry : m_entries) {
        bool matches = entry.filename.Lower().Contains(search_term) ||
                      entry.device_name.Lower().Contains(search_term) ||
                      entry.status.Lower().Contains(search_term) ||
                      entry.user_notes.Lower().Contains(search_term);
        
        if (matches) {
            long item_index = m_list_ctrl->InsertItem(displayed_index, entry.filename);
            m_list_ctrl->SetItem(item_index, 1, entry.device_name);
            m_list_ctrl->SetItem(item_index, 2, entry.print_time.Format("%Y-%m-%d %H:%M"));
            m_list_ctrl->SetItem(item_index, 3, entry.status);
            m_list_ctrl->SetItem(item_index, 4, FormatDuration(entry.duration_seconds));
            
            m_list_ctrl->SetItemData(item_index, entry.id);
            displayed_index++;
        }
    }
}

void PrintHistoryWidget::OnSearchCancel(wxCommandEvent& event) {
    m_search_ctrl->SetValue("");
    PopulateList();
}