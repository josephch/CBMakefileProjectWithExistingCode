/***************************************************************
 * Name:      MakefileProjectWithExistingCode
 * Purpose:   Code::Blocks plugin
 * Author:    Christo Joseph (christo.j@gmail.com)
 * Created:   2023-08-20
 * Copyright: Christo Joseph
 * License:   GPL
 **************************************************************/

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

    void OnMakefileProjectWithExistingCode( wxCommandEvent& event );
    DECLARE_EVENT_TABLE()
};

#endif // MakefileProjectWithExistingCode_H_INCLUDED
