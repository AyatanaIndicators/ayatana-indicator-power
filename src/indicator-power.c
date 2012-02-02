/*
An indicator to power related information in the menubar.

Copyright 2011 Canonical Ltd.

Authors:
    Javier Jardon <javier.jardon@codethink.co.uk>

This program is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License version 3, as published 
by the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranties of
MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* GStuff */
#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>

/* upower */
#include <libupower-glib/upower.h>

/* Indicator Stuff */
#include <libindicator/indicator.h>
#include <libindicator/indicator-object.h>

#define DEFAULT_ICON   "gpm-battery-missing"

#define DBUS_SERVICE                "org.gnome.SettingsDaemon"
#define DBUS_PATH                   "/org/gnome/SettingsDaemon"
#define POWER_DBUS_PATH             DBUS_PATH "/Power"
#define POWER_DBUS_INTERFACE        "org.gnome.SettingsDaemon.Power"

#define INDICATOR_POWER_TYPE            (indicator_power_get_type ())
#define INDICATOR_POWER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), INDICATOR_POWER_TYPE, IndicatorPower))
#define INDICATOR_POWER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), INDICATOR_POWER_TYPE, IndicatorPowerClass))
#define IS_INDICATOR_POWER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INDICATOR_POWER_TYPE))
#define IS_INDICATOR_POWER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), INDICATOR_POWER_TYPE))
#define INDICATOR_POWER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), INDICATOR_POWER_TYPE, IndicatorPowerClass))

GType indicator_power_get_type (void);

INDICATOR_SET_VERSION
INDICATOR_SET_TYPE (INDICATOR_POWER_TYPE)

typedef struct {
  IndicatorObjectClass parent_class;
}
IndicatorPowerClass;

typedef struct {
  GtkMenu   *menu;

  GtkLabel *label;
  GtkImage *status_image;
  gchar    *accessible_desc;

  GCancellable *proxy_cancel;
  GDBusProxy   *proxy;
  guint         watcher_id;

  GVariant *devices;
  GVariant *device;

  GSettings *settings;
}
IndicatorPowerPrivate;

typedef struct {
  IndicatorObject parent_instance;

  IndicatorPowerPrivate *priv;
}
IndicatorPower;

/* Prototypes */
static void             indicator_power_dispose         (GObject *object);
static void             indicator_power_finalize        (GObject *object);

static GtkLabel*        get_label                       (IndicatorObject * io);
static GtkImage*        get_image                       (IndicatorObject * io);
static GtkMenu*         get_menu                        (IndicatorObject * io);
static const gchar*     get_accessible_desc             (IndicatorObject * io);
static const gchar*     get_name_hint                   (IndicatorObject * io);

static void gsd_appeared_callback (GDBusConnection *connection, const gchar *name, const gchar *name_owner, gpointer user_data);

G_DEFINE_TYPE (IndicatorPower, indicator_power, INDICATOR_OBJECT_TYPE);

static void
indicator_power_class_init (IndicatorPowerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  IndicatorObjectClass *io_class = INDICATOR_OBJECT_CLASS (klass);

  object_class->dispose = indicator_power_dispose;
  object_class->finalize = indicator_power_finalize;

  io_class->get_label = get_label;
  io_class->get_image = get_image;
  io_class->get_menu = get_menu;
  io_class->get_accessible_desc = get_accessible_desc;
  io_class->get_name_hint = get_name_hint;

  g_type_class_add_private (klass, sizeof (IndicatorPowerPrivate));
}

static void
indicator_power_init (IndicatorPower *self)
{
  IndicatorPowerPrivate *priv;

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            INDICATOR_POWER_TYPE,
                                            IndicatorPowerPrivate);
  priv = self->priv;

  /* Init variables */
  priv->menu = NULL;
  priv->accessible_desc = NULL;

  priv->watcher_id = g_bus_watch_name (G_BUS_TYPE_SESSION,
                                       DBUS_SERVICE,
                                       G_BUS_NAME_WATCHER_FLAGS_NONE,
                                       gsd_appeared_callback,
                                       NULL,
                                       self,
                                       NULL);

  priv->settings = g_settings_new ("com.canonical.indicator.power");
}

