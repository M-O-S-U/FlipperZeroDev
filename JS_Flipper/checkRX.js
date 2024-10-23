// Create necessary objects
let subghz = require("subghz");
let notify = require("notification");

subghz.setup(); // init subghz object

// Switch SUBGHZ mode to RX and read signal strength
function print_signals(){
    let temp_rssi = 0;
    let signal_counter = 0;
    while (1){
        let rssi = subghz.getRssi();
        let freq = subghz.getFrequency();
        let ext = subghz.isExternal();
        if (rssi !== temp_rssi){
            print("RSSI: ", rssi, " dBm@ ", freq, "MHz", "ext: ", ext, "Signal count: ", signal_counter, "\n\n\n\n\n");
            temp_rssi = rssi;
            signal_counter++;
        }
        delay(500);
    }
}

// Actual code
if (subghz.getState() !== "RX"){
    notify.error();
    print("SUB-GHZ State is: ", subghz.getState());
    print("Switching to RX...");
    subghz.setRx();
    print_signals();
} else {
    notify.success();
    print("SUB-GHZ State is: ", subghz.getState());
    print_signals();
}