use clap::Parser;
use std::fs::OpenOptions;
use std::io::{self, BufWriter, Write};
use std::path::PathBuf;

mod decoder;
use decoder::{DecodeStatus, proto_log_decode};

#[derive(Parser, Debug)]
#[command(version, about, long_about = None)]
struct Args {
    /// Name of the input file
    #[arg(short, long)]
    input_file: PathBuf,

    /// Name of the output file
    #[arg(short, long)]
    output_file: Option<PathBuf>,
}

fn main() -> io::Result<()> {
    let args = Args::parse();

    // input file
    println!("reading : {}", &args.input_file.display());
    let proto_data = std::fs::read(args.input_file).unwrap();

    // decode
    let (proto_views, status) = proto_log_decode(proto_data);

    // output
    println!("Writing ...");
    let out_file_path = args.output_file.unwrap_or(PathBuf::from("output.txt"));
    let mut out_file = BufWriter::new(
        OpenOptions::new()
            .create_new(true)
            .write(true)
            .open(&out_file_path)?,
    );

    proto_views.iter().for_each(|proto_view| {
        if let Some(decoded) = &proto_view.data {
            let _ = writeln!(out_file, "{0} : {1:#}", proto_view.sequence, decoded);
        } else {
            println!("Failed to parse msg {}", proto_view.sequence);
        };
    });

    println!("Output written to : {}", out_file_path.display());
    println!(
        "Status {}",
        match status {
            DecodeStatus::Complete => "Complete",
            DecodeStatus::Partial => "Partial decode, Corrupt file!",
        }
    );

    return Ok(());
}