static void
indicator_power_dispose (GObject *object)
{
  IndicatorPowerPrivate *priv = INDICATOR_POWER(object)->priv;

  if (priv->devices != NULL) {
    g_variant_unref (priv->devices);
    priv->devices = NULL;
  }

  if (priv->device != NULL) {
    g_variant_unref (priv->device);
    priv->device = NULL;
  }

  g_clear_object (&priv->settings);

  G_OBJECT_CLASS (indicator_power_parent_class)->dispose (object);
}

static void
indicator_power_finalize (GObject *object)
{
  IndicatorPowerPrivate *priv = INDICATOR_POWER(object)->priv;

  g_free (priv->accessible_desc);

  G_OBJECT_CLASS (indicator_power_parent_class)->finalize (object);
}

/***
****
***/

static void
show_info_cb (GtkMenuItem *item,
              gpointer     data)
{
  /*TODO: show the statistics of the specific device*/
  const gchar *command = "gnome-power-statistics";

  if (g_spawn_command_line_async (command, NULL) == FALSE)
    g_warning ("Couldn't execute command: %s", command);
}

static void
option_toggled_cb (GtkCheckMenuItem *item,
                   gpointer     user_data)
{
  IndicatorPower *self = INDICATOR_POWER (user_data);
  IndicatorPowerPrivate *priv = self->priv;
  gboolean visible;

  visible = gtk_check_menu_item_get_active (item);

  gtk_widget_set_visible (GTK_WIDGET (priv->label),
                          visible);

  g_settings_set_boolean (priv->settings, "show-time",
                          visible);
}

static void
show_preferences_cb (GtkMenuItem *item,
                     gpointer     data)
{
  const gchar *command = "gnome-control-center power";

  if (g_spawn_command_line_async (command, NULL) == FALSE)
    g_warning ("Couldn't execute command: %s", command);
}

static void
get_timestring (guint64   time_secs,
                gchar   **short_timestring,
                gchar   **detailed_timestring)
{
  gint  hours;
  gint  minutes;

  /* Add 0.5 to do rounding */
  minutes = (int) ( ( time_secs / 60.0 ) + 0.5 );

  if (minutes == 0)
    {
      *short_timestring = g_strdup (_("Unknown time"));
      *detailed_timestring = g_strdup (_("Unknown time"));

      return;
    }

  if (minutes < 60)
    {
      *short_timestring = g_strdup_printf ("0:%.2i", minutes);
      *detailed_timestring = g_strdup_printf (g_dngettext (GETTEXT_PACKAGE, "%i minute",
                                              "%i minutes",
                                              minutes), minutes);
      return;
    }

  hours = minutes / 60;
  minutes = minutes % 60;

  *short_timestring = g_strdup_printf ("%i:%.2i", hours, minutes);

  if (minutes == 0)
    {
      *detailed_timestring = g_strdup_printf (g_dngettext (GETTEXT_PACKAGE, 
                                              "%i hour",
                                              "%i hours",
                                              hours), hours);
    }
  else
    {
      /* TRANSLATOR: "%i %s %i %s" are "%i hours %i minutes"
       * Swap order with "%2$s %2$i %1$s %1$i if needed */
      *detailed_timestring = g_strdup_printf (_("%i %s %i %s"),
                                              hours, g_dngettext (GETTEXT_PACKAGE, "hour", "hours", hours),
                                              minutes, g_dngettext (GETTEXT_PACKAGE, "minute", "minutes", minutes));
    }
}

