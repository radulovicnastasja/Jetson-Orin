
#ifndef GUI_H
#define GUI_H

/* Run GTK4 in the main thread; blocks until Quit */
void gui_run(void);

/* Called by the control thread to publish readings */
void gui_update_data(double pitch,
                     double motor0_rpm,
                     double motor1_rpm);

#endif
