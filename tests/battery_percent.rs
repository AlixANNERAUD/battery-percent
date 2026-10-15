use battery_percent::{adc_to_millivolts, Curve, Filter, Point};

#[test]
fn builtin_curves_cover_zero_to_hundred() {
    for curve in [
        Curve::lipo(),
        Curve::li_ion(),
        Curve::lifepo4(),
        Curve::nimh(),
        Curve::alkaline(),
    ] {
        let points = curve.points();
        assert_eq!(points.first().unwrap().percent, 0);
        assert_eq!(points.last().unwrap().percent, 100);
        assert!(points.windows(2).all(|w| w[0].millivolts < w[1].millivolts));
        assert!(points.windows(2).all(|w| w[0].percent < w[1].percent));
    }
}

#[test]
fn percent_clamps_and_interpolates() {
    let lipo = Curve::lipo();
    assert_eq!(lipo.percent(0), 0);
    assert_eq!(lipo.percent(4200), 100);
    assert_eq!(lipo.percent(u16::MAX), 100);

    let li_ion = Curve::li_ion();
    let low = li_ion.points()[1];
    let high = li_ion.points()[2];
    let middle = (low.millivolts + high.millivolts) / 2;
    let expected = (low.percent + high.percent) / 2;
    assert!(li_ion.percent(middle).abs_diff(expected) <= 1);
}

#[test]
fn custom_curve() {
    const POINTS: [Point; 3] = [
        Point::new(3000, 0),
        Point::new(3500, 20),
        Point::new(4000, 100),
    ];
    const CURVE: Curve<'static> = Curve::new(&POINTS);

    assert_eq!(CURVE.points(), &POINTS);
    assert_eq!(CURVE.percent(3250), 10);
    assert_eq!(CURVE.percent(3750), 60);
    assert_eq!(CURVE.pack_percent(7500, 2), 60);
    assert_eq!(CURVE.pack_percent(7500, 0), 0);
}

#[test]
fn empty_curve() {
    let curve = Curve::new(&[]);
    assert!(curve.points().is_empty());
    assert_eq!(curve.percent(3700), 0);
}

#[test]
fn adc_conversion() {
    assert_eq!(adc_to_millivolts(4095, 12, 3300, 0, 0), 3300);
    assert_eq!(adc_to_millivolts(2048, 12, 3300, 100_000, 100_000), 3300);
    assert_eq!(adc_to_millivolts(1, 0, 3300, 0, 0), 0);
}

#[test]
fn filter_converges() {
    let mut filter = Filter::new(2);
    assert_eq!(filter.update(4000), 4000);
    assert_eq!(filter.update(3600), 3900);
    let mut value = 0;
    for _ in 0..100 {
        value = filter.update(3600);
    }
    assert_eq!(value, 3600);
}
