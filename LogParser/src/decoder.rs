use once_cell::sync::Lazy;
use prost_reflect::{DescriptorPool, DynamicMessage};
use rayon::prelude::*;
use std::io::Cursor;

pub static DESCRIPTOR_POOL: Lazy<DescriptorPool> = Lazy::new(|| {
    DescriptorPool::decode(
        include_bytes!(concat!(env!("OUT_DIR"), "/file_descriptor_set.bin")).as_ref(),
    )
    .unwrap()
});

pub struct ProtoView {
    pub sequence: usize,
    pub data: Option<DynamicMessage>,
}

pub enum DecodeStatus {
    Complete,
    Partial,
}

impl ProtoView {
    pub fn new(sequence: usize, decoded: Option<DynamicMessage>) -> Self {
        return Self {
            sequence,
            data: decoded,
        };
    }
}

fn parse_proto_ranges(data: &[u8]) -> (Vec<(usize, usize)>, DecodeStatus) {
    let mut stream = Cursor::new(data);
    let mut ranges = Vec::new();

    while let Ok(length) = prost::decode_length_delimiter(&mut stream) {
        let start = stream.position() as usize;
        let end = start.saturating_add(length);
        if end > data.len() {
            return (ranges, DecodeStatus::Partial);
        }

        ranges.push((start, end));
        stream.set_position(end as u64);
    }

    (ranges, DecodeStatus::Complete)
}

pub fn proto_log_decode<B: AsRef<[u8]>>(proto_data: B) -> (Vec<ProtoView>, DecodeStatus) {
    let buffer = proto_data.as_ref();
    let (ranges, status) = parse_proto_ranges(buffer);

    let message_desc = DESCRIPTOR_POOL
        .get_message_by_name("Proto.LogMessage")
        .expect("Failed to find LogMessage definition in desc pool");

    // Decode directly from slices of the original buffer to avoid per-message copies.
    // Use `map` and share a cloned descriptor across threads.
    let message_desc = message_desc.clone();

    (
        ranges
            .into_par_iter()
            .enumerate()
            .map(|(sequence, (start, end))| {
                let decoded = DynamicMessage::decode(message_desc.clone(), &buffer[start..end]);
                ProtoView::new(sequence, decoded.ok())
            })
            .collect(),
        status,
    )
}
