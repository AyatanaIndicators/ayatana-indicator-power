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
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <gio/gio.h>

#include "dbus-listener.h"
#include "device.h"
#include "indicator-power.h"

#define ICON_POLICY_KEY "icon-policy"

#define DEFAULT_ICON   "gpm-battery-missing"

enum {
  POWER_INDICATOR_ICON_POLICY_PRESENT,
  POWER_INDICATOR_ICON_POLICY_CHARGE,
  POWER_INDICATOR_ICON_POLICY_NEVER
};

struct _IndicatorPowerPrivate
{
  GtkMenu   *menu;

  GtkLabel *label;
  GtkImage *status_image;
  gchar    *accessible_desc;

  IndicatorPowerDbusListener * dbus_listener;

  GSList * devices;
  IndicatorPowerDevice * device;

  GSettings *settings;
};


/* LCOV_EXCL_START */
INDICATOR_SET_VERSION
INDICATOR_SET_TYPE (INDICATOR_POWER_TYPE)
/* LCOV_EXCL_STOP */

/* Prototypes */
static void             indicator_power_dispose         (GObject *object);
static void             indicator_power_finalize        (GObject *object);

static GtkLabel*        get_label                       (IndicatorObject * io);
static GtkImage*        get_image                       (IndicatorObject * io);
static GtkMenu*         get_menu                        (IndicatorObject * io);
static const gchar*     get_accessible_desc             (IndicatorObject * io);
static const gchar*     get_name_hint                   (IndicatorObject * io);

static void             update_visibility               (IndicatorPower * self);
static gboolean         should_be_visible               (IndicatorPower * self);

static void             on_entry_added                  (IndicatorObject * io, IndicatorObjectEntry * entry, gpointer user_data);

/*
static void gsd_appeared_callback (GDBusConnection *connection, const gchar *name, const gchar *name_owner, gpointer user_data);
*/

/* LCOV_EXCL_START */
G_DEFINE_TYPE (IndicatorPower, indicator_power, INDICATOR_OBJECT_TYPE);
/* LCOV_EXCL_STOP */

static void
indicator_power_class_init (IndicatorPowerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  IndicatorObjectClass *io_class = INDICATOR_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (IndicatorPowerPrivate));

  object_class->dispose = indicator_power_dispose;
  object_class->finalize = indicator_power_finalize;

  io_class->get_label = get_label;
  io_class->get_image = get_image;
  io_class->get_menu = get_menu;
  io_class->get_accessible_desc = get_accessible_desc;
  io_class->get_name_hint = get_name_hint;
}

static void
indicator_power_init (IndicatorPower *self)
{
  IndicatorPowerPrivate * priv;

  priv = G_TYPE_INSTANCE_GET_PRIVATE (self, INDICATOR_POWER_TYPE, IndicatorPowerPrivate);

  priv->menu = GTK_MENU(gtk_menu_new());

  priv->accessible_desc = NULL;

  priv->dbus_listener = g_object_new (INDICATOR_POWER_DBUS_LISTENER_TYPE, NULL);
  g_signal_connect_swapped (priv->dbus_listener, INDICATOR_POWER_DBUS_LISTENER_DEVICES_ENUMERATED,
                            G_CALLBACK(indicator_power_set_devices), self);

  priv->settings = g_settings_new ("com.canonical.indicator.power");
  g_signal_connect_swapped (priv->settings, "changed::" ICON_POLICY_KEY,
                            G_CALLBACK(update_visibility), self);
  g_object_set (G_OBJECT(self),
                INDICATOR_OBJECT_DEFAULT_VISIBILITY, FALSE,
                NULL);

  g_signal_connect (INDICATOR_OBJECT(self), INDICATOR_OBJECT_SIGNAL_ENTRY_ADDED,
                    G_CALLBACK(on_entry_added), NULL);

  self->priv = priv;
}

static void
dispose_devices (IndicatorPower * self)
{
  IndicatorPowerPrivate * priv = self->priv;

  g_clear_object (&priv->device);
  g_slist_free_full (priv->devices, g_object_unref);
  priv->devices = NULL;
}
static void
indicator_power_dispose (GObject *object)
{
  IndicatorPower *self = INDICATOR_POWER(object);
  IndicatorPowerPrivate * priv = self->priv;

  dispose_devices (self);

  g_clear_object (&priv->label);
  g_clear_object (&priv->status_image);
  g_clear_object (&priv->dbus_listener);
  g_clear_object (&priv->settings);

  G_OBJECT_CLASS (indicator_power_parent_class)->dispose (object);
}

