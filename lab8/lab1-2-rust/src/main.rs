use esp_idf_sys::*;
use std::*;

fn main() {
    // It is necessary to call this function once. Otherwise some patches to the runtime
    // implemented by esp-idf-sys might not link properly. See https://github.com/esp-rs/esp-idf-template/issues/71
    esp_idf_svc::sys::link_patches();

    // Bind the log crate to the ESP Logging facilities
    esp_idf_svc::log::EspLogger::initialize_default();

    println!("Hello world!");

    /* Code here written after deadline */
    let target_str = std::str::from_utf8(&CONFIG_IDF_TARGET[..CONFIG_IDF_TARGET.len() - 1])
    .expect("Failed to convert to string");

    println!("This is {} chip with {} CPU core(s), {}{}{}{}",
        target_str,
        SOC_CPU_CORES_NUM,
        if SOC_WIFI_SUPPORTED == 1 { "WiFi, " } else { "" },
        if SOC_BT_SUPPORTED == 1 { "BT, " } else { "" },
        if SOC_BLE_SUPPORTED == 1 { "BLE, " } else { "" },
        if esp_mac_type_t_ESP_MAC_IEEE802154 > 0 { "802.15.4 (Zigbee/Thread)" } else { "" },
    );

    let mut flash_size: u32 = 0;
    unsafe{esp_flash_get_size(ptr::null_mut(), &mut flash_size);}

    println!("{}MB flash size", flash_size/1024/1024);

    // get minimum free heap size
    println!("Minimum free heap size: {} bytes", unsafe{esp_get_minimum_free_heap_size()});
    /* End post-deadline section */

    println!("Ryan King");
    
    for i in (0..11).rev() {
        println!("Restarting in {} seconds...", i);
        thread::sleep(time::Duration::from_secs(1));
    }
    println!("Restarting now!");
    unsafe {
        esp_restart();
    }
}
