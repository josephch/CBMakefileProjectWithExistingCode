#include <sdk.h> // Code::Blocks SDK


#include <wx/filename.h>
#include <wx/intl.h>
#include <wx/utils.h>
#include <wx/filename.h>
#include <wx/fs_zip.h>
#include <wx/menu.h>
#include <wx/xrc/xmlres.h>
#include <wx/dir.h>

#include <configurationpanel.h>
#include "MakefileProjectWithExistingCode.h"
#include "projectmanager.h"
#include "logmanager.h"
#include "cbproject.h"
#include "globals.h"
#include "filefilters.h"
#include "filegroupsandmasks.h"
#include "multiselectdlg.h"


// Register the plugin with Code::Blocks.
// We are using an anonymous namespace so we don't litter the global one.
namespace
{
PluginRegistrant<MakefileProjectWithExistingCode> reg(_T("MakefileProjectWithExistingCode"));
const int idMakefileProjectWithExistingCode = wxNewId();
}

BEGIN_EVENT_TABLE( MakefileProjectWithExistingCode, cbPlugin )
    EVT_MENU( idMakefileProjectWithExistingCode,  MakefileProjectWithExistingCode::OnMakefileProjectWithExistingCode )
END_EVENT_TABLE()

const char* pluginName = "MakefileProjectWithExistingCode";

MakefileProjectWithExistingCode::MakefileProjectWithExistingCode()
{
    // Make sure our resources are available.
    // In the generated boilerplate code we have no resources but when
    // we add some, it will be nice that this code is in place already ;)
    if(!Manager::LoadResource(_T("MakefileProjectWithExistingCode.zip")))
    {
        NotifyMissingFile(_T("MakefileProjectWithExistingCode.zip"));
    }
}

void MakefileProjectWithExistingCode::BuildMenu(wxMenuBar* menuBar)
{

    if (!IsAttached() || !menuBar)
    {
        Manager::Get()->GetLogManager()->LogError(wxString::Format(_("%s Preconditions not met!"), pluginName));
        return;
    }

    const int fileMenuIndex = menuBar->FindMenu( _("&File") );
    if ( fileMenuIndex == wxNOT_FOUND )
    {
        Manager::Get()->GetLogManager()->LogError(wxString::Format(_("%s Could not get File menu Idx!"), pluginName));
        return;
    }

    wxMenu* fileMenu = menuBar->GetMenu(fileMenuIndex);
    if (!fileMenu)
    {
        Manager::Get()->GetLogManager()->LogError(wxString::Format(_("%s Could not get File menu!"), pluginName));
        return;
    }

    const int pluginMenuId = fileMenu->FindItem(_("&Project from Directory"));
    if (pluginMenuId == wxNOT_FOUND)
    {
        wxMenuItemList menuItems = fileMenu->GetMenuItems();
        const int menuId = fileMenu->FindItem(_("R&ecent files"));
        wxMenuItem* recentFileItem = fileMenu->FindItem(menuId);
        int id = menuItems.IndexOf(recentFileItem);
        if (id == wxNOT_FOUND)
        {
            id = 7;
        }
        else
        {
            ++id;
        }
        // The position is hard-coded to "Recent Files" menu. Please adjust it if necessary
        fileMenu->Insert(++id, idMakefileProjectWithExistingCode, _("&Project from Directory"), _("&Project from Directory"));
        fileMenu->InsertSeparator(++id);
    }
    else
    {
        Manager::Get()->GetLogManager()->LogError(wxString::Format(_("%s Menu already present!"), pluginName));
    }

}