static void
indicator_power_finalize (GObject *object)
{
  IndicatorPower *self = INDICATOR_POWER(object);
  IndicatorPowerPrivate * priv = self->priv;

  g_free (priv->accessible_desc);

  G_OBJECT_CLASS (indicator_power_parent_class)->finalize (object);
}

/***
****
***/

static void
spawn_command_line_async (const char * command)
{
  GError * err = NULL;
  if (!g_spawn_command_line_async (command, &err))
      g_warning ("Couldn't execute command \"%s\": %s", command, err->message);
  g_clear_error (&err);
}

static void
option_toggled_cb (GtkCheckMenuItem *item, IndicatorPower * self)
{
  gtk_widget_set_visible (GTK_WIDGET (self->priv->label),
                          gtk_check_menu_item_get_active(item));
}

/* ensure that the entry is using self's accessible description */
static void
refresh_entry_accessible_desc (IndicatorPower * self, IndicatorObjectEntry * entry)
{
  const char * newval = self->priv->accessible_desc;

  if (entry->accessible_desc != newval)
  {
    g_debug ("%s: setting entry %p accessible description to '%s'", G_STRFUNC, entry, newval);
    entry->accessible_desc = newval;
    g_signal_emit (self, INDICATOR_OBJECT_SIGNAL_ACCESSIBLE_DESC_UPDATE_ID, 0, entry);
  }
}

static void
on_entry_added (IndicatorObject       * io,
                IndicatorObjectEntry  * entry,
                gpointer                user_data G_GNUC_UNUSED)
{
  refresh_entry_accessible_desc (INDICATOR_POWER(io), entry);
}

static void
set_accessible_desc (IndicatorPower *self, const gchar *desc)
{
  g_debug ("%s: setting accessible description to '%s'", G_STRFUNC, desc);

  if (desc && *desc)
  {
    /* update our copy of the string */
    char * oldval = self->priv->accessible_desc;
    self->priv->accessible_desc = g_strdup (desc);

    /* ensure that the entries are using self's accessible description */
    GList * l;
    GList * entries = indicator_object_get_entries(INDICATOR_OBJECT(self));
    for (l=entries; l!=NULL; l=l->next)
      refresh_entry_accessible_desc (self, l->data);

    /* cleanup */
    g_list_free (entries);
    g_free (oldval);
  }
}

static gboolean
menu_add_device (GtkMenu * menu, const IndicatorPowerDevice * device)
{
  gboolean added = FALSE;
  const UpDeviceKind kind = indicator_power_device_get_kind (device);

  if (kind != UP_DEVICE_KIND_LINE_POWER)
  {
    GtkWidget *icon;
    GtkWidget *item;
    GtkWidget *details_label;
    GtkWidget *grid;
    GIcon *device_gicon;
    gchar *short_details = NULL;
    gchar *details = NULL;
    gchar *accessible_name = NULL;
    AtkObject *atk_object;

    /* Process the data */
    device_gicon = indicator_power_device_get_gicon (device);
    icon = gtk_image_new_from_gicon (device_gicon, GTK_ICON_SIZE_SMALL_TOOLBAR);
    g_clear_object (&device_gicon);

    indicator_power_device_get_time_details (device, &short_details, &details, &accessible_name);

    /* Create menu item */
    item = gtk_image_menu_item_new ();
    atk_object = gtk_widget_get_accessible(item);
    if (atk_object != NULL)
      atk_object_set_name (atk_object, accessible_name);

    grid = gtk_grid_new ();
    gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
    gtk_grid_attach (GTK_GRID (grid), icon, 0, 0, 1, 1);
    details_label = gtk_label_new (details);
    gtk_grid_attach_next_to (GTK_GRID (grid), details_label, icon, GTK_POS_RIGHT, 1, 1);
    gtk_container_add (GTK_CONTAINER (item), grid);
    gtk_widget_show (grid);

    gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    added = TRUE;

    g_signal_connect_swapped (G_OBJECT (item), "activate",
                              G_CALLBACK (spawn_command_line_async), "gnome-power-statistics");

    g_free (short_details);
    g_free (details);
    g_free (accessible_name);
  }

  return added;
}

static gsize
menu_add_devices (GtkMenu * menu, GSList * devices)
{
  GSList * l;
  gsize n_added = 0;

  for (l=devices; l!=NULL; l=l->next)
    if (menu_add_device (menu, l->data))
      ++n_added;

  return n_added;
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
  GtkWidget *item;
  GtkWidget *image;
  GList *children;
  gsize n_devices = 0;
  IndicatorPowerPrivate * priv = self->priv;

  /* remove the existing menuitems */
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
    g_signal_connect (item, "toggled", G_CALLBACK(option_toggled_cb), self);
    g_settings_bind (priv->settings, "show-time", item, "active", G_SETTINGS_BIND_DEFAULT);
    gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), item);

    /* preferences */
    item = gtk_image_menu_item_new_with_label (_("Power Settings…"));
    image = gtk_image_new_from_icon_name (GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
    g_signal_connect_swapped (G_OBJECT (item), "activate",
                              G_CALLBACK (spawn_command_line_async), "gnome-control-center power");
    gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), item);
  }

  /* show the menu */
  gtk_widget_show_all (GTK_WIDGET (priv->menu));
}

