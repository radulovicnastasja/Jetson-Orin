#include <gtk/gtk.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include "gui.h"
#include "balance_loop.h"

static pthread_mutex_t data_lock = PTHREAD_MUTEX_INITIALIZER;
static double display_pitch = 0.0, display_rpm0 = 0.0, display_rpm1 = 0.0;
static GtkWidget *pitch_label, *rpm0_label, *rpm1_label;
static GtkWidget *status_label, *enable_button;
static GtkWidget *kp_spin, *ki_spin, *kd_spin;
static GtkApplication *application;
static gboolean updating_controls = FALSE;

void gui_update_data(double pitch, double rpm0, double rpm1)
{
    pthread_mutex_lock(&data_lock);
    display_pitch = pitch;
    display_rpm0 = rpm0;
    display_rpm1 = rpm1;
    pthread_mutex_unlock(&data_lock);
}

static double normalized_pitch(double raw)
{
    if (!isfinite(raw)) return NAN;
    double p = raw - 180.0;
    while (p < -180.0) p += 360.0;
    while (p >= 180.0) p -= 360.0;
    return p;
}

static gboolean update_display(gpointer unused)
{
    (void)unused;
    double pitch, rpm0, rpm1;
    pthread_mutex_lock(&data_lock);
    pitch = display_pitch;
    rpm0 = display_rpm0;
    rpm1 = display_rpm1;
    pthread_mutex_unlock(&data_lock);
    char buf[128];
    snprintf(buf, sizeof(buf), "Corrected pitch: %+.2f deg", normalized_pitch(pitch));
    gtk_label_set_text(GTK_LABEL(pitch_label), buf);
    snprintf(buf, sizeof(buf), "Motor 0: %.2f RPM", rpm0);
    gtk_label_set_text(GTK_LABEL(rpm0_label), buf);
    snprintf(buf, sizeof(buf), "Motor 1: %.2f RPM", rpm1);
    gtk_label_set_text(GTK_LABEL(rpm1_label), buf);
    gboolean on = balance_loop_is_enabled();
    gtk_label_set_text(GTK_LABEL(status_label), on ? "Controller: ENABLED" : "Controller: DISABLED");
    gtk_button_set_label(GTK_BUTTON(enable_button), on ? "Disable Motors" : "Enable Motors");
    return G_SOURCE_CONTINUE;
}

static void gains_changed(GtkSpinButton *spin, gpointer unused)
{
    (void)spin; (void)unused;
    if (updating_controls) return;
    balance_loop_set_gains(
        gtk_spin_button_get_value(GTK_SPIN_BUTTON(kp_spin)),
        gtk_spin_button_get_value(GTK_SPIN_BUTTON(ki_spin)),
        gtk_spin_button_get_value(GTK_SPIN_BUTTON(kd_spin)));
}

static void reset_gains(GtkButton *button, gpointer unused)
{
    (void)button; (void)unused;
    updating_controls = TRUE;
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(kp_spin), 0.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(ki_spin), 0.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(kd_spin), 0.0);
    updating_controls = FALSE;
    balance_loop_set_gains(0.0, 0.0, 0.0);
}

static void toggle_motors(GtkButton *button, gpointer unused)
{
    (void)button; (void)unused;
    balance_loop_set_enabled(!balance_loop_is_enabled());
    update_display(NULL);
}

static void quit_clicked(GtkButton *button, gpointer unused)
{
    (void)button; (void)unused;
    balance_loop_set_enabled(0);
    g_application_quit(G_APPLICATION(application));
}

static GtkWidget *gain_row(GtkWidget *parent, const char *name, GtkWidget **spin_out)
{
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    GtkWidget *label = gtk_label_new(name);
    gtk_widget_set_size_request(label, 30, -1);
    GtkAdjustment *adjustment = gtk_adjustment_new(0.0, 0.0, 10.0, 0.001, 0.01, 0.0);
    GtkWidget *spin = gtk_spin_button_new(adjustment, 0.001, 3);
    gtk_widget_set_hexpand(spin, TRUE);
    gtk_box_append(GTK_BOX(row), label);
    gtk_box_append(GTK_BOX(row), spin);
    gtk_box_append(GTK_BOX(parent), row);
    *spin_out = spin;
    return row;
}

static void activate(GtkApplication *app, gpointer unused)
{
    (void)unused;
    GtkWidget *window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(window), "Lab 05 - Self-Balancing Robot");
    gtk_window_set_default_size(GTK_WINDOW(window), 390, 360);
    GtkWidget *layout = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_top(layout, 16);
    gtk_widget_set_margin_bottom(layout, 16);
    gtk_widget_set_margin_start(layout, 16);
    gtk_widget_set_margin_end(layout, 16);
    gtk_window_set_child(GTK_WINDOW(window), layout);

    pitch_label = gtk_label_new("Corrected pitch: --");
    rpm0_label = gtk_label_new("Motor 0: -- RPM");
    rpm1_label = gtk_label_new("Motor 1: -- RPM");
    status_label = gtk_label_new("Controller: DISABLED");
    gtk_box_append(GTK_BOX(layout), pitch_label);
    gtk_box_append(GTK_BOX(layout), rpm0_label);
    gtk_box_append(GTK_BOX(layout), rpm1_label);
    gtk_box_append(GTK_BOX(layout), status_label);

    gain_row(layout, "Kp", &kp_spin);
    gain_row(layout, "Ki", &ki_spin);
    gain_row(layout, "Kd", &kd_spin);
    g_signal_connect(kp_spin, "value-changed", G_CALLBACK(gains_changed), NULL);
    g_signal_connect(ki_spin, "value-changed", G_CALLBACK(gains_changed), NULL);
    g_signal_connect(kd_spin, "value-changed", G_CALLBACK(gains_changed), NULL);

    GtkWidget *buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
    gtk_box_append(GTK_BOX(layout), buttons);
    enable_button = gtk_button_new_with_label("Enable Motors");
    GtkWidget *reset = gtk_button_new_with_label("Reset Gains");
    GtkWidget *quit = gtk_button_new_with_label("Quit");
    gtk_box_append(GTK_BOX(buttons), enable_button);
    gtk_box_append(GTK_BOX(buttons), reset);
    gtk_box_append(GTK_BOX(buttons), quit);
    g_signal_connect(enable_button, "clicked", G_CALLBACK(toggle_motors), NULL);
    g_signal_connect(reset, "clicked", G_CALLBACK(reset_gains), NULL);
    g_signal_connect(quit, "clicked", G_CALLBACK(quit_clicked), NULL);

    g_timeout_add(50, update_display, NULL);
    gtk_window_present(GTK_WINDOW(window));
}

void gui_run(void)
{
    application = gtk_application_new("edu.msoe.cpe4700.lab05", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(application, "activate", G_CALLBACK(activate), NULL);
    g_application_run(G_APPLICATION(application), 0, NULL);
    balance_loop_set_enabled(0);
    g_object_unref(application);
    application = NULL;
}
