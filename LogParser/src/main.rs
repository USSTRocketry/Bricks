use clap::Parser;
use prost::Message;
use rayon::prelude::*;
use std::fs::OpenOptions;
use std::io::{self, BufWriter, Cursor, Write};
use std::path::PathBuf;

mod decoder;
use decoder::*;

/// Simple program to greet a person
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

    println!("reading : {}", &args.input_file.display());
    let proto_data = std::fs::read(args.input_file)?;

    // chunk into proto views
    let raw_proto: Vec<ProtoView> = decoder::parse_proto_view(proto_data)
        .filter_map(Result::ok)
        .collect();

    // decode the actual proto msg
    let mut decoded_proto: Vec<ProtoView> = raw_proto
        .par_iter()
        .filter_map(|view| {
            if let Data::Raw(ref raw_data) = view.data {
                Some(ProtoView {
                    sequence: view.sequence,
                    data: Data::Decoded(
                        proto::LogMessage::decode(&mut Cursor::new(&raw_data)).ok(),
                    ),
                })
            } else {
                None
            }
        })
        .collect();

    decoded_proto.par_sort_by(|a, b| a.sequence.cmp(&b.sequence));

    let out_file_path = args.output_file.unwrap_or(PathBuf::from("output.txt"));
    let mut out_file = BufWriter::new(
        OpenOptions::new()
            .create_new(true)
            .write(true)
            .open(&out_file_path)?,
    );

    // print to console
    decoded_proto.iter().for_each(|proto_view| {
        if let Data::Decoded(Some(decoded)) = &proto_view.data {
            let _ = writeln!(out_file, "{:?}: {:?}", proto_view.sequence, decoded);
        } else {
            println!("Failed to parse msg {}", proto_view.sequence);
        };
    });

    println!("Output written to : {}", out_file_path.display());

    return Ok(());
}
