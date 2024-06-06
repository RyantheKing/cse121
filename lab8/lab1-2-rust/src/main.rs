use log::info;
use esp_idf_sys::*;
use std::*;

fn main() {
    // It is necessary to call this function once. Otherwise some patches to the runtime
    // implemented by esp-idf-sys might not link properly. See https://github.com/esp-rs/esp-idf-template/issues/71
    esp_idf_svc::sys::link_patches();

    // Bind the log crate to the ESP Logging facilities
    esp_idf_svc::log::EspLogger::initialize_default();

    info!("Hello world!");

    info!("Ryan King");
    
    for i in (0..11).rev() {
        info!("Restarting in {} seconds...", i);
        thread::sleep(time::Duration::from_secs(1));
    }
    info!("Restarting now!");
    unsafe {
        esp_restart();
    }
}
