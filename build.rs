fn main() {
    println!("cargo:rerun-if-changed=src/battery_percent.c");
    println!("cargo:rerun-if-changed=include/battery_percent.h");

    cc::Build::new()
        .file("src/battery_percent.c")
        .include("include")
        .std("c99")
        .warnings(true)
        .compile("battery_percent");

    println!(
        "cargo:include={}/include",
        std::env::var("CARGO_MANIFEST_DIR").unwrap()
    );
}
