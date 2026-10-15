#![no_std]
#![warn(missing_docs)]
#![doc = include_str!("../README.md")]

use core::marker::PhantomData;

/// One point of a discharge curve: a cell voltage and the charge percentage at that voltage.
#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Eq, Hash)]
pub struct Point {
    /// Cell voltage in millivolts.
    pub millivolts: u16,
    /// Charge percentage at that voltage, from 0 to 100.
    pub percent: u8,
}

impl Point {
    /// Creates a point, usable in `const` contexts.
    ///
    /// ```
    /// use battery_percent::Point;
    ///
    /// const EMPTY: Point = Point::new(3000, 0);
    /// assert_eq!(EMPTY.millivolts, 3000);
    /// ```
    pub const fn new(millivolts: u16, percent: u8) -> Self {
        Self {
            millivolts,
            percent,
        }
    }
}

/// A discharge curve: points sorted by strictly increasing voltage, with linear interpolation between them.
///
/// Use one of the built-in curves, or build your own from cells you have measured with [`Curve::new`]. The curve
/// borrows its points and is layout-compatible with the C `battery_percent_curve_t`.
///
/// ```
/// use battery_percent::Curve;
///
/// let lipo = Curve::lipo();
/// assert_eq!(lipo.percent(3821), 50);
/// assert_eq!(lipo.percent(5000), 100);
/// ```
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
    /// Creates a curve from points sorted by strictly increasing voltage, usable in `const` contexts.
    ///
    /// Percentages should increase with voltage.
    ///
    /// ```
    /// use battery_percent::{Curve, Point};
    ///
    /// const POINTS: [Point; 3] = [Point::new(3000, 0), Point::new(3500, 20), Point::new(4000, 100)];
    /// const MY_CELL: Curve<'static> = Curve::new(&POINTS);
    ///
    /// assert_eq!(MY_CELL.percent(3250), 10);
    /// ```
    pub const fn new(points: &'a [Point]) -> Self {
        Self {
            points: points.as_ptr(),
            count: points.len(),
            _points: PhantomData,
        }
    }

    /// Lithium polymer (LiPo) cell, 3306 to 4177 mV.
    ///
    /// Open-circuit voltage after a 60 minute rest of a typical Li-ion polymer cell, from Maxim/ADI Application
    /// Note 4189.
    pub fn lipo() -> Curve<'static> {
        unsafe { ffi::battery_percent_curve_lipo }
    }

    /// Cylindrical Li-ion NMC cell such as an 18650, 3281 to 4180 mV.
    ///
    /// Open-circuit voltage after a 2 hour rest of a Samsung INR18650-20R, from CALCE incremental OCV data. 0% is the
    /// 2.5 V cutoff under a 0.5C load.
    pub fn li_ion() -> Curve<'static> {
        unsafe { ffi::battery_percent_curve_li_ion }
    }

    /// Lithium iron phosphate (LiFePO4) cell, 2500 to 3500 mV.
    ///
    /// Average of C/20 charge and discharge curves of A123 cells, from CALCE low-current OCV data. The curve is nearly
    /// flat between 40% and 90%, where voltage can only resolve the charge coarsely.
    pub fn lifepo4() -> Curve<'static> {
        unsafe { ffi::battery_percent_curve_lifepo4 }
    }

    /// Nickel-metal hydride (NiMH) AA cell, 1000 to 1440 mV.
    ///
    /// Voltage under a 0.1C load of an Energizer NH15-2300, from its datasheet.
    pub fn nimh() -> Curve<'static> {
        unsafe { ffi::battery_percent_curve_nimh }
    }

    /// Alkaline AA cell, 900 to 1500 mV.
    ///
    /// Voltage under a 50 mA intermittent load of an Energizer E91, from its datasheet, with 0% at 0.9 V.
    pub fn alkaline() -> Curve<'static> {
        unsafe { ffi::battery_percent_curve_alkaline }
    }

    /// Points of the curve, sorted by increasing voltage.
    ///
    /// ```
    /// use battery_percent::Curve;
    ///
    /// let points = Curve::lipo().points();
    /// assert_eq!(points.first().unwrap().percent, 0);
    /// assert_eq!(points.last().unwrap().percent, 100);
    /// ```
    pub fn points(&self) -> &'a [Point] {
        if self.count == 0 {
            return &[];
        }
        unsafe { core::slice::from_raw_parts(self.points, self.count) }
    }

    /// Charge percentage of a single cell, from 0 to 100.
    ///
    /// Interpolates linearly between the two points surrounding `millivolts` and rounds to the nearest percent.
    /// Voltages below the first point return its percentage, and voltages above the last point return its
    /// percentage. An empty curve returns 0.
    ///
    /// ```
    /// use battery_percent::Curve;
    ///
    /// assert_eq!(Curve::li_ion().percent(3672), 50);
    /// assert_eq!(Curve::li_ion().percent(0), 0);
    /// ```
    pub fn percent(&self, millivolts: u16) -> u8 {
        unsafe { ffi::battery_percent_from_millivolts(self, millivolts) }
    }

    /// Charge percentage of a pack of `cells` identical cells in series, from 0 to 100.
    ///
    /// Divides the pack voltage by the number of cells, rounding to the nearest millivolt, then calls
    /// [`Curve::percent`]. Returns 0 when `cells` is 0.
    ///
    /// ```
    /// use battery_percent::Curve;
    ///
    /// assert_eq!(Curve::li_ion().pack_percent(11016, 3), 50);
    /// ```
    pub fn pack_percent(&self, millivolts: u32, cells: u8) -> u8 {
        unsafe { ffi::battery_percent_from_pack(self, millivolts, cells) }
    }
}

