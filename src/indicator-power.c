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

  GCancellable *proxy_cancel;
  GDBusProxy   *proxy;
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

static gchar *
get_timestring (guint64 time_secs)
{
  gchar* timestring = NULL;
  gint  hours;
  gint  minutes;

  /* Add 0.5 to do rounding */
  minutes = (int) ( ( time_secs / 60.0 ) + 0.5 );

  if (minutes == 0)
    {
      timestring = g_strdup (_("Unknown time"));
      return timestring;
    }

  if (minutes < 60)
    {
      timestring = g_strdup_printf (ngettext ("%i minute",
                                    "%i minutes",
                                    minutes), minutes);
      return timestring;
    }

  hours = minutes / 60;
  minutes = minutes % 60;

  if (minutes == 0)
    {
      timestring = g_strdup_printf (ngettext (
                                    "%i hour",
                                    "%i hours",
                                    hours), hours);
      return timestring;
    }

  /* TRANSLATOR: "%i %s %i %s" are "%i hours %i minutes"
   * Swap order with "%2$s %2$i %1$s %1$i if needed */
  timestring = g_strdup_printf (_("%i %s %i %s"),
                                hours, ngettext ("hour", "hours", hours),
                                minutes, ngettext ("minute", "minutes", minutes));
  return timestring;
}

static void
get_primary_device_cb (GObject      *source_object,
                       GAsyncResult *res,
                       gpointer      user_data)
{
  IndicatorPowerPrivate *priv = INDICATOR_POWER (user_data)->priv;
  UpDeviceKind kind;
  UpDeviceState state;
  GVariant *result;
  GError *error = NULL;
  const gchar *title = NULL;
  gchar *details = NULL;
  gchar **device_icons;
  gchar *device_icon = NULL;
  gchar *object_path = NULL;
  gdouble percentage;
  guint64 time;
  gchar *time_string = NULL;

  result = g_dbus_proxy_call_finish (G_DBUS_PROXY (source_object), res, &error);
  if (result == NULL)
    {
      g_printerr ("Error getting primary device: %s\n", error->message);
      g_error_free (error);

      return;
    }

  /* set the icon and text */
  g_variant_get (result,
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

  /* get the title
   * translate it as it has limited entries as devices that are
   * fully charged are not returned as the primary device */
  if (kind == UP_DEVICE_KIND_BATTERY)
    {
      switch (state)
        {
          case UP_DEVICE_STATE_CHARGING:
            title = _("Battery charging");
            break;
          case UP_DEVICE_STATE_DISCHARGING:
            title = _("Battery discharging");
            break;
          default:
            break;
        }
    }
  else if (kind == UP_DEVICE_KIND_UPS)
    {
      switch (state)
        {
          case UP_DEVICE_STATE_CHARGING:
            title = _("UPS charging");
            break;
          case UP_DEVICE_STATE_DISCHARGING:
            title = _("UPS discharging");
            break;
          default:
            break;
        }
    }

  /* get the description */
  if (time > 0)
    {
      time_string = get_timestring (time);

      if (state == UP_DEVICE_STATE_CHARGING)
        {
          /* TRANSLATORS: %1 is a time string, e.g. "1 hour 5 minutes" */
          details = g_strdup_printf(_("%s until charged (%.0lf%%)"),
                                    time_string, percentage);
        }
      else if (state == UP_DEVICE_STATE_DISCHARGING)
        {
          /* TRANSLATORS: %1 is a time string, e.g. "1 hour 5 minutes" */
          details = g_strdup_printf(_("%s until empty (%.0lf%%)"),
                                    time_string, percentage);
        }
    }
  else
    {
      /* TRANSLATORS: %1 is a percentage value. Note: this string is only
       * used when we don't have a time value */
      details = g_strdup_printf(_("%.0lf%% charged"),
                                percentage);
    }

  g_free (details);
  g_free (device_icon);
  g_free (time_string);
  g_free (object_path);
  g_variant_unref (result);
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
  /*TODO*/
}

static void
show_preferences_cb (GtkMenuItem *item,
                     gpointer     data)
{
  const gchar *command = "gnome-power-preferences";

  if (g_spawn_command_line_async (command, NULL) == FALSE)
    g_warning ("Couldn't execute command: %s", command);
}

static void
build_menu (IndicatorPower *self)
{
  IndicatorPowerPrivate *priv = self->priv;
  GtkWidget *item;
  GtkWidget *image;
  guint n_devices = 1; /*TODO*/

  priv->menu = GTK_MENU (gtk_menu_new ());

  item = gtk_image_menu_item_new_from_stock ("battery", NULL);
  gtk_menu_item_set_label (GTK_MENU_ITEM (item), "Battery Remaining: 0:45s");  /*TODO*/
  g_signal_connect (G_OBJECT (item), "activate",
                    G_CALLBACK (show_info_cb), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), item);

  /* only do the seporator if we have at least one device */
  if (n_devices != 0)
    {
      item = gtk_separator_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), item);
    }

  /* options */
  item = gtk_check_menu_item_new_with_label (_("Icon Only"));
  g_object_set (item, "draw-as-radio", TRUE, NULL);
  g_signal_connect (G_OBJECT (item), "toggled",
                    G_CALLBACK (option_toggled_cb), item);
  gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), item);

  item = gtk_check_menu_item_new_with_label (_("Time Remining"));
  g_object_set (item, "draw-as-radio", TRUE, NULL);
  g_signal_connect (G_OBJECT (item), "toggled",
                    G_CALLBACK (option_toggled_cb), item);
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
indicator_power_init (IndicatorPower *self)
{
  IndicatorPowerPrivate *priv;

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            INDICATOR_POWER_TYPE,
                                            IndicatorPowerPrivate);
  priv = self->priv;

  /* Init variables */
  priv->menu = NULL;

  build_menu (self);

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
    priv->label = GTK_LABEL (gtk_label_new ("Power"));
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

static GtkMenu *
get_menu (IndicatorObject *io)
{
  IndicatorPower *self = INDICATOR_POWER (io);
  IndicatorPowerPrivate *priv = self->priv;

  return GTK_MENU (priv->menu);
}

static const gchar *
get_accessible_desc (IndicatorObject *io)
{
  IndicatorPower *self = INDICATOR_POWER (io);
  IndicatorPowerPrivate *priv = self->priv;
  const gchar *name;

  if (priv->label != NULL)
  {
    name = gtk_label_get_text (priv->label);

    return name;
  }

  return NULL;
}
