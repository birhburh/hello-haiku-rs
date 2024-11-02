#include <Application.h>
#include <DirectWindow.h>
#include <Rect.h>
#include <memory>
#include "rust/cxx.h"

class TeapotApp : public BApplication {
public:
	TeapotApp(const char* sign): BApplication(sign) {};
};

std::shared_ptr<TeapotApp> new_teapot_app(rust::Str sign);
int teapot_app_run(std::shared_ptr<TeapotApp> app, std::shared_ptr<BRect> rect, rust::Str name); 
std::shared_ptr<BRect> new_brect(float left, float top, float right, float bottom);
