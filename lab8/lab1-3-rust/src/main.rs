use esp_idf_svc::hal::gpio::*;
use esp_idf_svc::hal::peripherals::Peripherals;
use std::*;

fn main() {
    // It is necessary to call this function once. Otherwise some patches to the runtime
    // implemented by esp-idf-sys might not link properly. See https://github.com/esp-rs/esp-idf-template/issues/71
    esp_idf_svc::sys::link_patches();

    // Bind the log crate to the ESP Logging facilities
    esp_idf_svc::log::EspLogger::initialize_default();

    // start blink task
    let peripherals = Peripherals::take().unwrap();
    let pins = peripherals.pins;
    let mut led = PinDriver::output(pins.gpio7).expect("Failed to initialize LED pin");

    loop {
        led.set_high().expect("Error: Unable to set pin high");
        thread::sleep(time::Duration::from_secs(1));
        led.set_low().expect("Error: Unable to set pin low");
        thread::sleep(time::Duration::from_secs(1));
    }
}
