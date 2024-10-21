// Declare necessary objects
let usbcheck = require("badusb");
let notify = require("notification");

// Setup USB properties
usbcheck.setup({
    vid: 0xABCD,
    pid: 0xBCDA,
    mfrName: "USBCheck",
    prodName: "M-O-S-U"
});

// Handle connection issues
delay(1000);

// Do the things and quit
if (usbcheck.isConnected()){
    notify.success();
    print("USB is plugged!");
    usbcheck.print("MOSU");
} else {
    notify.error();
    print("USB is not plugged!");
}
usbcheck.quit();