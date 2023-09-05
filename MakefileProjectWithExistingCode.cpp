/*
 * This file uses parts of the Code::Blocks IDE and licensed under the GNU General Public License, version 3
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

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
    LogManager* logManager = Manager::Get()->GetLogManager();

    if (!IsAttached() || !menuBar)
    {
        logManager->LogError(wxString::Format(_("%s Preconditions not met!"), pluginName));
        return;
    }

    const int fileMenuIndex = menuBar->FindMenu( _("&File") );
    if ( fileMenuIndex == wxNOT_FOUND )
    {
        logManager->LogError(wxString::Format(_("%s Could not get File menu Idx!"), pluginName));
        return;
    }

    wxMenu* fileMenu = menuBar->GetMenu(fileMenuIndex);
    if (!fileMenu)
    {
        logManager->LogError(wxString::Format(_("%s Could not get File menu!"), pluginName));
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
        fileMenu->Insert(++id, idMakefileProjectWithExistingCode, _("&Makefile Project With Existing Code"), _("&Makefile Project With Existing Code"));
        fileMenu->InsertSeparator(++id);
    }
    else
    {
        logManager->LogError(wxString::Format(_("%s Menu already present!"), pluginName));
    }

}

void MakefileProjectWithExistingCode::OnMakefileProjectWithExistingCode( wxCommandEvent& event )
{
    wxString errorString;
    if (!CreateMakefileProjectWithExistingCode(errorString))
    {
        if (!errorString.IsEmpty())
        {
            Manager::Get()->GetLogManager()->LogError(errorString);
            cbMessageBox(_(errorString), _("Error"), wxICON_ERROR);
        }
    }
}

bool MakefileProjectWithExistingCode::CreateProjectInternal(const wxString& fileName, const wxArrayString& filelist, wxString &errorString)
{
    bool ret;
    ProjectManager* projectManager = Manager::Get()->GetProjectManager();
    cbProject* prj = projectManager->NewProject(fileName);
    if (!prj)
    {
        errorString = wxString::Format(_("%s : Could not create project!"), pluginName);
        ret = false;
    }
    else
    {
        prj->SetMakefileCustom(true);
        prj->AddBuildTarget("all");

        wxArrayInt targets;
        targets.Add(0);

        projectManager->AddMultipleFilesToProject(filelist, prj, targets);
        Manager::Get()->GetLogManager()->Log(wxString::Format(_("%s Added %d files!"), pluginName, (int)filelist.GetCount()));
        prj->SetModified(true);
        prj->CalculateCommonTopLevelPath();
        prj->Save();

        if (!projectManager->IsLoadingWorkspace())
        {
            projectManager->SetProject(prj);
            /*Changes from Pecan - instead of PM SetProject, close and open projects
            https://forums.codeblocks.org/index.php/topic,25509.msg173704.html#msg173704
            */
            bool ok = projectManager->CloseProject(prj, true, true);
            if (!ok)
            {
                errorString = wxString::Format(_("%s : Could not close project!"), pluginName);
                ret = false;
            }
            cbProject* prj = projectManager->LoadProject(fileName, true);
            if (!prj)
            {
                errorString = wxString::Format(_("%s : Could not open project!"), pluginName);
                ret = false;
            }
        }
        projectManager->GetUI().RebuildTree();
        ret = true;
    }
    return ret;
}

bool MakefileProjectWithExistingCode::CreateMakefileProjectWithExistingCode(wxString &errorString)
{
    LogManager* logManager = Manager::Get()->GetLogManager();

    logManager->Log(wxString::Format(_("%s Yay!"), pluginName));
    wxString basePath;

    wxString dir = ChooseDirectory(nullptr,
                                   _("Open directory..."),
                                   basePath,
                                   wxEmptyString,
                                   false,
                                   false);
    if (dir.IsEmpty())
    {
        errorString = wxString::Format(_("%s: Empty directory!"), pluginName);
        return false;
    }

    wxString fileName = dir.AfterLast(wxFileName::GetPathSeparator());

    wxFileName fileNameFull(dir + wxFileName::GetPathSeparator() + fileName);
    fileNameFull.SetExt(FileFilters::CODEBLOCKS_EXT);
    logManager->Log(wxString::Format(_("%s dir %s fileNameFull %s!"), pluginName, dir, fileNameFull.GetFullPath()));

    bool projectSetupCompleted = false;
    // generate list of files to add
    wxArrayString array;
    wxDir::GetAllFiles(dir, &array, wxEmptyString, wxDIR_FILES | wxDIR_DIRS);
    if (array.GetCount() == 0)
    {
        errorString = wxString::Format(_("%s : Could not find any files!"), pluginName);
    }
    else
    {
        // remove any directory entries, some well known SCMs control folders and  C::B project files from the list...
        const size_t endIdx = array.GetCount() - 1;
        for (int i  = endIdx; i >= 0; i--)
        {
            if (wxDirExists(array[i]) ||
                    (wxString::npos != array[i].find(_T("/.git/"))) ||
                    (wxString::npos != array[i].find(_T("\\.git\\"))) ||
                    (wxString::npos != array[i].find(_T("\\.hg\\"))) ||
                    (wxString::npos != array[i].find(_T("/.hg/"))) ||
                    (wxString::npos != array[i].find(_T("\\.svn\\"))) ||
                    (wxString::npos != array[i].find(_T("/.svn/"))) ||
                    (wxString::npos != array[i].find(_T("\\CVS\\"))) ||
                    (wxString::npos != array[i].find(_T("/CVS/"))) ||
                    (wxString::npos != array[i].find(_T("\\CMakeFiles\\"))) ||
                    (wxString::npos != array[i].find(_T("/CMakeFiles/"))) ||
                    (wxString::npos != array[i].find(_T("/.cache/"))) ||
                    (wxString::npos != array[i].find(_T("\\.cache\\"))) ||
                    array[i].Lower().Matches(_T("*.cbp")))
            {
                array.RemoveAt(i);
            }
        }

        if (array.GetCount() == 0)
        {
            errorString = wxString::Format(_("%s : Could not find any valid files!"), pluginName);
        }
        else
        {
            logManager->Log(wxString::Format(_("%s : Before filter %d files present!"), pluginName, (int)array.GetCount()));

            wxString wild;
            const FilesGroupsAndMasks* fgam = Manager::Get()->GetProjectManager()->GetFilesGroupsAndMasks();
            for (unsigned fm_idx = 0; fm_idx < fgam->GetGroupsCount(); fm_idx++)
                wild += fgam->GetFileMasks(fm_idx);

            MultiSelectDlg dlg(nullptr, array, wild, _("Select the files to add to the project:"));
            PlaceWindow(&dlg);
            if (dlg.ShowModal() != wxID_OK)
            {
                logManager->Log(wxString::Format(_("%s : Dialog ShowModal canceled!"), pluginName));
            }
            else
            {
                array = dlg.GetSelectedStrings();

                if (array.GetCount() == 0)
                {
                    errorString = wxString::Format(_("%s : No files selected!"), pluginName);
                }
                else
                {
                    projectSetupCompleted = CreateProjectInternal(fileNameFull.GetFullPath(), array, errorString);
                }
            }
        }

    }
    return projectSetupCompleted;
}

void MakefileProjectWithExistingCode::OnAttach()
{
}

void MakefileProjectWithExistingCode::OnRelease(bool appShutDown)
{
}

