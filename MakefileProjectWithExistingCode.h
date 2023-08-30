/*
 * This file uses parts of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#ifndef MakefileProjectWithExistingCode_H_INCLUDED
#define MakefileProjectWithExistingCode_H_INCLUDED

// For compilers that support precompilation, includes <wx/wx.h>
#include <wx/wxprec.h>

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include "cbplugin.h"
#include <memory>

class MakefileProjectWithExistingCode : public cbPlugin
{
public:
    /** Constructor. */
    MakefileProjectWithExistingCode();
    ~MakefileProjectWithExistingCode() = default;

    void BuildMenu(wxMenuBar* menuBar) override;

protected:
    void OnAttach() override;

    void OnRelease(bool appShutDown) override;
    DECLARE_EVENT_TABLE()

private:
    void OnMakefileProjectWithExistingCode( wxCommandEvent& event );
    bool CreateMakefileProjectWithExistingCode(wxString &error);
};

#endif // MakefileProjectWithExistingCode_H_INCLUDED
