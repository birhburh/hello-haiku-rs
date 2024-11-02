fn main() {
    cxx_build::bridge("src/main.rs")
        .include("src")
        .file("src/shims.cpp")
        .compile("hello-rs");

    println!("cargo:rustc-link-lib=be");
    println!("cargo:rustc-link-lib=game");
    println!("cargo:rerun-if-changed=src/main.rs");
    println!("cargo:rerun-if-changed=src/shims.cpp");
    println!("cargo:rerun-if-changed=include/shims.h");
}
