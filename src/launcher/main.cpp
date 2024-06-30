#include <wx/app.h>
#include <wx/frame.h>

class Launcher : public wxApp {
public:
    bool OnInit() override {
        auto *frame{new wxFrame(nullptr, wxID_ANY, "Frame")};
        frame->Show();

        return true;
    }
};

wxIMPLEMENT_APP(Launcher);