static const gchar *
device_kind_to_localised_string (UpDeviceKind kind)
{
  const gchar *text = NULL;

  switch (kind) {
    case UP_DEVICE_KIND_LINE_POWER:
      /* TRANSLATORS: system power cord */
      text = _("AC adapter");
      break;
    case UP_DEVICE_KIND_BATTERY:
      /* TRANSLATORS: laptop primary battery */
      text = _("Battery");
      break;
    case UP_DEVICE_KIND_UPS:
      /* TRANSLATORS: battery-backed AC power source */
      text = _("UPS");
      break;
    case UP_DEVICE_KIND_MONITOR:
      /* TRANSLATORS: a monitor is a device to measure voltage and current */
      text = _("Monitor");
      break;
    case UP_DEVICE_KIND_MOUSE:
      /* TRANSLATORS: wireless mice with internal batteries */
      text = _("Mouse");
      break;
    case UP_DEVICE_KIND_KEYBOARD:
      /* TRANSLATORS: wireless keyboard with internal battery */
      text = _("Keyboard");
      break;
    case UP_DEVICE_KIND_PDA:
      /* TRANSLATORS: portable device */
      text = _("PDA");
      break;
    case UP_DEVICE_KIND_PHONE:
      /* TRANSLATORS: cell phone (mobile...) */
      text = _("Cell phone");
      break;
    case UP_DEVICE_KIND_MEDIA_PLAYER:
      /* TRANSLATORS: media player, mp3 etc */
      text = _("Media player");
      break;
    case UP_DEVICE_KIND_TABLET:
      /* TRANSLATORS: tablet device */
      text = _("Tablet");
      break;
    case UP_DEVICE_KIND_COMPUTER:
      /* TRANSLATORS: tablet device */
      text = _("Computer");
      break;
    default:
      g_warning ("enum unrecognised: %i", kind);
      text = up_device_kind_to_string (kind);
    }

  return text;
}

static void
build_device_time_details (const gchar    *device_name,
                           guint64         time,
                           UpDeviceState   state,
                           gdouble         percentage,
                           gchar         **short_details,
                           gchar         **details,
                           gchar         **accesible_name)
{
  gchar *short_timestring = NULL;
  gchar *detailed_timestring = NULL;

  if (time > 0)
    {
      get_timestring (time,
                      &short_timestring,
                      &detailed_timestring);

      if (state == UP_DEVICE_STATE_CHARGING)
        {
          /* TRANSLATORS: %2 is a time string, e.g. "1 hour 5 minutes" */
          *accesible_name = g_strdup_printf (_("%s (%s to charge (%.0lf%%))"),
                                             device_name, detailed_timestring, percentage);
          *details = g_strdup_printf (_("%s (%s to charge)"),
                                      device_name, short_timestring);
          *short_details = g_strdup_printf ("(%s)", short_timestring);
        }
      else if (state == UP_DEVICE_STATE_DISCHARGING)
        {
          *short_details = g_strdup_printf ("%s", short_timestring);

          if (time > 43200) /* 12 hours */
            {
              *accesible_name = g_strdup_printf (_("%s"), device_name);
              *details = g_strdup_printf (_("%s"), device_name);
            }
          else
            {
              /* TRANSLATORS: %2 is a time string, e.g. "1 hour 5 minutes" */
              *accesible_name = g_strdup_printf (_("%s (%s left (%.0lf%%))"),
                                                 device_name, detailed_timestring, percentage);
              *details = g_strdup_printf (_("%s (%s left)"),
                                          device_name, short_timestring);
            }
        }

      g_free (short_timestring);
      g_free (detailed_timestring);
    }
  else
    {
      if (state == UP_DEVICE_STATE_FULLY_CHARGED)
        {
          *details = g_strdup_printf (_("%s (charged)"), device_name);
          *accesible_name = g_strdup (*details);
          *short_details = g_strdup ("");
        }
      else if (percentage > 0)
        {
          /* TRANSLATORS: %2 is a percentage value. Note: this string is only
           * used when we don't have a time value */
          *details = g_strdup_printf (_("%s (%.0lf%%)"),
                                      device_name, percentage);
          *accesible_name = g_strdup (*details);
          *short_details = g_strdup_printf (_("(%.0lf%%)"),
                                            percentage);
        }
      else
        {
          *details = g_strdup_printf (_("%s (not present)"), device_name);
          *accesible_name = g_strdup (*details);
          *short_details = g_strdup (_("(not present)"));
        }
    }
}