/* sort devices from most interesting to least interesting on this criteria:
   1. discharging items from least time remaining until most time remaining
   2. discharging items with an unknown time remaining
   3. charging items from most time left to charge to least time left to charge
   4. charging items with an unknown time remaining
   5. everything except line-power, because it's not interesting
   6. line-power */
static gint
device_compare_func (gconstpointer ga, gconstpointer gb)
{
  int ret;
  int state;
  const IndicatorPowerDevice * a = INDICATOR_POWER_DEVICE(ga);
  const IndicatorPowerDevice * b = INDICATOR_POWER_DEVICE(gb);
  const int a_state = indicator_power_device_get_state (a);
  const int b_state = indicator_power_device_get_state (b);
  const gdouble a_percentage = indicator_power_device_get_percentage (a);
  const gdouble b_percentage = indicator_power_device_get_percentage (b);
  const time_t a_time = indicator_power_device_get_time (a);
  const time_t b_time = indicator_power_device_get_time (b);

  ret = 0;

  state = UP_DEVICE_STATE_DISCHARGING;
  if (!ret && ((a_state == state) || (b_state == state)))
    {
      if (a_state != state) /* b is discharging */
        {
          ret = 1;
        }
      else if (b_state != state) /* a is discharging */
        {
          ret = -1;
        }
      else /* both are discharging; least-time-left goes first */
        {
          if (!a_time || !b_time) /* known time always trumps unknown time */
            ret = a_time ? -1 : 1;
          else if (a_time != b_time)
            ret = a_time < b_time ? -1 : 1;
          else
            ret = a_percentage < b_percentage ? -1 : 1;
        }
    }

  state = UP_DEVICE_STATE_CHARGING;
  if (!ret && (((a_state == state) && a_time) || ((b_state == state) && b_time)))
    {
      if (a_state != state) /* b is charging */
        {
          ret = 1;
        }
      else if (b_state != state) /* a is charging */
        {
          ret = -1;
        }
      else /* both are discharging; most-time-to-charge goes first */
        {
          if (!a_time || !b_time) /* known time always trumps unknown time */
            ret = a_time ? -1 : 1;
          else if (a_time != b_time)
            ret = a_time > b_time ? -1 : 1;
          else
            ret = a_percentage < b_percentage ? -1 : 1;
        }
    }

  if (!ret) /* make UP_DEVICE_KIND_LINE_POWER go last because it's not interesting */
    {
      const UpDeviceKind a_kind = indicator_power_device_get_kind (a);
      const UpDeviceKind b_kind = indicator_power_device_get_kind (b);

      if ((a_kind == UP_DEVICE_KIND_LINE_POWER) || (b_kind == UP_DEVICE_KIND_LINE_POWER))
        {
          if (a_kind != UP_DEVICE_KIND_LINE_POWER) /* b is a line-power */
            {
              ret = -1;
            }
          else if (b_kind != UP_DEVICE_KIND_LINE_POWER) /* a is a line-power */
            {
              ret = 1;
            }
        }
    }

  if (!ret)
    ret = a_state - b_state;

  return ret;
}

IndicatorPowerDevice *
indicator_power_choose_primary_device (GSList * devices)
{
  IndicatorPowerDevice * primary = NULL;

  if (devices != NULL)
    {
      GSList * tmp;

      tmp = g_slist_copy (devices);
      tmp = g_slist_sort (tmp, device_compare_func);
      primary = g_object_ref (tmp->data);

      g_slist_free (tmp);
    }

  return primary;
}

static void
put_primary_device (IndicatorPower *self, IndicatorPowerDevice *device)
{
  IndicatorPowerPrivate * priv = self->priv;

  /* set icon */
  GIcon * device_gicon = indicator_power_device_get_gicon (device);
  gtk_image_set_from_gicon (priv->status_image, device_gicon, GTK_ICON_SIZE_LARGE_TOOLBAR);
  g_clear_object (&device_gicon);
  gtk_widget_show (GTK_WIDGET (priv->status_image));

  /* get the description */
  gchar * short_details;
  gchar * details;
  gchar * accessible_name;
  indicator_power_device_get_time_details (device, &short_details, &details, &accessible_name);
  gtk_label_set_label (GTK_LABEL (priv->label), short_details);
  set_accessible_desc (self, accessible_name);
  g_free (accessible_name);
  g_free (details);
  g_free (short_details);
}

