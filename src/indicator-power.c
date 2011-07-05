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
#include <libindicator/indicator-image-helper.h>

#define DEFAULT_ICON   "gpm-battery-empty"

#define INDICATOR_POWER_TYPE            (indicator_power_get_type ())
#define INDICATOR_POWER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), INDICATOR_POWER_TYPE, IndicatorPower))
#define INDICATOR_POWER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), INDICATOR_POWER_TYPE, IndicatorPowerClass))
#define IS_INDICATOR_POWER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INDICATOR_POWER_TYPE))
#define IS_INDICATOR_POWER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), INDICATOR_POWER_TYPE))
#define INDICATOR_POWER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), INDICATOR_POWER_TYPE, IndicatorPowerClass))

typedef struct _IndicatorPower         IndicatorPower;
typedef struct _IndicatorPowerClass    IndicatorPowerClass;
typedef struct _IndicatorPowerPrivate  IndicatorPowerPrivate;

struct _IndicatorPower
{
  IndicatorObject parent_instance;

  IndicatorPowerPrivate *priv;
};

struct _IndicatorPowerClass
{
  IndicatorObjectClass parent_class;
};

GType indicator_power_get_type (void) G_GNUC_CONST;


struct _IndicatorPowerPrivate
{
  GtkMenu   *menu;

  GtkLabel *label;
  GtkImage *status_image;
  gchar    *accessible_desc;

  GCancellable *proxy_cancel;
  GDBusProxy   *proxy;

  GVariant *device;
};

/* Prototypes */
static void             indicator_power_class_init      (IndicatorPowerClass *klass);
static void             indicator_power_init            (IndicatorPower *self);
static void             indicator_power_dispose         (GObject *object);
static void             indicator_power_finalize        (GObject *object);

static GtkLabel*        get_label                       (IndicatorObject * io);
static GtkImage*        get_image                       (IndicatorObject * io);
static GtkMenu*         get_menu                        (IndicatorObject * io);
static const gchar*     get_accessible_desc             (IndicatorObject * io);


G_DEFINE_TYPE (IndicatorPower, indicator_power, INDICATOR_OBJECT_TYPE);


/* Indicator stuff */
INDICATOR_SET_VERSION
INDICATOR_SET_TYPE (INDICATOR_POWER_TYPE)


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

  g_type_class_add_private (klass, sizeof (IndicatorPowerPrivate));
}

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

  gtk_widget_set_visible (GTK_WIDGET (priv->label),
                          gtk_check_menu_item_get_active (item));
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
      *short_timestring = g_strdup_printf ("0:%i", minutes);
      *detailed_timestring = g_strdup_printf (ngettext ("%i minute",
                                              "%i minutes",
                                              minutes), minutes);
      return;
    }

  hours = minutes / 60;
  minutes = minutes % 60;

  *short_timestring = g_strdup_printf ("%i:%i", hours, minutes);

  if (minutes == 0)
    {
      *detailed_timestring = g_strdup_printf (ngettext (
                                              "%i hour",
                                              "%i hours",
                                              hours), hours);
    }
  else
    {
      /* TRANSLATOR: "%i %s %i %s" are "%i hours %i minutes"
       * Swap order with "%2$s %2$i %1$s %1$i if needed */
      *detailed_timestring = g_strdup_printf (_("%i %s %i %s"),
                                              hours, ngettext ("hour", "hours", hours),
                                              minutes, ngettext ("minute", "minutes", minutes));
    }
}