static void
set_accessible_desc (IndicatorPower *self,
                     const gchar    *desc)
{
  IndicatorPowerPrivate *priv = self->priv;

  if (desc == NULL || desc[0] == '\0')
    return;

  g_free (priv->accessible_desc);

  priv->accessible_desc = g_strdup (desc);
}

static const gchar *
get_icon_percentage_for_status (const gchar *status)
{

  if (g_strcmp0 (status, "caution") == 0)
    return "000";
  else if (g_strcmp0 (status, "low") == 0)
    return "040";
  else if (g_strcmp0 (status, "good") == 0)
    return "080";
  else
    return "100";
}

static GIcon*
build_battery_icon (UpDeviceState  state,
                    gchar         *suffix_str)
{
  GIcon *gicon;

  GString *filename;
  gchar **iconnames;

  filename = g_string_new (NULL);

  if (state == UP_DEVICE_STATE_FULLY_CHARGED)
    {
      g_string_append (filename, "battery-charged;");
      g_string_append (filename, "battery-full-charged-symbolic;");
      g_string_append (filename, "battery-full-charged;");
      g_string_append (filename, "gpm-battery-charged;");
      g_string_append (filename, "gpm-battery-100-charging;");
    }
  else if (state == UP_DEVICE_STATE_CHARGING)
    {
      g_string_append (filename, "battery-000-charging;");
      g_string_append (filename, "battery-caution-charging-symbolic;");
      g_string_append (filename, "battery-caution-charging;");
      g_string_append (filename, "gpm-battery-000-charging;");
    }
  else if (state == UP_DEVICE_STATE_DISCHARGING)
    {
      const gchar *percentage = get_icon_percentage_for_status (suffix_str);
      g_string_append_printf (filename, "battery-%s;", suffix_str);
      g_string_append_printf (filename, "battery-%s-symbolic;", suffix_str);
      g_string_append_printf (filename, "battery-%s;", percentage);
      g_string_append_printf (filename, "gpm-battery-%s;", percentage);
    }

  iconnames = g_strsplit (filename->str, ";", -1);
  gicon = g_themed_icon_new_from_names (iconnames, -1);

  g_strfreev (iconnames);
  g_string_free (filename, TRUE);

  return gicon;
}

static GIcon*
get_device_icon (UpDeviceKind   kind,
                 UpDeviceState  state,
                 guint64        time_sec,
                 gchar         *device_icon)
{
  GIcon *gicon;

  gicon = g_icon_new_for_string (device_icon, NULL);

  if (kind == UP_DEVICE_KIND_BATTERY &&
      (state == UP_DEVICE_STATE_FULLY_CHARGED ||
       state == UP_DEVICE_STATE_CHARGING ||
       state == UP_DEVICE_STATE_DISCHARGING))
    {
      if (state == UP_DEVICE_STATE_FULLY_CHARGED ||
          state == UP_DEVICE_STATE_CHARGING)
        {
          gicon = build_battery_icon (state, NULL);
        }
      else if (state == UP_DEVICE_STATE_DISCHARGING)
        {
          if ((time_sec > 60 * 30) && /* more than 30 minutes left */
              (g_strrstr (device_icon, "000") ||
               g_strrstr (device_icon, "020") ||
               g_strrstr (device_icon, "caution"))) /* the icon is red */
            {
              gicon = build_battery_icon (state, "low");
            }
        }
    }

  return gicon;
}


