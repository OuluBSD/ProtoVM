#include "ProtoVMApp.h"
#include "MainFrame.h"

bool ProtoVMApp::OnInit()
{
    if (!wxApp::OnInit())
        return false;

    MainFrame* mainFrame = new MainFrame("ProtoVM Circuit Designer");
    mainFrame->Show(true);

    return true;
}

int ProtoVMApp::OnExit()
{
    return wxApp::OnExit();
}