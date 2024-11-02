#[cxx::bridge]
mod ffi {
    unsafe extern "C++" {
        include!("Rect.h");

        include!("shims.h");

        type BRect;
        type TeapotApp;

        fn new_teapot_app(arg: &str) -> SharedPtr<TeapotApp>;
        fn teapot_app_run(app: SharedPtr<TeapotApp>, rect: SharedPtr<BRect>, name: &str) -> i32;
        fn new_brect(left: f32, top: f32, right: f32, bottom: f32) -> SharedPtr<BRect>;
    }
}

struct TeapotApp {
    app: cxx::SharedPtr<ffi::TeapotApp>,
}

impl TeapotApp {
    fn new() -> Self {
        Self {
            app: ffi::new_teapot_app("application/x-vnd.Haiku-GLTeapot"),
        }
    }

    fn run(&self) {
        ffi::teapot_app_run(
            self.app.clone(),
            ffi::new_brect(5., 25., 300., 315.),
            "GLTeapot",
        );
    }
}

fn main() {
    let app = TeapotApp::new();
    app.run();
}
