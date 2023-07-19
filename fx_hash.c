// This hashing algorithm was extracted from the Rustc compiler.
// This is the same hashing algorithm used for some internal operations in Firefox.
// The strength of this algorithm is in hashing 8 bytes at a time on any platform,
// where the FNV algorithm works on one byte at a time.
//
// This hashing algorithm should not be used for cryptographic, or in scenarios where
// DOS attacks are a concern.

use std::collections::{HashMap, HashSet};
use std::default::Default;
use std::hash::{BuildHasherDefault, Hash, Hasher};
use std::ops::BitXor;

extern crate byteorder;
use byteorder::{ByteOrder, NativeEndian};

/// A builder for default Fx hashers.
pub type FxBuildHasher = BuildHasherDefault<FxHasher>;

/// A `HashMap` using a default Fx hasher.
///
/// Use `FxHashMap::default()`, not `new()` to create a new `FxHashMap`.
/// To create with a reserved capacity, use `FxHashMap::with_capacity_and_hasher(num, Default::default())`.
pub type FxHashMap<K, V> = HashMap<K, V, FxBuildHasher>;

/// A `HashSet` using a default Fx hasher.
///
/// Note: Use `FxHashSet::default()`, not `new()` to create a new `FxHashSet`.
/// To create with a reserved capacity, use `FxHashSet::with_capacity_and_hasher(num, Default::default())`.
pub type FxHashSet<V> = HashSet<V, FxBuildHasher>;

const ROTATE: u32 = 5;
const SEED64: u64 = 0x51_7c_c1_b7_27_22_0a_95;
const SEED32: u32 = 0x9e_37_79_b9;

#[cfg(target_pointer_width = "32")]
const SEED: usize = SEED32 as usize;
#[cfg(target_pointer_width = "64")]
const SEED: usize = SEED64 as usize;


macro_rules! impl_hash_word {
    ($($ty:ty = $key:ident),* $(,)*) => (
        $(
            impl HashWord for $ty {
                #[inline]
                fn hash_word(&mut self, word: Self) {
                    *self = self.rotate_left(ROTATE).bitxor(word).wrapping_mul($key);
                }
            }
        )*
    )
}

impl_hash_word!(usize = SEED, u32 = SEED32, u64 = SEED64);

#[inline]
fn write64(mut hash: u64, mut bytes: &[u8]) -> u64 {
    while bytes.len() >= 8 {
        hash.hash_word(NativeEndian::read_u64(bytes));
        bytes = &bytes[8..];
    }

    if bytes.len() >= 4 {
        hash.hash_word(u64::from(NativeEndian::read_u32(bytes)));
        bytes = &bytes[4..];
    }

    if bytes.len() >= 2 {
        hash.hash_word(u64::from(NativeEndian::read_u16(bytes)));
        bytes = &bytes[2..];
    }

    if let Some(&byte) = bytes.first() {
        hash.hash_word(u64::from(byte));
    }

    hash
}


#[inline]
#[cfg(target_pointer_width = "64")]
fn write(hash: usize, bytes: &[u8]) -> usize {
    write64(hash as u64, bytes) as usize
}


#[derive(Debug, Clone)]
pub struct FxHasher64 {
    hash: u64,
}

impl Default for FxHasher64 {
    #[inline]
    fn default() -> FxHasher64 {
        FxHasher64 { hash: 0 }
    }
}

impl Hasher for FxHasher64 {
    #[inline]
    fn write(&mut self, bytes: &[u8]) {
        self.hash = write64(self.hash, bytes);
    }

    #[inline]
    fn write_u8(&mut self, i: u8) {
        self.hash.hash_word(u64::from(i));
    }

    #[inline]
    fn write_u16(&mut self, i: u16) {
        self.hash.hash_word(u64::from(i));
    }

    #[inline]
    fn write_u32(&mut self, i: u32) {
        self.hash.hash_word(u64::from(i));
    }

    fn write_u64(&mut self, i: u64) {
        self.hash.hash_word(i);
    }

    #[inline]
    fn write_usize(&mut self, i: usize) {
        self.hash.hash_word(i as u64);
    }

    #[inline]
    fn finish(&self) -> u64 {
        self.hash
    }
}

///