static void
menu_add_device (GtkMenu  *menu,
                 GVariant *device)
{
  UpDeviceKind kind;
  UpDeviceState state;
  GtkWidget *icon;
  GtkWidget *item;
  GtkWidget *details_label;
  GtkWidget *grid;
  GIcon *device_gicons;
  gchar *device_icon = NULL;
  gchar *object_path = NULL;
  gdouble percentage;
  guint64 time;
  const gchar *device_name;
  gchar *short_details = NULL;
  gchar *details = NULL;
  gchar *accesible_name = NULL;

  if (device == NULL)
    return;

  g_variant_get (device,
                 "(susdut)",
                 &object_path,
                 &kind,
                 &device_icon,
                 &percentage,
                 &state,
                 &time);

  g_debug ("%s: got data from object %s", G_STRFUNC, object_path);

  if (kind == UP_DEVICE_KIND_LINE_POWER)
    return;

  /* Process the data */
  device_gicons = get_device_icon (kind, state, time, device_icon);
  icon = gtk_image_new_from_gicon (device_gicons,
                                   GTK_ICON_SIZE_SMALL_TOOLBAR);

  device_name = device_kind_to_localised_string (kind);

  build_device_time_details (device_name, time, state, percentage, &short_details, &details, &accesible_name);

  /* Create menu item */
  item = gtk_image_menu_item_new ();

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_attach (GTK_GRID (grid), icon, 0, 0, 1, 1);
  details_label = gtk_label_new (details);
  gtk_grid_attach_next_to (GTK_GRID (grid), details_label, icon, GTK_POS_RIGHT, 1, 1);
  gtk_container_add (GTK_CONTAINER (item), grid);
  gtk_widget_show (grid);

  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  g_signal_connect (G_OBJECT (item), "activate",
                    G_CALLBACK (show_info_cb), NULL);

  g_free (short_details);
  g_free (details);
  g_free (accesible_name);
  g_free (device_icon);
  g_free (object_path);
}

static gsize
menu_add_devices (GtkMenu  *menu,
                  GVariant *devices)
{
  GVariant *device;
  gsize n_devices;
  guint i;

  if (devices == NULL)
    return 0;

  n_devices = g_variant_n_children (devices);
  g_debug ("Num devices: '%" G_GSIZE_FORMAT "'\n", n_devices);

  for (i = 0; i < n_devices; i++)
    {
      device = g_variant_get_child_value (devices, i);
      menu_add_device (menu, device);
    }

  return n_devices;
}

static gboolean
get_greeter_mode (void)
{
  const gchar *var;
  var = g_getenv("INDICATOR_GREETER_MODE");
  return (g_strcmp0(var, "1") == 0);
}

static void
build_menu (IndicatorPower *self)
{
  IndicatorPowerPrivate *priv = self->priv;
  GtkWidget *item;
  GtkWidget *image;
  GList *children;
  gsize n_devices = 0;
  gboolean visible;

  if (priv->menu == NULL)
    priv->menu = GTK_MENU (gtk_menu_new ());

  children = gtk_container_get_children (GTK_CONTAINER (priv->menu));
  g_list_foreach (children, (GFunc) gtk_widget_destroy, NULL);
  g_list_free (children);

  /* devices */
  n_devices = menu_add_devices (priv->menu, priv->devices);

  if (!get_greeter_mode ()) {
    /* only do the separator if we have at least one device */
    if (n_devices != 0)
      {
        item = gtk_separator_menu_item_new ();
        gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), item);
      }

    /* options */
    item = gtk_check_menu_item_new_with_label (_("Show Time in Menu Bar"));
    g_signal_connect (G_OBJECT (item), "toggled",
                      G_CALLBACK (option_toggled_cb), self);
    visible = g_settings_get_boolean (priv->settings, "show-time");
    gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), visible);
    gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), item);

    /* preferences */
    item = gtk_image_menu_item_new_with_label (_("Power Settings..."));
    image = gtk_image_new_from_icon_name (GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
    g_signal_connect (G_OBJECT (item), "activate",
                      G_CALLBACK (show_preferences_cb), NULL);
    gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), item);
  }

  /* show the menu */
  gtk_widget_show_all (GTK_WIDGET (priv->menu));
}