/// Converts a raw ADC reading into the battery voltage in millivolts.
///
/// Scales `raw` so that the full-scale reading (2<sup>`resolution_bits`</sup> - 1) equals `reference_millivolts`,
/// then undoes a voltage divider made of `resistor_1` between the battery and the ADC pin and `resistor_2` between
/// the pin and ground, both in the same unit. Pass `resistor_2 = 0` without a divider.
///
/// Readings above full scale are clamped, results are rounded to the nearest millivolt and saturate to `u32::MAX`.
/// Returns 0 when `resolution_bits` is not between 1 and 32.
///
/// ```
/// use battery_percent::adc_to_millivolts;
///
/// assert_eq!(adc_to_millivolts(4095, 12, 3300, 0, 0), 3300);
/// assert_eq!(adc_to_millivolts(2048, 12, 3300, 100_000, 100_000), 3300);
/// ```
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

/// Integer exponential moving average filter that smooths noisy voltage readings.
///
/// Each reading has a weight of 1 / 2<sup>`shift`</sup>. The first reading is returned unchanged and seeds the
/// filter.
///
/// ```
/// use battery_percent::Filter;
///
/// let mut filter = Filter::new(2);
/// assert_eq!(filter.update(4000), 4000);
/// assert_eq!(filter.update(3600), 3900);
/// ```
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct Filter {
    state: u32,
    shift: u8,
    primed: u8,
}

impl Filter {
    /// Creates a filter where each reading has a weight of 1 / 2<sup>`shift`</sup>.
    ///
    /// 0 disables smoothing, and higher values smooth more but react more slowly. `shift` is clamped to 16.
    pub fn new(shift: u8) -> Self {
        let mut filter = Self {
            state: 0,
            shift: 0,
            primed: 0,
        };
        unsafe { ffi::battery_percent_filter_init(&mut filter, shift) };
        filter
    }

    /// Feeds a reading in millivolts and returns the smoothed voltage in millivolts.
    pub fn update(&mut self, millivolts: u16) -> u16 {
        unsafe { ffi::battery_percent_filter_update(self, millivolts) }
    }
}

/// Raw bindings to the C library, as declared in `battery_percent.h`.
///
/// Prefer the safe API at the crate root. These are useful to pass curves or filters to other C code.
pub mod ffi {
    use super::{Curve, Filter};

    unsafe extern "C" {
        /// `battery_percent_curve_lipo`, see [`Curve::lipo`].
        pub static battery_percent_curve_lipo: Curve<'static>;
        /// `battery_percent_curve_li_ion`, see [`Curve::li_ion`].
        pub static battery_percent_curve_li_ion: Curve<'static>;
        /// `battery_percent_curve_lifepo4`, see [`Curve::lifepo4`].
        pub static battery_percent_curve_lifepo4: Curve<'static>;
        /// `battery_percent_curve_nimh`, see [`Curve::nimh`].
        pub static battery_percent_curve_nimh: Curve<'static>;
        /// `battery_percent_curve_alkaline`, see [`Curve::alkaline`].
        pub static battery_percent_curve_alkaline: Curve<'static>;

        /// `battery_percent_from_millivolts`, see [`Curve::percent`]. `curve` may be null.
        pub fn battery_percent_from_millivolts(curve: *const Curve<'_>, millivolts: u16) -> u8;
        /// `battery_percent_from_pack`, see [`Curve::pack_percent`]. `curve` may be null.
        pub fn battery_percent_from_pack(curve: *const Curve<'_>, millivolts: u32, cells: u8)
        -> u8;
        /// `battery_percent_adc_to_millivolts`, see [`adc_to_millivolts`](crate::adc_to_millivolts).
        pub fn battery_percent_adc_to_millivolts(
            raw: u32,
            resolution_bits: u8,
            reference_millivolts: u32,
            resistor_1: u32,
            resistor_2: u32,
        ) -> u32;
        /// `battery_percent_filter_init`, see [`Filter::new`]. Does nothing when `filter` is null.
        pub fn battery_percent_filter_init(filter: *mut Filter, shift: u8);
        /// `battery_percent_filter_update`, see [`Filter::update`]. Returns `millivolts` when `filter` is null.
        pub fn battery_percent_filter_update(filter: *mut Filter, millivolts: u16) -> u16;
    }
}
