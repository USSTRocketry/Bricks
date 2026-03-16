use glob::glob;
use std::io::Result;

fn main() -> Result<()> {
    const PROTO_FOLDER: &str = "Tisdale-proto-files";

    // Tell Cargo that if the given file changes, to rerun this build script.
    println!("cargo:rerun-if-changed={}", PROTO_FOLDER);

    let proto_files: Vec<String> = glob(&format!("{}/**/*.proto", PROTO_FOLDER))
        .expect("Failed to read proto files")
        .filter_map(|entry| entry.ok()?.to_str().map(|s| s.to_string()))
        .collect();

    prost_build::compile_protos(&proto_files, &[PROTO_FOLDER])?;

    return Ok(());
}
