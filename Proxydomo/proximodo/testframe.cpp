//------------------------------------------------------------------
//
//this file is part of Proximodo
//Copyright (C) 2004 Antony BOUCHER ( kuruden@users.sourceforge.net )
//              2005 Paul Rupe      ( prupe@users.sourceforge.net )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
//------------------------------------------------------------------
// Modifications: (date, author, description)
//
//------------------------------------------------------------------


#include "testframe.h"
#include "settings.h"
#include "filterowner.h"
#include "matcher.h"
#include "expander.h"
#include "filter.h"
#include "const.h"
#include "log.h"
#include "util.h"
#include "controls.h"
#include "images/icon32.xpm"
#include <wx/sysopt.h>
#include <wx/settings.h>
#include <wx/event.h>
#include <wx/msgdlg.h>
#include <wx/icon.h>
#include <wx/timer.h>
#include <string>
#include <sstream>

using namespace std;

/* Event table
 */
BEGIN_EVENT_TABLE(CTestFrame, wxFrame)
    EVT_CLOSE(CTestFrame::OnClose)
    EVT_TEXT  (ID_TESTTEXT, CTestFrame::OnCommand)
    EVT_BUTTON(ID_TEST,     CTestFrame::OnCommand)
    EVT_BUTTON(ID_PROFILE,  CTestFrame::OnCommand)
END_EVENT_TABLE()


/* Saved window position
 */
int CTestFrame::savedX = BIG_NUMBER,
    CTestFrame::savedY = 0,
    CTestFrame::savedW = 0,
    CTestFrame::savedH = 0;

/* Max number of times to run the filter for profiling
 */
static const int profileRuns = 1000;


/* Constructor
 */
CTestFrame::CTestFrame(CFilterDescriptor* desc)
       : wxFrame((wxFrame *)NULL, wxID_ANY,
                 S2W(CSettings::ref().getMessage("TEST_WINDOW_TITLE")),
                 wxDefaultPosition, wxDefaultSize,
                 wxDEFAULT_FRAME_STYLE |
                 wxTAB_TRAVERSAL |
                 wxCLIP_CHILDREN ),
         current(desc) {

    SetIcon(wxIcon(icon32_xpm));
    CSettings& settings = CSettings::ref();
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(mainSizer);

    testMemo =  new pmTextCtrl(this, ID_TESTTEXT, S2W(CLog::ref().testString),
        wxPoint(5,5),wxSize(400,120)  ,
        wxTE_MULTILINE);
    mainSizer->Add(testMemo,1,wxGROW | wxALL,5);

    resultMemo =  new pmTextCtrl(this, wxID_ANY, wxT(""),
        wxPoint(12,75),wxSize(400,120)  ,
        wxTE_MULTILINE | wxTE_READONLY);
    mainSizer->Add(resultMemo,1,wxGROW | wxALL,5);

    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    mainSizer->Add(buttonSizer,0,wxALIGN_CENTER_HORIZONTAL | wxALL,0);

    pmButton* testButton =  new pmButton(this, ID_TEST,
        S2W(settings.getMessage("TEST_WINDOW_TEST_BUTTON")),
        wxPoint(0,0),wxSize(75,25) );
    buttonSizer->Add(testButton,0,wxALIGN_CENTER_VERTICAL | wxALL,5);

    pmButton* profButton =  new pmButton(this, ID_PROFILE,
        S2W(settings.getMessage("TEST_WINDOW_PROF_BUTTON")),
        wxPoint(0,0),wxSize(75,25) );
    buttonSizer->Add(profButton,0,wxALIGN_CENTER_VERTICAL | wxALL,5);

    GetSizer()->Fit(this);
    GetSizer()->SetSizeHints(this);

    // Reload position and size
    if (savedX == BIG_NUMBER) {
        Centre(wxBOTH | wxCENTRE_ON_SCREEN);
    } else {
        Move(savedX, savedY);
        SetSize(savedW, savedH);
    }
}


/* Destructor
 */
CTestFrame::~CTestFrame() {

    // Record position and size
    GetPosition(&savedX, &savedY);
    GetSize(&savedW, &savedH);
}


/* Change current filter
 */
void CTestFrame::setCurrent(CFilterDescriptor* desc) {
    current = desc;
}


/* Hide the window when clicking on the [X] button
 * The window is destroyed by the config screen on its own destruction
 */
void CTestFrame::OnClose(wxCloseEvent& event) {
    Hide();
}


/* Screen containing proxy settings and others
 */
