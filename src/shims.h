#include <Application.h>
#include <DirectWindow.h>
#include <Rect.h>
#include <memory>

class TeapotApp : public BApplication {
public:
	TeapotApp(const char* sign): BApplication(sign) {};
};

extern "C" {
	TeapotApp* new_teapot_app(const char* sign); 
	int teapot_app_run(TeapotApp* app, BRect* rect, const char* name);
	BRect* new_brect(float left, float top, float right, float bottom);
	
	void delete_teapot_app(TeapotApp* app);
	void delete_brect(BRect* rect);
}