void
indicator_power_set_devices (IndicatorPower * self, GSList * devices)
{
  /* LCOV_EXCL_START */
  g_return_if_fail (IS_INDICATOR_POWER(self));
  /* LCOV_EXCL_STOP */

  IndicatorPowerPrivate * priv = self->priv;

  /* update our devices & primary device */
  g_slist_foreach (devices, (GFunc)g_object_ref, NULL);
  dispose_devices (self);
  priv->devices = g_slist_copy (devices);
  priv->device = indicator_power_choose_primary_device (devices);

  /* and our menus/visibility from the new device list */
  if (priv->device != NULL)
      put_primary_device (self, priv->device);
  else
      g_message ("Couldn't find primary device");
  build_menu (self);
  update_visibility (self);
}

static void
update_visibility (IndicatorPower * self)
{
  indicator_object_set_visible (INDICATOR_OBJECT (self),
                                should_be_visible (self));
}


/* Grabs the label. Creates it if it doesn't
   exist already */
static GtkLabel *
get_label (IndicatorObject *io)
{
  IndicatorPower *self = INDICATOR_POWER (io);
  IndicatorPowerPrivate * priv = self->priv;

  if (priv->label == NULL)
    {
      /* Create the label if it doesn't exist already */
      priv->label = GTK_LABEL (gtk_label_new (""));
      g_object_ref_sink (priv->label);
      gtk_widget_set_visible (GTK_WIDGET (priv->label), FALSE);
    }

  return priv->label;
}

static GtkImage *
get_image (IndicatorObject *io)
{
  GIcon *gicon;
  IndicatorPower *self = INDICATOR_POWER (io);
  IndicatorPowerPrivate * priv = self->priv;

  if (priv->status_image == NULL)
  {
    /* Will create the status icon if it doesn't exist already */
    gicon = g_themed_icon_new (DEFAULT_ICON);
    priv->status_image = GTK_IMAGE (gtk_image_new_from_gicon (gicon,
                                                              GTK_ICON_SIZE_LARGE_TOOLBAR));
    g_object_ref_sink (priv->status_image);
    g_object_unref (gicon);
  }

  return priv->status_image;
}

static GtkMenu *
get_menu (IndicatorObject *io)
{
  IndicatorPower *self = INDICATOR_POWER (io);

  build_menu (self);

  return GTK_MENU (self->priv->menu);
}

static const gchar *
get_accessible_desc (IndicatorObject *io)
{
  IndicatorPower *self = INDICATOR_POWER (io);

  return self->priv->accessible_desc;
}

static const gchar *
get_name_hint (IndicatorObject *io)
{
  return PACKAGE_NAME;
}

/***
****
***/

static void
count_batteries (GSList * devices, int *total, int *inuse)
{
  GSList * l;

  for (l=devices; l!=NULL; l=l->next)
    {
      const IndicatorPowerDevice * device = INDICATOR_POWER_DEVICE(l->data);

      if (indicator_power_device_get_kind(device) == UP_DEVICE_KIND_BATTERY)
        {
          ++*total;

          const UpDeviceState state = indicator_power_device_get_state (device);
          if ((state == UP_DEVICE_STATE_CHARGING) || (state == UP_DEVICE_STATE_DISCHARGING))
            ++*inuse;
        }
    }

    g_debug("count_batteries found %d batteries (%d are charging/discharging)", *total, *inuse);
}

static gboolean
should_be_visible (IndicatorPower * self)
{
  gboolean visible = TRUE;
  IndicatorPowerPrivate * priv = self->priv;

  const int icon_policy = g_settings_get_enum (priv->settings, ICON_POLICY_KEY);

  g_debug ("icon_policy is: %d (present==0, charge==1, never==2)", icon_policy);

  if (icon_policy == POWER_INDICATOR_ICON_POLICY_NEVER)
    {
      visible = FALSE;
    }
    else
    {
      int batteries=0, inuse=0;
      count_batteries (priv->devices, &batteries, &inuse);

      if (icon_policy == POWER_INDICATOR_ICON_POLICY_PRESENT)
        {
          visible = batteries > 0;
        }
      else if (icon_policy == POWER_INDICATOR_ICON_POLICY_CHARGE)
        {
          visible = inuse > 0;
        }
    }

  g_debug ("should_be_visible: %s", visible?"yes":"no");
  return visible;
}