void CTestFrame::OnCommand(wxCommandEvent& event) {
    bool prof = false;
    switch (event.GetId()) {

        case ID_TESTTEXT: {
            CLog::ref().testString = W2S(testMemo->GetValue());
            break;
        }

        case ID_PROFILE: prof = true;
        case ID_TEST: {

            // ensure there is no parsing error in this filter
            if (!current->errorMsg.empty()) {
                wxMessageBox(S2W(current->errorMsg), wxT(APP_NAME));
                return;
            }

            // create a fake text filter (would normally be a
            // resident CRequestManager + CTextBuffer + CTextFilter).
            // This instanciation is not profiled, because it the real
            // world this only happens when Proximodo starts up and
            // processes the first few pages.
            CFilterOwner owner;
            CFilterOwner::setHeader(owner.inHeaders, "Host", "www.host.org");
            string url = "http://www.host.org/path/page.html?query=true#anchor";
            owner.url.parseUrl(url);
            owner.cnxNumber = 1;
            CFilter filter(owner);
            bool okayChars[256];
            CMatcher matcher(current->matchPattern, filter);
            matcher.mayMatch(okayChars);
            CMatcher* boundsMatcher = NULL;
            if (current->filterType == CFilterDescriptor::TEXT && !current->boundsPattern.empty()) {
                boundsMatcher = new CMatcher(current->boundsPattern, filter);
                bool tab[256];
                boundsMatcher->mayMatch(tab);
                for (int i=0; i<256; i++) okayChars[i] = okayChars[i] && tab[i];
            }

            // special processing for <start> and <end> filters
            if (current->matchPattern == "<start>" || current->matchPattern == "<end>") {
                string str = CExpander::expand(current->replacePattern, filter);
                filter.unlock();
                if (current->matchPattern == "<start>") {
                    str += W2S(testMemo->GetValue());
                } else {
                    str = W2S(testMemo->GetValue()) + str;
                }
                resultMemo->SetValue(S2W(str));
                return;
            }

            stringstream result;
            string text;
            int size = 0;
            int run = 0;
            int numMatch = 0;
            wxStopWatch timer;
            
            do {
                text = W2S(testMemo->GetValue());
                size = text.size();
                result.str("");
                owner.killed = false;
                filter.bypassed = false;

                // Test of a text filter.
                // We don't take URL pattern into account.
                if (current->filterType == CFilterDescriptor::TEXT) {

                    bool found = false;
                    const char* bufHead = text.c_str();
                    const char* bufTail = bufHead + size;
                    const char* index   = bufHead;
                    const char* done    = bufHead;
                    while (index<=bufTail && !owner.killed && !filter.bypassed) {
                        if (index<bufTail && !okayChars[(unsigned char)(*index)]) {
                            ++index;
                            continue;
                        }
                        bool matched;
                        const char *end, *reached;
                        const char *stop = index + current->windowWidth;
                        if (stop > bufTail) stop = bufTail;
                        filter.clearMemory();
                        if (boundsMatcher) {
                            matched = boundsMatcher->match(index, stop, end, reached);
                            filter.unlock();
                            if (!matched) {
                                ++index;
                                continue;
                            }
                            stop = end;
                        }
                        matched = matcher.match(index, stop, end, reached);
                        filter.unlock();
                        if (!matched || (boundsMatcher && end != stop)) {
                            ++index;
                            continue;
                        }
                        found = true;
                        if (run == 0) numMatch++;
                        result << string(done, (size_t)(index-done));
                        result << CExpander::expand(current->replacePattern, filter);
                        filter.unlock();
                        done = end;
                        if (end == index) ++index; else index = end;
                    }
                    if (!owner.killed) result << string(done, (size_t)(bufTail-done));
                    if (!found) result.str(CSettings::ref().getMessage("TEST_NO_MATCH"));

                // Test of a header filter. Much simpler...
                } else {

                    const char *index, *stop, *end, *reached;
                    index = text.c_str();
                    stop  = index + text.size();
                    if ((!text.empty() || current->matchPattern.empty())
                        && matcher.match(index, stop, end, reached)) {

                        result << CExpander::expand(current->replacePattern, filter);
                        if (run == 0) numMatch++;
                    } else {
                        result << CSettings::ref().getMessage("TEST_NO_MATCH");
                    }
                    filter.unlock();
                }
            // If profiling, repeat this loop 1000 times or 10 seconds,
            // whichever comes first.
            } while (prof && run++ < profileRuns && timer.Time() < 10000);
            if (boundsMatcher) delete boundsMatcher;
            timer.Pause();
            if (prof) {
                stringstream ss[4];
                double len = testMemo->GetValue().size();
                double time = timer.Time();
                ss[0] << len;
                ss[1] << numMatch;
                ss[2] << (1000.0 * time / run);
                if (time)
                    ss[3] << (1000.0 * len * run / (1024.0 * time));
                else
                    ss[3] << wxT("--");
                string text =
                    CSettings::ref().getMessage("TEST_PROFILE_RESULTS",
                                                ss[0].str(), ss[1].str(),
                                                ss[2].str(), ss[3].str());
                resultMemo->SetValue(S2W(text));
            } else {
                resultMemo->SetValue(SS2W(result));
            }
            break;
        }
    }
}
