#include <stdlib.h>
#include <furi.h>
#include <gui/gui.h>
#include <gui/view.h>
#include <gui/view_dispatcher.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>
#include <subghz/devices/devices.h>

// Create an array for supported frequencies
char* frequencies[] = {"300.00", "302.00", "303.87", "303.90", "304.25", "307.00", "307.50",
                       "307.80", "309.00", "310.00", "312.00", "312.10", "312.20", "313.00",
                       "313.85", "314.00", "314.35", "314.98", "315.00", "318.00", "330.00",
                       "345.00", "348.00", "350.00", "387.00", "390.00", "418.00", "430.00",
                       "430.50", "431.00", "431.50", "433.07", "433.22", "433.42", "433.65",
                       "433.88", "433.92", "434.07", "434.17", "434.19", "434.39", "434.42",
                       "434.62", "434.77", "438.90", "440.17", "464.00", "467.75", "779.00",
                       "868.35", "868.40", "868.80", "868.95", "906.40", "915.00", "925.00",
                       "928.00"};

// Create enum for views
typedef enum {
    SignalMainScreen,
    SignalSelectFreqScreen,
    SignalMonitorScreen,
} SignalTestScreenState;

// Main app structure
typedef struct {
    ViewDispatcher* view_dispatcher;
    Gui* gui;
    SignalTestScreenState screen_state;
    NotificationApp* notify;
    View* view1;
    View* view2;
    View* view3;
    uint32_t converted_freq;
    int freq_index;
} MainApp;

// Model for handling variable etc.
const SubGhzDevice* subghzdevice;
char* current_frequency;
char* converted_rssi;
float rssi;

// Helpers
char* floatToString(float num) {
    // Allocate memory for the string (buffer size 10 should be sufficient for most floats)
    char* str = (char*)malloc(10 * sizeof(char));

    // Use snprintf to convert float to string
    snprintf(str, 10, "%f", (double)num);
    return str;
}
void convert_frequency(MainApp* app, char* frequency) {
    // Use strtof to convert the string to a float
    char* endPtr;
    float result = strtof(frequency, &endPtr);
    app->converted_freq = result * 1000000;
}

// Init subghz
void bootup_subghz() {
    subghz_devices_init();
    subghzdevice = subghz_devices_get_by_name("cc1101_int");
    subghz_devices_begin(subghzdevice);
    subghz_devices_reset(subghzdevice);
    subghz_devices_load_preset(subghzdevice, FuriHalSubGhzPresetOok650Async, NULL);
}

// Activate subghz
void activate_signals(MainApp* app) {
    subghz_devices_flush_rx(subghzdevice);
    subghz_devices_idle(subghzdevice);
    subghz_devices_set_frequency(subghzdevice, app->converted_freq);
    subghz_devices_set_rx(subghzdevice);
}

// Close and deinit subghz
void close_signals() {
    subghz_devices_flush_rx(subghzdevice);
    subghz_devices_idle(subghzdevice);
    subghz_devices_sleep(subghzdevice);
    subghz_devices_end(subghzdevice);
    subghz_devices_deinit();
}

// Draw callbacks for every view
static void main_screen_draw_callback(Canvas* canvas, void* ctx) {
    UNUSED(ctx);
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 5, 10, "- Signal Monitor -");
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 5, 25, "@CYB3RMX &");
    canvas_draw_str(canvas, 5, 35, "@MathematicianGoat");
    canvas_draw_frame(canvas, 65, 46, 61, 17);
    canvas_draw_str_aligned(canvas, 95, 55, AlignCenter, AlignCenter, "->Freq select");
}
static void signal_select_freq_draw_callback(Canvas* canvas, void* ctx) {
    UNUSED(ctx);
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 5, 10, "- Frequency Select -");
    canvas_draw_frame(canvas, 5, 20, 50, 20);
    canvas_draw_str_aligned(canvas, 30, 30, AlignCenter, AlignCenter, current_frequency);
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 60, 25, "Up: Increase");
    canvas_draw_str(canvas, 60, 35, "Down: Decrease");
    canvas_draw_str(canvas, 60, 45, "OK: Select");
}
static void signal_monitor_draw_callback(Canvas* canvas, void* ctx) {
    UNUSED(ctx);
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 5, 10, "- Signal Monitor -");
    rssi = subghz_devices_get_rssi(subghzdevice);
    converted_rssi = floatToString(rssi);
    canvas_draw_str(canvas, 5, 25, "Rssi: ");
    canvas_draw_str(canvas, 40, 25, converted_rssi);
    free(converted_rssi);
}

