#include "shims.h"

#include <stdio.h>
#include <new>

#include <Application.h>
#include <Catalog.h>
#include <DirectWindow.h>
#include <InterfaceKit.h>
#include <Point.h>
#include <Rect.h>

#include "ObjectView.h"

std::shared_ptr<BRect> new_brect(float left, float top, float right, float bottom)
{
  return std::make_shared<BRect>(left, top, right, bottom);
}

class TeapotWindow : public BDirectWindow {
        public:
        	TeapotWindow(BRect r, const char* name, window_type wt, 
                	     ulong something);
                virtual bool    QuitRequested();
                virtual void    MessageReceived(BMessage* msg);
 	private:
 		ObjectView*             fObjectView;
};


std::shared_ptr<TeapotApp> new_teapot_app(rust::Str sign)
{
  return std::make_shared<TeapotApp>(std::string(sign).c_str());
}

int teapot_app_run(std::shared_ptr<TeapotApp> app, std::shared_ptr<BRect> rect, rust::Str name) {
	TeapotWindow*   fTeapotWindow = new TeapotWindow(*rect, std::string(name).c_str(), B_TITLED_WINDOW, 0); 
        fTeapotWindow->Show();
        return app->Run();
}

TeapotWindow::TeapotWindow(BRect rect, const char* name, window_type wt, 
        ulong something)
        :   
        BDirectWindow(rect, name, wt, something)
{
        Lock();
	BRect bounds = Bounds();
	BView *subView = new BView(bounds, "subview", B_FOLLOW_ALL, 0); 
        AddChild(subView); 
        
        bounds = subView->Bounds(); 
	GLenum type = BGL_RGB | BGL_DEPTH | BGL_DOUBLE;
        fObjectView = new(std::nothrow) ObjectView(bounds, "objectView", 
                B_FOLLOW_ALL_SIDES, type); 
        subView->AddChild(fObjectView); 
        SetSizeLimits(32, 1024, 32, 1024);
        Unlock();
}


bool
TeapotWindow::QuitRequested()
{
        be_app->PostMessage(B_QUIT_REQUESTED);
        return true;
}

void
TeapotWindow::MessageReceived(BMessage* msg)
{
        switch (msg->what) {
                default:
                        BDirectWindow::MessageReceived(msg);
        }
}
