#[repr(C)]
pub struct TeapotApp {
    _private: [u8; 0],
}

#[repr(C)]
pub struct BRect {
    _private: [u8; 0],
}

#[link(name = "shims_lib")]
extern "C" {
    fn new_teapot_app(arg: *const libc::c_char) -> *mut TeapotApp;
    fn teapot_app_run(app: *mut TeapotApp, rect: *mut BRect, name: *const libc::c_char) -> i32;
    fn new_brect(left: f32, top: f32, right: f32, bottom: f32) -> *mut BRect;
}

struct HelloApp {
    app: *mut TeapotApp,
}

impl HelloApp {
    fn new() -> Self {
        Self {
            app: unsafe { new_teapot_app("application/x-vnd.Haiku-GLTeapot\0".as_ptr() as *const libc::c_char) },
        }
    }

    fn run(&self) {
	unsafe {
            teapot_app_run(
                self.app.clone(),
                new_brect(5., 25., 300., 315.),
                "GLTeapot\0".as_ptr() as *const libc::c_char,
            );
        }
    }
}

fn main() {
    let app = HelloApp::new();
    app.run();
}
