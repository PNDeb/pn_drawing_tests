#include <gtk/gtk.h>
#include <time.h>
// modified from: https://docs.gtk.org/gtk3/getting_started.html#custom-drawing
// gcc -Wall -Wextra `pkg-config --cflags gtk+-3.0` -o drawing example.c `pkg-config --libs gtk+-3.0`

/* Surface to store current scribbles */
static cairo_surface_t *surface = NULL;
GtkWidget *drawing_area;
gdouble last_x, last_y;
// cache the last actually blitted region and do nothing if this repeats
gdouble last_drawn_x, last_drawn_y;
gdouble auto_x, auto_y;

struct timespec time_last_call;

// half-width with of drawn rect
const int w_x = 2;
const int w_y = 2;

static void print_extent(cairo_t *cr){
	double x1, y1, x2, y2;
	cairo_clip_extents(cr, &x1, &y1, &x2, &y2);
	printf("clip extents: (%f-%f) (%f-%f)\n", x1, x2, y1, y2);
}


static void
clear_surface (void)
{
  cairo_t *cr;

  cr = cairo_create (surface);

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);

  cairo_destroy (cr);
}

/* Create a new surface of the appropriate size to store our scribbles */
static gboolean
configure_event_cb (GtkWidget         *widget,
                    GdkEventConfigure *event,
                    gpointer           data)
{
  if (surface)
    cairo_surface_destroy (surface);

  surface = gdk_window_create_similar_surface (gtk_widget_get_window (widget),
                                               CAIRO_CONTENT_COLOR,
                                               gtk_widget_get_allocated_width (widget),
                                               gtk_widget_get_allocated_height (widget));

  /* Initialize the surface to white */
  clear_surface ();

  /* We've handled the configure event, no need for further processing. */
  return TRUE;
}

/* Redraw the screen from the surface. Note that the ::draw
 * signal receives a ready-to-be-used cairo_t that is already
 * clipped to only draw the exposed areas of the widget
 */
static gboolean
draw_cb (GtkWidget *widget,
         cairo_t   *cr,
         gpointer   data)
{
	if ((last_x == last_drawn_x) && (last_y == last_drawn_y))
		return FALSE;
	printf("draw_cb\n");
  cairo_set_source_surface (cr, surface, 0, 0);

  /* int last_x = 50; */
  /* int last_y = 20; */

  /* cairo_reset_clip(cr); */
  /* cairo_new_path(cr); */
  /* printf("last x/y: %f/%f\n", last_x, last_y); */
  /* cairo_move_to(cr, last_x+3, last_y-3); */
  /* cairo_line_to(cr, last_x-3, last_y-3); */
  /* cairo_line_to(cr, last_x-3, last_y+3); */
  /* cairo_line_to(cr, last_x+3, last_y+3); */
  /* cairo_close_path(cr); */
  /* cairo_clip(cr); */
  print_extent(cr);

  cairo_paint (cr);

  last_drawn_x = last_x;
  last_drawn_y = last_y;

  return FALSE;
}

/* Draw a rectangle on the surface at the given position */
static void
draw_brush (GtkWidget *widget,
            gdouble    x,
            gdouble    y)
{
  cairo_t *cr;

  /* Paint to the surface, where we store our state */
  cr = cairo_create (surface);

  printf("    brush x/y: %f/%f\n", x, y);
  cairo_rectangle (cr, x - w_x, y - w_y, w_x * 2, w_y * 2);
  cairo_fill (cr);


  last_x = x;
  last_y = y;

  cairo_destroy (cr);

  /* Now invalidate the affected region of the drawing area. */
  gtk_widget_queue_draw_area (widget, x - w_x, y - w_y, w_x * 2, w_y * 2);
}


int keep_drawing(){
	// printf("keep drawing\n");
	struct timespec time_now;

    	clock_gettime(CLOCK_MONOTONIC, &time_now);
	// printf("last: %lu %lu\n", time_last_call.tv_sec, time_last_call.tv_nsec);
	// printf("now: %lu %lu\n", time_now.tv_sec, time_now.tv_nsec);
	static unsigned int nsec = 1000000000;
	unsigned long int diff_ns = time_now.tv_sec * nsec + time_now.tv_nsec - time_last_call.tv_nsec - time_last_call.tv_sec * nsec;
	printf("time since last drawing call: %lu ms\n",
			diff_ns / 1000000
		);

	time_last_call = time_now;
	draw_brush(drawing_area, auto_x, auto_y);
	auto_x++;
	if (auto_x >= 1000)
		return G_SOURCE_REMOVE;
	return TRUE;
}

int draw_loop(){
  int res;
  struct timespec ts;

  ts.tv_sec = 1;
  ts.tv_nsec = 50 * 100000;

  while (1){
	  printf("Loop\n");
	draw_brush(drawing_area, auto_x, auto_y);
	auto_x++;
	  res = nanosleep(&ts, &ts);

  }
  return G_SOURCE_REMOVE;
}

/* Handle button press events by either drawing a rectangle
 * or clearing the surface, depending on which button was pressed.
 * The ::button-press signal handler receives a GdkEventButton
 * struct which contains this information.
 */
