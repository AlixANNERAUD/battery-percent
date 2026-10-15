#![no_std]
#![doc = include_str!("../README.md")]

use core::marker::PhantomData;

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct Point {
    pub millivolts: u16,
    pub percent: u8,
}

impl Point {
    pub const fn new(millivolts: u16, percent: u8) -> Self {
        Self {
            millivolts,
            percent,
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct Curve<'a> {
    points: *const Point,
    count: usize,
    _points: PhantomData<&'a [Point]>,
}

unsafe impl Send for Curve<'_> {}
unsafe impl Sync for Curve<'_> {}

impl<'a> Curve<'a> {
    pub const fn new(points: &'a [Point]) -> Self {
        Self {
            points: points.as_ptr(),
            count: points.len(),
            _points: PhantomData,
        }
    }

    pub fn lipo() -> Curve<'static> {
        unsafe { ffi::battery_percent_curve_lipo }
    }

    pub fn li_ion() -> Curve<'static> {
        unsafe { ffi::battery_percent_curve_li_ion }
    }

    pub fn lifepo4() -> Curve<'static> {
        unsafe { ffi::battery_percent_curve_lifepo4 }
    }

    pub fn nimh() -> Curve<'static> {
        unsafe { ffi::battery_percent_curve_nimh }
    }

    pub fn alkaline() -> Curve<'static> {
        unsafe { ffi::battery_percent_curve_alkaline }
    }

    pub fn points(&self) -> &'a [Point] {
        if self.count == 0 {
            return &[];
        }
        unsafe { core::slice::from_raw_parts(self.points, self.count) }
    }

    pub fn percent(&self, millivolts: u16) -> u8 {
        unsafe { ffi::battery_percent_from_millivolts(self, millivolts) }
    }

    pub fn pack_percent(&self, millivolts: u32, cells: u8) -> u8 {
        unsafe { ffi::battery_percent_from_pack(self, millivolts, cells) }
    }
}

pub fn adc_to_millivolts(
    raw: u32,
    resolution_bits: u8,
    reference_millivolts: u32,
    resistor_1: u32,
    resistor_2: u32,
) -> u32 {
    unsafe {
        ffi::battery_percent_adc_to_millivolts(
            raw,
            resolution_bits,
            reference_millivolts,
            resistor_1,
            resistor_2,
        )
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct Filter {
    state: u32,
    shift: u8,
    primed: u8,
}

impl Filter {
    pub fn new(shift: u8) -> Self {
        let mut filter = Self {
            state: 0,
            shift: 0,
            primed: 0,
        };
        unsafe { ffi::battery_percent_filter_init(&mut filter, shift) };
        filter
    }

    pub fn update(&mut self, millivolts: u16) -> u16 {
        unsafe { ffi::battery_percent_filter_update(self, millivolts) }
    }
}

pub mod ffi {
    use super::{Curve, Filter};

    unsafe extern "C" {
        pub static battery_percent_curve_lipo: Curve<'static>;
        pub static battery_percent_curve_li_ion: Curve<'static>;
        pub static battery_percent_curve_lifepo4: Curve<'static>;
        pub static battery_percent_curve_nimh: Curve<'static>;
        pub static battery_percent_curve_alkaline: Curve<'static>;

        pub fn battery_percent_from_millivolts(curve: *const Curve<'_>, millivolts: u16) -> u8;
        pub fn battery_percent_from_pack(curve: *const Curve<'_>, millivolts: u32, cells: u8)
        -> u8;
        pub fn battery_percent_adc_to_millivolts(
            raw: u32,
            resolution_bits: u8,
            reference_millivolts: u32,
            resistor_1: u32,
            resistor_2: u32,
        ) -> u32;
        pub fn battery_percent_filter_init(filter: *mut Filter, shift: u8);
        pub fn battery_percent_filter_update(filter: *mut Filter, millivolts: u16) -> u16;
    }
}