static GVariant *
get_primary_device (GVariant *devices)
{
  UpDeviceKind kind;
  UpDeviceState state;
  GVariant *device;
  GVariant *primary_device_charging = NULL;
  GVariant *primary_device_discharging = NULL;
  GVariant *primary_device = NULL;
  gboolean charging = FALSE;
  gboolean discharging = FALSE;
  gchar *object_path;
  gchar *device_icon;
  gdouble percentage;
  guint64 time;
  guint64 min_discharging_time = G_MAXUINT64;
  guint64 max_charging_time = 0;
  gsize n_devices;
  guint i;

  n_devices = g_variant_n_children (devices);
  g_debug ("Num devices: '%" G_GSIZE_FORMAT "'\n", n_devices);

  for (i = 0; i < n_devices; i++)
    {
      time = 0;
      device = g_variant_get_child_value (devices, i);
      g_variant_get (device,
                     "(susdut)",
                     &object_path,
                     &kind,
                     &device_icon,
                     &percentage,
                     &state,
                     &time);

      g_debug ("%s: got data from object %s", G_STRFUNC, object_path);

      /* Try to fix the case when we get a empty battery bay as a real battery */
      if (state == UP_DEVICE_STATE_UNKNOWN &&
          percentage == 0)
        continue;

      /* not battery */
      if (kind != UP_DEVICE_KIND_BATTERY)
        continue;

      if (state == UP_DEVICE_STATE_DISCHARGING)
        {
          discharging = TRUE;
          if (time < min_discharging_time)
            {
              min_discharging_time = time;
              primary_device_discharging = device;
            }
        }
      else if (state == UP_DEVICE_STATE_CHARGING)
        {
          charging = TRUE;
          if (time == 0) /* Battery broken */
            {
              primary_device_charging = device;
            }
          if (time > max_charging_time)
            {
              max_charging_time = time;
              primary_device_charging = device;
            }
        }
      else
        {
          primary_device = device;
        }

      g_free (device_icon);
      g_free (object_path);
    }

  if (discharging)
    {
      primary_device = primary_device_discharging;
    }
  else if (charging)
    {
      primary_device = primary_device_charging;
    }

  return primary_device;
}