// Input callback
static bool input_callback_handler(InputEvent* input_event, void* ctx) {
    furi_assert(ctx);
    bool handled = false;
    MainApp* app = ctx;

    if(input_event->type == InputTypeShort) { // Handle short keystrokes
        // Catch keystrokes and define custom events for them
        if(input_event->key == InputKeyRight) {
            view_dispatcher_send_custom_event(app->view_dispatcher, 0);
            handled = true;
        } else if(input_event->key == InputKeyLeft) {
            view_dispatcher_send_custom_event(app->view_dispatcher, 1);
            handled = true;
        } else if(input_event->key == InputKeyBack) {
            // We dont know even one reason why it works. But fuck it anyway it works!!
            handled = false;
        }

        // Handle the frequency select screen events
        if(app->screen_state == SignalSelectFreqScreen) {
            if(input_event->key == InputKeyUp && app->freq_index < 57) {
                app->freq_index++;
                current_frequency = frequencies[app->freq_index];
                handled = true;
            } else if(input_event->key == InputKeyDown && app->freq_index > 0) {
                app->freq_index--;
                current_frequency = frequencies[app->freq_index];
                handled = true;
            } else if(input_event->key == InputKeyOk) {
                app->screen_state = SignalMonitorScreen;
                notification_message(app->notify, &sequence_success);

                // Activate the subghz on RX mode then switch the view
                convert_frequency(app, current_frequency);
                activate_signals(app);
                view_dispatcher_switch_to_view(app->view_dispatcher, app->screen_state);
                handled = true;
            }
        }
    }
    return handled;
}

// Custom event callback
bool custom_event_callback_handler(void* ctx, uint32_t eventid) {
    furi_assert(ctx);
    bool handled = false;
    MainApp* app = ctx;
    if(eventid ==
       0) { // This event id should handle the navigation event between Main screen -> Select freq screen
        if(app->screen_state == SignalMainScreen) {
            app->screen_state = SignalSelectFreqScreen;
            handled = true;
        }
        view_dispatcher_switch_to_view(app->view_dispatcher, app->screen_state);
    } else if(eventid == 1) { // This event id should handle the reverse navigation events
        if(app->screen_state == SignalMonitorScreen) {
            app->screen_state = SignalSelectFreqScreen;
            handled = true;
        } else if(app->screen_state == SignalSelectFreqScreen) {
            app->screen_state = SignalMainScreen;
            handled = true;
        }
        view_dispatcher_switch_to_view(app->view_dispatcher, app->screen_state);
    }
    return handled;
}

//Handle back button for exit
bool view_dispatcher_navigation_event_callback(void* ctx) {
    UNUSED(ctx);
    return false;
}

//Init app structure
MainApp* init_main_app() {
    MainApp* app = malloc(sizeof(MainApp));
    app->view_dispatcher = view_dispatcher_alloc();
    app->gui = furi_record_open(RECORD_GUI);
    app->notify = furi_record_open(RECORD_NOTIFICATION);
    app->screen_state = SignalMainScreen;
    app->view1 = view_alloc();
    app->view2 = view_alloc();
    app->view3 = view_alloc();
    app->freq_index = 0;
    current_frequency = frequencies[app->freq_index]; // 300.00 by default
    bootup_subghz();
    return app;
}

//Free memory
void free_everything(MainApp* app) {
    // Close subghz
    close_signals();

    //Remove every view
    view_dispatcher_remove_view(app->view_dispatcher, SignalMainScreen);
    view_dispatcher_remove_view(app->view_dispatcher, SignalSelectFreqScreen);
    view_dispatcher_remove_view(app->view_dispatcher, SignalMonitorScreen);

    //Remove dispatcher itself
    view_dispatcher_free(app->view_dispatcher);

    //Remove GUI and free app struct
    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_NOTIFICATION);
    free(converted_rssi);
    free(current_frequency);
    free(app);
}

// Configure properties and views
void configure_views(
    View* view,
    ViewDrawCallback draw_callback,
    ViewInputCallback input_callback,
    ViewOrientation orientation,
    void* ctx) {
    view_set_context(view, ctx);
    view_set_draw_callback(view, draw_callback);
    view_set_input_callback(view, input_callback);
    view_set_orientation(view, orientation);
}
void configure_properties(MainApp* app) {
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_navigation_event_callback(
        app->view_dispatcher, view_dispatcher_navigation_event_callback);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, custom_event_callback_handler);
    view_dispatcher_enable_queue(app->view_dispatcher);
}

int32_t signal_test_main(void* p) {
    UNUSED(p);

    // Init app
    MainApp* app = init_main_app();

    // Configure views and properties
    configure_views(
        app->view1,
        main_screen_draw_callback,
        input_callback_handler,
        ViewOrientationHorizontal,
        app);
    configure_views(
        app->view2,
        signal_select_freq_draw_callback,
        input_callback_handler,
        ViewOrientationHorizontal,
        app);
    configure_views(
        app->view3,
        signal_monitor_draw_callback,
        input_callback_handler,
        ViewOrientationHorizontal,
        app);
    configure_properties(app);

    // Add views to dispatcher
    view_dispatcher_add_view(app->view_dispatcher, SignalMainScreen, app->view1);
    view_dispatcher_add_view(app->view_dispatcher, SignalSelectFreqScreen, app->view2);
    view_dispatcher_add_view(app->view_dispatcher, SignalMonitorScreen, app->view3);

    // Attach views to GUI
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_switch_to_view(app->view_dispatcher, app->screen_state);
    view_dispatcher_run(app->view_dispatcher);

    // Free
    free_everything(app);
    return 0;
}