void MakefileProjectWithExistingCode::OnMakefileProjectWithExistingCode( wxCommandEvent& event )
{
    Manager::Get()->GetLogManager()->Log(wxString::Format(_("%s Yay!"), pluginName));
    ProjectManager* pm = Manager::Get()->GetProjectManager();
    wxString basePath;


    wxString dir = ChooseDirectory(nullptr,
                                   _("Open directory..."),
                                   basePath,
                                   wxEmptyString,
                                   false,
                                   false);
    if (dir.IsEmpty())
    {
        Manager::Get()->GetLogManager()->LogError(wxString::Format(_("%s Empty directory!"), pluginName));
        return;
    }


    wxString fileName = dir.AfterLast(wxFileName::GetPathSeparator());

    wxFileName the_file(dir + wxFileName::GetPathSeparator() + fileName);
    the_file.SetExt(FileFilters::CODEBLOCKS_EXT);
    Manager::Get()->GetLogManager()->Log(wxString::Format(_("%s dir %s fileName %s!"), pluginName, dir, the_file.GetFullPath()));

    cbProject* prj = pm->NewProject(the_file.GetFullPath());
    if (!prj)
    {
        Manager::Get()->GetLogManager()->LogError(wxString::Format(_("%s Could not create project!"), pluginName));
        return;
    }

    // generate list of files to add
    wxArrayString array;
    wxDir::GetAllFiles(dir, &array, wxEmptyString, wxDIR_FILES | wxDIR_DIRS);
    if (array.GetCount() == 0)
        return;

    // for usability reasons, remove any directory entries from the list...
    unsigned int i = 0;
    while (i < array.GetCount())
    {
        // discard directories, as well as some well known SCMs control folders ;)
        // also discard C::B project files
        if (wxDirExists(array[i]) ||
                array[i].Contains(_T("/.git/")) ||
                array[i].Contains(_T("\\.git\\")) ||
                array[i].Contains(_T("\\.hg\\")) ||
                array[i].Contains(_T("/.hg/")) ||
                array[i].Contains(_T("\\.svn\\")) ||
                array[i].Contains(_T("/.svn/")) ||
                array[i].Contains(_T("\\CVS\\")) ||
                array[i].Contains(_T("/CVS/")) ||
                array[i].Lower().Matches(_T("*.cbp")))
        {
            array.RemoveAt(i);
        }
        else
            ++i;
    }
    Manager::Get()->GetLogManager()->Log(wxString::Format(_("%s Array before dialog %d files!"), pluginName, (int)array.GetCount()));

    wxString wild;
    const FilesGroupsAndMasks* fgam = pm->GetFilesGroupsAndMasks();
    for (unsigned fm_idx = 0; fm_idx < fgam->GetGroupsCount(); fm_idx++)
        wild += fgam->GetFileMasks(fm_idx);

    MultiSelectDlg dlg(nullptr, array, wild, _("Select the files to add to the project:"));
    PlaceWindow(&dlg);
    if (dlg.ShowModal() != wxID_OK)
    {
        Manager::Get()->GetLogManager()->LogError(wxString::Format(_("%s Dialog ShowModal failed!"), pluginName));
        return;
    }
    array = dlg.GetSelectedStrings();

    prj->SetMakefileCustom(true);
    prj->AddBuildTarget("all");

    wxArrayInt targets;
    // ask for target only if more than one
    if (prj->GetBuildTargetsCount() >= 1)
        targets.Add(0);

    // finally add the files
    pm->AddMultipleFilesToProject(array, prj, targets);
    Manager::Get()->GetLogManager()->Log(wxString::Format(_("%s Added %d files!"), pluginName, (int)array.GetCount()));
    prj->SetModified(true);
    prj->CalculateCommonTopLevelPath();
    prj->Save();

    if (!Manager::Get()->GetProjectManager()->IsLoadingWorkspace())
        Manager::Get()->GetProjectManager()->SetProject(prj);
    //Manager::Get()->GetProjectManager()->CloseProject(prj, true, false);
    Manager::Get()->GetProjectManager()->GetUI().RebuildTree();
}

void MakefileProjectWithExistingCode::OnAttach()
{
}

void MakefileProjectWithExistingCode::OnRelease(bool appShutDown)
{
}