static void
put_primary_device (IndicatorPower *self,
                    GVariant *device)
{
  IndicatorPowerPrivate *priv = self->priv;
  UpDeviceKind kind;
  UpDeviceState state;
  GIcon *device_gicons;
  gchar *short_details = NULL;
  gchar *details = NULL;
  gchar *accesible_name = NULL;
  gchar *device_icon = NULL;
  gchar *object_path = NULL;
  gdouble percentage;
  guint64 time;
  const gchar *device_name;

  /* set the icon and text */
  g_variant_get (device,
                 "(susdut)",
                 &object_path,
                 &kind,
                 &device_icon,
                 &percentage,
                 &state,
                 &time);

  g_debug ("%s: got data from object %s", G_STRFUNC, object_path);

  /* set icon */
  device_gicons = get_device_icon (kind, state, time, device_icon);
  gtk_image_set_from_gicon (priv->status_image,
                            device_gicons,
                            GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_widget_show (GTK_WIDGET (priv->status_image));


  /* get the device name */
  device_name = device_kind_to_localised_string (kind);

  /* get the description */
  build_device_time_details (device_name, time, state, percentage, &short_details, &details, &accesible_name);

  gtk_label_set_label (GTK_LABEL (priv->label),
                       short_details);
  set_accessible_desc (self, accesible_name);

  g_free (short_details);
  g_free (details);
  g_free (accesible_name);
  g_free (device_icon);
  g_free (object_path);
}

static void
get_devices_cb (GObject      *source_object,
                GAsyncResult *res,
                gpointer      user_data)
{
  IndicatorPower *self = INDICATOR_POWER (user_data);
  IndicatorPowerPrivate *priv = self->priv;
  GVariant *devices_container;
  GError *error = NULL;

  devices_container = g_dbus_proxy_call_finish (G_DBUS_PROXY (source_object), res, &error);
  if (devices_container == NULL)
    {
      g_printerr ("Error getting devices: %s\n", error->message);
      g_error_free (error);

      return;
    }
  priv->devices = g_variant_get_child_value (devices_container, 0);
  g_variant_unref (devices_container);

  priv->device = get_primary_device (priv->devices);
  if (priv->device == NULL)
    {
      g_printerr ("Error getting primary device");

      return;
    }

  put_primary_device (self, priv->device);

  build_menu (self);
}

static void
receive_signal (GDBusProxy *proxy,
                gchar      *sender_name,
                gchar      *signal_name,
                GVariant   *parameters,
                gpointer    user_data)
{
  IndicatorPower *self = INDICATOR_POWER (user_data);
  IndicatorPowerPrivate *priv = self->priv;

  if (g_strcmp0 (signal_name, "Changed") == 0)
    {
      /* get the new state */
      g_dbus_proxy_call (priv->proxy,
                         "GetDevices",
                         NULL,
                         G_DBUS_CALL_FLAGS_NONE,
                         -1,
                         priv->proxy_cancel,
                         get_devices_cb,
                         user_data);
    }
}

static void
service_proxy_cb (GObject      *object,
                  GAsyncResult *res,
                  gpointer      user_data)
{
  IndicatorPower *self = INDICATOR_POWER (user_data);
  IndicatorPowerPrivate *priv = self->priv;
  GError *error = NULL;

  priv->proxy = g_dbus_proxy_new_for_bus_finish (res, &error);

  if (priv->proxy_cancel != NULL)
    {
      g_object_unref (priv->proxy_cancel);
      priv->proxy_cancel = NULL;
    }

  if (error != NULL)
    {
      g_error ("Error creating proxy: %s", error->message);
      g_error_free (error);

      return;
    }

  /* we want to change the primary device changes */
  g_signal_connect (priv->proxy,
                    "g-signal",
                    G_CALLBACK (receive_signal),
                    user_data);

  /* get the initial state */
  g_dbus_proxy_call (priv->proxy,
                     "GetDevices",
                     NULL,
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     priv->proxy_cancel,
                     get_devices_cb,
                     user_data);
}

static void
gsd_appeared_callback (GDBusConnection *connection,
                       const gchar     *name,
                       const gchar     *name_owner,
                       gpointer         user_data)
{
  IndicatorPower *self = INDICATOR_POWER (user_data);
  IndicatorPowerPrivate *priv = self->priv;

  priv->proxy_cancel = g_cancellable_new ();

  g_dbus_proxy_new (connection,
                    G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
                    NULL,
                    name,
                    POWER_DBUS_PATH,
                    POWER_DBUS_INTERFACE,
                    priv->proxy_cancel,
                    service_proxy_cb,
                    self);
}




/* Grabs the label. Creates it if it doesn't
   exist already */
static GtkLabel *
get_label (IndicatorObject *io)
{
  IndicatorPower *self = INDICATOR_POWER (io);
  IndicatorPowerPrivate *priv = self->priv;

  if (priv->label == NULL)
    {
      /* Create the label if it doesn't exist already */
      priv->label = GTK_LABEL (gtk_label_new (""));
      gtk_widget_set_visible (GTK_WIDGET (priv->label), FALSE);
    }

  return priv->label;
}

static GtkImage *
get_image (IndicatorObject *io)
{
  IndicatorPower *self = INDICATOR_POWER (io);
  IndicatorPowerPrivate *priv = self->priv;
  GIcon *gicon;

  if (priv->status_image == NULL)
  {
    /* Will create the status icon if it doesn't exist already */
    gicon = g_themed_icon_new (DEFAULT_ICON);
    priv->status_image = GTK_IMAGE (gtk_image_new_from_gicon (gicon,
                                                              GTK_ICON_SIZE_LARGE_TOOLBAR));
  }

  return priv->status_image;
}

static GtkMenu *
get_menu (IndicatorObject *io)
{
  IndicatorPower *self = INDICATOR_POWER (io);
  IndicatorPowerPrivate *priv = self->priv;

  build_menu (self);

  return GTK_MENU (priv->menu);
}

static const gchar *
get_accessible_desc (IndicatorObject *io)
{
  IndicatorPower *self = INDICATOR_POWER (io);
  IndicatorPowerPrivate *priv = self->priv;

  return priv->accessible_desc;
}

static const gchar *
get_name_hint (IndicatorObject *io)
{
  return PACKAGE_NAME;
}