static gboolean
button_press_event_cb (GtkWidget      *widget,
                       GdkEventButton *event,
                       gpointer        data)
{
  /* paranoia check, in case we haven't gotten a configure event */
  if (surface == NULL)
    return FALSE;

  if (event->button == GDK_BUTTON_PRIMARY)
    {
      draw_brush (widget, event->x, event->y);
    }
  else if (event->button == GDK_BUTTON_SECONDARY)
    {
      clear_surface ();
      gtk_widget_queue_draw (widget);
    }

  /* We've handled the event, stop processing */
  return TRUE;
}

/* Handle motion events by continuing to draw if button 1 is
 * still held down. The ::motion-notify signal handler receives
 * a GdkEventMotion struct which contains this information.
 */
static gboolean
motion_notify_event_cb (GtkWidget      *widget,
                        GdkEventMotion *event,
                        gpointer        data)
{
  /* paranoia check, in case we haven't gotten a configure event */
  if (surface == NULL)
    return FALSE;

  if (event->state & GDK_BUTTON1_MASK)
    draw_brush (widget, event->x, event->y);

  /* We've handled it, stop processing */
  return TRUE;
}

static void
close_window (void)
{
  if (surface)
    cairo_surface_destroy (surface);
}

static void
activate (GtkApplication *app,
          gpointer        user_data)
{
  GtkWidget *window;
  GtkWidget *frame;

  window = gtk_application_window_new (app);
  // hide cursor startnew(GDK_BLANK_CURSOR);

  // GdkCursor* Cursor = gdk_cursor_new(GDK_BLANK_CURSOR);
  GdkDisplay * display = gdk_display_get_default();
  GdkCursor* Cursor = gdk_cursor_new_for_display(
		  display, GDK_BLANK_CURSOR);

  // hide cursor end

  /*
  // GdkDeviceManager *device_manager = gdk_display_get_device_manager (display);
GdkDevice *pointer = gdk_device_manager_get_client_pointer (device_manager);

	gdk_device_grab (
		pointer,
	       	win,
	       	GDK_OWNERSHIP_NONE, TRUE,
	        GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK,
	        Cursor,
	       	GDK_CURRENT_TIME);
		*/

  gtk_window_set_title (GTK_WINDOW (window), "Drawing Area");

  g_signal_connect (window, "destroy", G_CALLBACK (close_window), NULL);

  gtk_container_set_border_width (GTK_CONTAINER (window), 8);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (window), frame);

  drawing_area = gtk_drawing_area_new ();

  /* set a minimum size */
  gtk_widget_set_size_request (drawing_area, 100, 100);

  gtk_container_add (GTK_CONTAINER (frame), drawing_area);

  /* Signals used to handle the backing surface */
  g_signal_connect (drawing_area, "draw",
                    G_CALLBACK (draw_cb), NULL);
  g_signal_connect (drawing_area,"configure-event",
                    G_CALLBACK (configure_event_cb), NULL);

  /* Event signals */
  g_signal_connect (drawing_area, "motion-notify-event",
                    G_CALLBACK (motion_notify_event_cb), NULL);
  g_signal_connect (drawing_area, "button-press-event",
                    G_CALLBACK (button_press_event_cb), NULL);

  /* Ask to receive events the drawing area doesn't normally
   * subscribe to. In particular, we need to ask for the
   * button press and motion notify events that want to handle.
   */
  gtk_widget_set_events (drawing_area, gtk_widget_get_events (drawing_area)
                                     | GDK_BUTTON_PRESS_MASK
                                     | GDK_POINTER_MOTION_MASK);

  gtk_widget_show_all (window);

  GdkWindow* win = gtk_widget_get_window(window);
  if (win == NULL){
	  printf("WINDOW IS NULL\n");

  }
  gdk_window_set_cursor(win, Cursor);

  GdkSeat* seat = gdk_display_get_default_seat(display);
  GdkSeatCapabilities caps;
  caps = gdk_seat_get_capabilities(seat);
  printf("Capabitlities: %i\n", caps);



  GList * seat_list;
  seat_list = gdk_display_list_seats(display);
  printf("Number fo seats: %i\n",
		  g_list_length(seat_list)
		 );

  GdkGrabStatus status;
  status = gdk_seat_grab(
	seat,
	win,
	// GDK_SEAT_CAPABILITY_ALL_POINTING,
	GDK_SEAT_CAPABILITY_TABLET_STYLUS,
	TRUE,
	Cursor,
	NULL,
	NULL,
	NULL
   );
  if (status == GDK_GRAB_SUCCESS){
	printf("GRAb successful!\n");
  }
	else
	printf("GRAb NOT successful!: %i\n", status);



    clock_gettime(CLOCK_MONOTONIC, &time_last_call);

  g_timeout_add(
	50,
	G_SOURCE_FUNC(keep_drawing),
	// G_SOURCE_FUNC(draw_loop),
	NULL
   );

}

int
main (int    argc,
      char **argv)
{
  GtkApplication *app;
  int status;

  app = gtk_application_new ("org.gtk.example", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}

