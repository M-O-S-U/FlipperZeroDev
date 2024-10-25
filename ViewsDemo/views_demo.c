#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <gui/view.h>
#include <gui/view_dispatcher.h>
#include <notification/notification.h>
#include <notification/notification_messages.h>

// Enum for views
typedef enum {
    MainScreenView,
    SecondScreenView,
    ThirdScreenView,
} ScreensList;

// App structure
typedef struct {
    ViewDispatcher* view_dispatcher;
    Gui* gui;
    NotificationApp* notify;
    ScreensList screen_state;
    FuriMutex** mutex;
} MainApp;

// Vibration for every view
void short_vibrate() {
    furi_hal_vibro_on(true);
    furi_delay_ms(100);
    furi_hal_vibro_on(false);
}

// Draw callbacks for every view
static void main_screen_draw_callback(Canvas* canvas, void* ctx) {
    UNUSED(ctx);
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 5, 30, "MainScreenView!");
}
static void second_screen_draw_callback(Canvas* canvas, void* ctx) {
    UNUSED(ctx);
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 5, 30, "SecondScreenView!");
}
static void third_screen_draw_callback(Canvas* canvas, void* ctx) {
    UNUSED(ctx);
    canvas_clear(canvas);
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 5, 30, "ThirdScreenView!");
}

// Input callback
static bool input_callback_handle(InputEvent* input_event, void* ctx) {
    furi_assert(ctx);
    bool handled = false;
    MainApp* app = ctx;

    if(input_event->type == InputTypeShort) {
        if(input_event->key == InputKeyRight) {
            view_dispatcher_send_custom_event(app->view_dispatcher, 31);
            handled = true;
        } else if(input_event->key == InputKeyLeft) {
            view_dispatcher_send_custom_event(app->view_dispatcher, 32);
            handled = true;
        }
    }
    return handled;
}

// Custom event callback
bool custom_event_callback_handler(void* ctx, uint32_t eventid) {
    furi_assert(ctx);
    bool handled = false;
    MainApp* app = ctx;

    // This event should only handle switching between views
    if(eventid == 31) {
        if(app->screen_state == MainScreenView) {
            app->screen_state = SecondScreenView;
            notification_message(app->notify, &sequence_blink_red_100);
            short_vibrate();
        } else if(app->screen_state == SecondScreenView) {
            app->screen_state = ThirdScreenView;
            notification_message(app->notify, &sequence_blink_green_100);
            short_vibrate();
        } else {
            app->screen_state = MainScreenView;
            notification_message(app->notify, &sequence_blink_blue_100);
            short_vibrate();
        }
        view_dispatcher_switch_to_view(app->view_dispatcher, app->screen_state);
        handled = true;
    } else if(eventid == 32) {
        if(app->screen_state == MainScreenView) {
            app->screen_state = ThirdScreenView;
            notification_message(app->notify, &sequence_set_green_255);
            short_vibrate();
        } else if(app->screen_state == SecondScreenView) {
            app->screen_state = MainScreenView;
            notification_message(app->notify, &sequence_set_blue_255);
            short_vibrate();
        } else {
            app->screen_state = SecondScreenView;
            notification_message(app->notify, &sequence_set_red_255);
            short_vibrate();
        }
        view_dispatcher_switch_to_view(app->view_dispatcher, app->screen_state);
        handled = true;
    }
    return handled;
}

bool view_dispatcher_navigation_event_callback(void* ctx) {
    UNUSED(ctx);
    // We did not handle the event, so return false.
    return false;
}

// Init app structure
MainApp* init_main_app() {
    MainApp* app = malloc(sizeof(MainApp));
    app->view_dispatcher = view_dispatcher_alloc();
    app->gui = furi_record_open(RECORD_GUI);
    app->notify = furi_record_open(RECORD_NOTIFICATION);
    app->screen_state = MainScreenView;
    app->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    return app;
}

// Free memory
void free_everything(MainApp* app) {
    // Remove views first
    view_dispatcher_remove_view(app->view_dispatcher, MainScreenView);
    view_dispatcher_remove_view(app->view_dispatcher, SecondScreenView);
    view_dispatcher_remove_view(app->view_dispatcher, ThirdScreenView);

    // Free dispatcher
    view_dispatcher_free(app->view_dispatcher);

    // Remove GUI and free app struct
    furi_record_close(RECORD_GUI);
    furi_record_close(RECORD_NOTIFICATION);
    furi_mutex_free(app->mutex);
    free(app);
}

// Configure views
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

int32_t views_demo_main(void* p) {
    UNUSED(p);
    MainApp* app = init_main_app();

    // Create views
    View* view1 = view_alloc();
    configure_views(
        view1, main_screen_draw_callback, input_callback_handle, ViewOrientationHorizontal, app);

    View* view2 = view_alloc();
    configure_views(
        view2, second_screen_draw_callback, input_callback_handle, ViewOrientationHorizontal, app);

    View* view3 = view_alloc();
    configure_views(
        view3, third_screen_draw_callback, input_callback_handle, ViewOrientationHorizontal, app);

    // Set properties
    view_dispatcher_set_event_callback_context(app->view_dispatcher, app);
    view_dispatcher_set_navigation_event_callback(
        app->view_dispatcher, view_dispatcher_navigation_event_callback);
    view_dispatcher_set_custom_event_callback(app->view_dispatcher, custom_event_callback_handler);
    view_dispatcher_enable_queue(app->view_dispatcher);

    // Add all views
    view_dispatcher_add_view(app->view_dispatcher, MainScreenView, view1);
    view_dispatcher_add_view(app->view_dispatcher, SecondScreenView, view2);
    view_dispatcher_add_view(app->view_dispatcher, ThirdScreenView, view3);

    // Attach gui
    view_dispatcher_attach_to_gui(app->view_dispatcher, app->gui, ViewDispatcherTypeFullscreen);
    view_dispatcher_switch_to_view(app->view_dispatcher, app->screen_state);
    view_dispatcher_run(app->view_dispatcher);
    free_everything(app);
    return 0;
}
