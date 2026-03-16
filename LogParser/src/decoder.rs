use std::io::{self, Cursor, prelude::*};

pub mod proto {
    include!(concat!(env!("OUT_DIR"), "/proto.rs"));
}

pub enum Data {
    Raw(Vec<u8>),
    Decoded(Option<proto::LogMessage>),
}

pub struct ProtoView {
    pub sequence: usize,
    pub data: Data,
}

impl ProtoView {
    pub fn new(sequence: usize, data: Vec<u8>) -> Self {
        return Self {
            sequence,
            data: Data::Raw(data),
        };
    }
}

pub fn parse_proto_view<B: Into<Vec<u8>>>(data: B) -> impl Iterator<Item = io::Result<ProtoView>> {
    let buffer: Vec<u8> = data.into();
    let mut stream = Cursor::new(buffer);

    let mut sequence = 0;

    std::iter::repeat_with(move || {
        // Create a mutable slice from the buffer
        let length = prost::decode_length_delimiter(&mut stream)
            .map_err(|e| io::Error::new(io::ErrorKind::InvalidData, e))?;

        let mut data = vec![0; length as usize];
        stream.read_exact(&mut data)?;

        let view = ProtoView::new(sequence, data);
        sequence += 1;

        Ok(view)
    })
    .take_while(Result::is_ok)
}