static const gchar *
device_kind_to_localised_string (UpDeviceKind kind)
{
  const gchar *text = NULL;

  switch (kind) {
    case UP_DEVICE_KIND_LINE_POWER:
      /* TRANSLATORS: system power cord */
      text = gettext ("AC adapter");
      break;
    case UP_DEVICE_KIND_BATTERY:
      /* TRANSLATORS: laptop primary battery */
      text = gettext ("Battery");
      break;
    case UP_DEVICE_KIND_UPS:
      /* TRANSLATORS: battery-backed AC power source */
      text = gettext ("UPS");
      break;
    case UP_DEVICE_KIND_MONITOR:
      /* TRANSLATORS: a monitor is a device to measure voltage and current */
      text = gettext ("Monitor");
      break;
    case UP_DEVICE_KIND_MOUSE:
      /* TRANSLATORS: wireless mice with internal batteries */
      text = gettext ("Mouse");
      break;
    case UP_DEVICE_KIND_KEYBOARD:
      /* TRANSLATORS: wireless keyboard with internal battery */
      text = gettext ("Keyboard");
      break;
    case UP_DEVICE_KIND_PDA:
      /* TRANSLATORS: portable device */
      text = gettext ("PDA");
      break;
    case UP_DEVICE_KIND_PHONE:
      /* TRANSLATORS: cell phone (mobile...) */
      text = gettext ("Cell phone");
      break;
    case UP_DEVICE_KIND_MEDIA_PLAYER:
      /* TRANSLATORS: media player, mp3 etc */
      text = gettext ("Media player");
      break;
    case UP_DEVICE_KIND_TABLET:
      /* TRANSLATORS: tablet device */
      text = gettext ("Tablet");
      break;
    case UP_DEVICE_KIND_COMPUTER:
      /* TRANSLATORS: tablet device */
      text = gettext ("Computer");
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
                           gchar         **details)
{
  gchar *short_timestring = NULL;
  gchar *detailed_timestring = NULL;

  if (time > 0)
    {
      get_timestring (time,
                      &short_timestring,
                      &detailed_timestring);

      *short_details = g_strdup_printf ("(%s)",
                                        short_timestring);

      if (state == UP_DEVICE_STATE_CHARGING)
        {
          /* TRANSLATORS: %2 is a time string, e.g. "1 hour 5 minutes" */
          *details = g_strdup_printf (_("%s (%s until charged (%.0lf%%))"),
                                      device_name, detailed_timestring, percentage);
        }
      else if (state == UP_DEVICE_STATE_DISCHARGING)
        {
          /* TRANSLATORS: %2 is a time string, e.g. "1 hour 5 minutes" */
          *details = g_strdup_printf (_("%s (%s until empty (%.0lf%%))"),
                                      device_name, detailed_timestring, percentage);
        }
    }
  else
    {
      /* TRANSLATORS: %2 is a percentage value. Note: this string is only
       * used when we don't have a time value */
      *details = g_strdup_printf (_("%s (%.0lf%%)"),
                                  device_name, percentage);
      *short_details = g_strdup_printf (_("(%.0lf%%)"),
                                        percentage);
    }
}

static void
set_accessible_desc (IndicatorPower *self,
                     const gchar    *desc)
{
  IndicatorPowerPrivate *priv = self->priv;

  if (desc == NULL || strlen(desc) == 0)
    return;

  g_free (priv->accessible_desc);

  priv->accessible_desc = g_strdup (desc);
}

static guint
menu_add_device (GtkMenu  *menu,
                 GVariant *device)
{
  UpDeviceKind kind;
  UpDeviceState state;
  GtkWidget *icon;
  GtkWidget *item;
  gchar *device_icon = NULL;
  gchar *object_path = NULL;
  gdouble percentage;
  guint64 time;
  const gchar *device_name;
  gchar *short_details = NULL;
  gchar *details = NULL;
  guint n_devices = 0;

  if (device == NULL)
    return n_devices;

  g_variant_get (device,
                 "((susdut))",
                 &object_path,
                 &kind,
                 &device_icon,
                 &percentage,
                 &state,
                 &time);

  g_debug ("%s: got data from object %s", G_STRFUNC, object_path);

  n_devices++;

  icon = gtk_image_new_from_icon_name (device_icon, GTK_ICON_SIZE_MENU);
  device_name = device_kind_to_localised_string (kind);

  build_device_time_details (device_name, time, state, percentage, &short_details, &details);

  item = gtk_image_menu_item_new ();
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), icon);
  gtk_menu_item_set_label (GTK_MENU_ITEM (item), details);
  gtk_image_menu_item_set_always_show_image (GTK_IMAGE_MENU_ITEM (item), TRUE);
  g_signal_connect (G_OBJECT (item), "activate",
                    G_CALLBACK (show_info_cb), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

  return n_devices;
}

static void
build_menu (IndicatorPower *self)
{
  IndicatorPowerPrivate *priv = self->priv;
  GtkWidget *item;
  GtkWidget *image;
  guint n_devices = 0;

  priv->menu = GTK_MENU (gtk_menu_new ());

  /* devices */
  n_devices += menu_add_device (priv->menu, priv->device);

  g_print ("Num devices: %d\n", n_devices);

  /* only do the separator if we have at least one device */
  if (n_devices != 0)
    {
      item = gtk_separator_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), item);
    }

  /* options */
  item = gtk_check_menu_item_new_with_label (_("Show Time Remining"));
  g_object_set (item, "draw-as-radio", TRUE, NULL);
  g_signal_connect (G_OBJECT (item), "toggled",
                    G_CALLBACK (option_toggled_cb), self);
  gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), item);

  /* separator */
  item = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), item);

  /* preferences */
  item = gtk_image_menu_item_new_with_mnemonic (_("Power Settings ..."));
  image = gtk_image_new_from_icon_name (GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU);
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
  g_signal_connect (G_OBJECT (item), "activate",
                    G_CALLBACK (show_preferences_cb), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), item);

  /* show the menu */
  gtk_widget_show_all (GTK_WIDGET (priv->menu));
}

