#ifndef PROTOWXAPP_H
#define PROTOWXAPP_H

#include <wx/wx.h>

class ProtoVMApp : public wxApp
{
public:
    virtual bool OnInit();
    virtual int OnExit();
};

#endif // PROTOWXAPP_H