# BambuStudio Print History Feature

## Overview
The Print History feature provides a local database-backed history of all your print jobs. It tracks print statistics, timing, and metadata for each completed print.

## Features
- **Local SQLite Database**: All print history is stored locally in a SQLite database
- **Automatic Tracking**: Print jobs are automatically added when they complete
- **Export Functionality**: Export your print history to CSV or JSON format
- **Search and Filter**: Search through your print history by filename, device, status, or notes
- **Print Details**: View detailed information about each print job
- **Sample Data**: Add sample data for testing and demonstration

## Database Location
The print history database is stored at:
- **Windows**: `%APPDATA%/BambuStudio/print_history.db`
- **macOS**: `~/Library/Application Support/BambuStudio/print_history.db`
- **Linux**: `~/.config/BambuStudio/print_history.db`

## Data Structure
Each print history entry contains:
- **ID**: Unique identifier
- **Filename**: Name of the printed file/project
- **Device Name**: Name of the printer used
- **Print Time**: Date and time when the print was started
- **Status**: Print result (Success, Failed, Cancelled, etc.)
- **Duration**: Time taken for the print in seconds
- **User Notes**: Optional notes about the print
- **GCode Path**: Path to the GCode file (if available)
- **Cover Image Path**: Path to preview image (if available)

## Usage

### Accessing Print History
1. Open BambuStudio
2. Navigate to the "Print History" tab
3. View your complete print history in the list

### Adding Sample Data
For testing or demonstration purposes:
1. Click the "Add Sample Data" button
2. Sample print entries will be added to your database
3. Refresh the view to see the new entries

### Exporting Data
1. Click "Export CSV" or "Export JSON" 
2. Choose a location to save the file
3. Your complete print history will be exported

### Searching
1. Use the search box at the top to filter entries
2. Search works across filename, device name, status, and notes
3. Clear the search to show all entries

### Managing Entries
- **View Details**: Click on any entry to see full details in the right panel
- **Delete Entry**: Right-click on an entry and select "Delete Entry"

## Technical Implementation

### Architecture
- **PrintHistoryManager**: Core class handling database operations
- **PrintHistoryWidget**: wxWidgets-based UI component
- **Integration**: Hooks into BambuStudio's print completion events

### Dependencies
- SQLite3 for database operations
- wxWidgets for UI components
- Standard C++ libraries

### Database Schema
```sql
CREATE TABLE print_history (
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
```

## Development Notes
- Thread-safe database operations
- Proper error handling and logging
- Follows BambuStudio coding conventions
- Integrates with existing event system
- Minimal UI that matches BambuStudio design

## Troubleshooting

### Database Issues
- If the database becomes corrupted, delete `print_history.db` and restart BambuStudio
- Check file permissions on the database location
- Ensure SQLite3 is properly installed on the system

### Missing Data
- Print history is only tracked for prints completed after this feature is enabled
- Use "Add Sample Data" to populate the history for testing
- Check that the print completion events are being triggered

### UI Issues
- Refresh the print history tab if data doesn't appear
- Check the application logs for any error messages
- Ensure the print history tab is properly initialized

## Future Enhancements
- Import functionality for external print history data
- Advanced filtering and sorting options
- Print statistics and analytics
- Integration with cloud backup systems
- Print cost tracking and analysis