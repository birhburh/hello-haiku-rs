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

class TeapotWindow : public BDirectWindow {
        public:
        	TeapotWindow(BRect r, const char* name, window_type wt, 
                	     ulong something);
                virtual bool    QuitRequested();
                virtual void    MessageReceived(BMessage* msg);
 	private:
 		ObjectView*             fObjectView;
};

extern "C" {
    BRect* new_brect(float left, float top, float right, float bottom)
    {
      return new BRect(left, top, right, bottom);
    }

    TeapotApp* new_teapot_app(const char* sign) 
    {
      return new TeapotApp(sign);
    }
    
    int teapot_app_run(TeapotApp* app, BRect* rect, const char* name) {
	TeapotWindow* fTeapotWindow = new TeapotWindow(*rect, name, B_TITLED_WINDOW, 0);
        fTeapotWindow->Show();
        return app->Run();
    }
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
