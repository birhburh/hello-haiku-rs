fn main() {
    cc::Build::new()
        .cpp(true)
        .include("src")
        .file("src/shims.cpp")
        .file("src/ObjectView.cpp")
        .file("src/FPS.cpp")
        .compile("shims_lib");

    println!("cargo:rustc-link-lib=be");
    println!("cargo:rustc-link-lib=game");
    println!("cargo:rustc-link-lib=GL");

    println!("cargo:rerun-if-changed=src/shims.cpp");
    println!("cargo:rerun-if-changed=src/shims.h");
    println!("cargo:rerun-if-changed=src/ObjectView.cpp");
    println!("cargo:rerun-if-changed=src/ObjectView.h");
    println!("cargo:rerun-if-changed=src/FPS.cpp");
    println!("cargo:rerun-if-changed=src/FPS.h");
}
