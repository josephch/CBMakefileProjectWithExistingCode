# MakefileProjectWithExistingCode

Create new C::B project from make file project with existing code.

## Usage
Build using C::B

Install via Plugins-> Manage Plugins -> Install New  
On successful installation, File menu will have following item  
> Makefile Project With Existing Code
 
Select, browse to the directory to be added and select.

## Known issues

### Issue with clangd_plugin
After creating project, project is not popultated if clangd_plugin is enabled.
Works fine if code completion plugin is enabled and clangd_plugin is disabled.

On restarting C::B, the project works as expected with clangd_plugin

## Test status
Tested on Ubuntu 16.04 with svn rev 13343 and wx30