static void
get_primary_device_cb (GObject      *source_object,
                       GAsyncResult *res,
                       gpointer      user_data)
{
  IndicatorPower *self = INDICATOR_POWER (user_data);
  IndicatorPowerPrivate *priv = self->priv;
  UpDeviceKind kind;
  UpDeviceState state;
  GError *error = NULL;
  gchar *short_details = NULL;
  gchar *details = NULL;
  gchar **device_icons;
  gchar *device_icon = NULL;
  gchar *object_path = NULL;
  gdouble percentage;
  guint64 time;
  const gchar *device_name;
  gchar *short_timestring = NULL;
  gchar *detailed_timestring = NULL;

  priv->device = g_dbus_proxy_call_finish (G_DBUS_PROXY (source_object), res, &error);
  if (priv->device == NULL)
    {
      g_printerr ("Error getting primary device: %s\n", error->message);
      g_error_free (error);

      return;
    }

  /* set the icon and text */
  g_variant_get (priv->device,
                 "((susdut))",
                 &object_path,
                 &kind,
                 &device_icon,
                 &percentage,
                 &state,
                 &time);

  g_debug ("got data from object %s", object_path);

  /* set icon */
  device_icons = g_strsplit (device_icon, " ", -1);
  indicator_image_helper_update (priv->status_image,
                                 device_icons[3]);
  g_strfreev (device_icons);
  gtk_widget_show (GTK_WIDGET (priv->status_image));

  /* get the device name */
  device_name = device_kind_to_localised_string (kind);

  /* get the description */
  build_device_time_details (device_name, time, state, percentage, &short_details, &details);

  gtk_label_set_label (GTK_LABEL (priv->label),
                       short_details);
  set_accessible_desc (self, details);

  build_menu (self);

  g_free (short_details);
  g_free (details);
  g_free (device_icon);
  g_free (short_timestring);
  g_free (detailed_timestring);
  g_free (object_path);
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
                         "GetPrimaryDevice",
                         NULL,
                         G_DBUS_CALL_FLAGS_NONE,
                         -1,
                         priv->proxy_cancel,
                         get_primary_device_cb,
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
                     "GetPrimaryDevice",
                     NULL,
                     G_DBUS_CALL_FLAGS_NONE,
                     -1,
                     priv->proxy_cancel,
                     get_primary_device_cb,
                     user_data);
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

  priv->proxy_cancel = g_cancellable_new();

  g_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                            G_DBUS_PROXY_FLAGS_NONE,
                            NULL,
                            "org.gnome.PowerManager",
                            "/org/gnome/PowerManager",
                            "org.gnome.PowerManager",
                            priv->proxy_cancel,
                            service_proxy_cb,
                            self);
}

static void
indicator_power_dispose (GObject *object)
{
  G_OBJECT_CLASS (indicator_power_parent_class)->dispose (object);
}

static void
indicator_power_finalize (GObject *object)
{
  G_OBJECT_CLASS (indicator_power_parent_class)->finalize (object);
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
    }

  return priv->label;
}

static GtkImage *
get_image (IndicatorObject *io)
{
  IndicatorPower *self = INDICATOR_POWER (io);
  IndicatorPowerPrivate *priv = self->priv;

  if (priv->status_image == NULL)
  {
    /* Will create the status icon if it doesn't exist already */
    priv->status_image = indicator_image_helper (DEFAULT_ICON);
    gtk_widget_show (GTK_WIDGET (priv->status_image));
  }

  return priv->status_image;
}

static void
build_menu_cb (GtkWidget * menu,
               G_GNUC_UNUSED GParamSpec *pspec,
               gpointer user_data)
{
  IndicatorPower *self = INDICATOR_POWER (user_data);

  build_menu (self);
}

static GtkMenu *
get_menu (IndicatorObject *io)
{
  IndicatorPower *self = INDICATOR_POWER (io);
  IndicatorPowerPrivate *priv = self->priv;

  build_menu (self);

  g_signal_connect (priv->menu, "notify::visible",
                    G_CALLBACK (build_menu_cb), self);

  return GTK_MENU (priv->menu);
}

static const gchar *
get_accessible_desc (IndicatorObject *io)
{
  IndicatorPower *self = INDICATOR_POWER (io);
  IndicatorPowerPrivate *priv = self->priv;

  return priv->accessible_desc;
}
