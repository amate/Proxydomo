#ifndef __logviewer__
#define __logviewer__

#include <wx/event.h>
#include "events.h"
#include "log.h"


class CLogViewer : public wxEvtHandler {

public:
    void OnProxyEvent(CProxyEvent& evt) {
        cout << "PROXYEVT " << evt.type << " " << evt.addr.Service() << endl;
        cout << "------------------------" << endl;
        evt.Skip();
    }
    void OnHttpEvent(CHttpEvent& evt) {
        cout << "HTTP " << evt.type << " " << evt.addr.Service() << " " << evt.req << endl;
        cout << " - - - - - - - - - - - -" << endl;
        cout << evt.text << endl;
        cout << "------------------------" << endl;
        evt.Skip();
    }
    void OnFilterEvent(CFilterEvent& evt) {
        cout << "FILTER " << evt.type << " " << evt.req << endl;
        cout << " - - - - - - - - - - - -" << endl;
        cout << evt.title << endl;
        cout << evt.text << endl;
        cout << "------------------------" << endl;
        evt.Skip();
    }
    CLogViewer() {
        CLog::ref().proxyListeners.insert(this);
        CLog::ref().httpListeners.insert(this);
        CLog::ref().filterListeners.insert(this);
    }
    ~CLogViewer() {
        CLog::ref().proxyListeners.erase(this);
        CLog::ref().httpListeners.erase(this);
        CLog::ref().filterListeners.erase(this);
    }
    
    DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(CLogViewer, wxEvtHandler)
    EVT_PROXY  (CLogViewer::OnProxyEvent)
    EVT_HTTP   (CLogViewer::OnHttpEvent)
    EVT_FILTER (CLogViewer::OnFilterEvent)
END_EVENT_TABLE()

#endif

