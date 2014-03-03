/*
 * Copyright 2013 Canonical Ltd.
 *
 * Authors:
 *   Charles Kerr <charles.kerr@canonical.com>
 *   Ted Gould <ted@canonical.com>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib/gi18n.h>
#include <gio/gio.h>
#include <url-dispatcher.h>

#include "device.h"
#include "device-provider.h"
#include "ib-brightness-control.h"
#include "ib-brightness-powerd-control.h"
#include "service.h"

#define BUS_NAME "com.canonical.indicator.power"
#define BUS_PATH "/com/canonical/indicator/power"

#define SETTINGS_SHOW_TIME_S "show-time"
#define SETTINGS_ICON_POLICY_S "icon-policy"
#define SETTINGS_SHOW_PERCENTAGE_S "show-percentage"

G_DEFINE_TYPE (IndicatorPowerService,
               indicator_power_service,
               G_TYPE_OBJECT)

enum
{
  SIGNAL_NAME_LOST,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

enum
{
  PROP_0,
  PROP_DEVICE_PROVIDER,
  LAST_PROP
};

static GParamSpec * properties[LAST_PROP];

enum
{
  SECTION_HEADER    = (1<<0),
  SECTION_DEVICES   = (1<<1),
  SECTION_SETTINGS  = (1<<2),
};

enum
{
  PROFILE_PHONE,
  PROFILE_DESKTOP,
  PROFILE_DESKTOP_GREETER,
  N_PROFILES
};

static const char * const menu_names[N_PROFILES] =
{
  "phone",
  "desktop",
  "desktop_greeter"
};

enum
{
  POWER_INDICATOR_ICON_POLICY_PRESENT,
  POWER_INDICATOR_ICON_POLICY_CHARGE,
  POWER_INDICATOR_ICON_POLICY_NEVER
};

struct ProfileMenuInfo
{
  /* the root level -- the header is the only child of this */
  GMenu * menu;

  /* parent of the sections. This is the header's submenu */
  GMenu * submenu;

  guint export_id;
};

struct _IndicatorPowerServicePrivate
{
  GCancellable * cancellable;

  GSettings * settings;

  IbBrightnessControl * brightness_control;
  IbBrightnessPowerdControl * brightness_powerd_control;

  guint own_id;
  guint actions_export_id;
  GDBusConnection * conn;

  struct ProfileMenuInfo menus[N_PROFILES];

  GSimpleActionGroup * actions;
  GSimpleAction * header_action;
  GSimpleAction * battery_level_action;
  GSimpleAction * brightness_action;

  IndicatorPowerDevice * primary_device;
  GList * devices; /* IndicatorPowerDevice */

  IndicatorPowerDeviceProvider * device_provider;
};

typedef IndicatorPowerServicePrivate priv_t;

/***
****
****  DEVICES
****
***/

/* the higher the weight, the more interesting the device */
static int
get_device_kind_weight (const IndicatorPowerDevice * device)
{
  UpDeviceKind kind;
  static gboolean initialized = FALSE;
  static int weights[UP_DEVICE_KIND_LAST];

  kind = indicator_power_device_get_kind (device);
  g_return_val_if_fail (0<=kind && kind<UP_DEVICE_KIND_LAST, 0);

  if (G_UNLIKELY(!initialized))
    {
      int i;

      initialized = TRUE;

      for (i=0; i<UP_DEVICE_KIND_LAST; i++)
        weights[i] = 1;
      weights[UP_DEVICE_KIND_BATTERY] = 2;
      weights[UP_DEVICE_KIND_LINE_POWER] = 0;
    }

  return weights[kind];
}

/* sort devices from most interesting to least interesting on this criteria:
   1. discharging items from least time remaining until most time remaining
   2. discharging items with an unknown time remaining
   3. charging items from most time left to charge to least time left to charge
   4. charging items with an unknown time remaining
   5. batteries, then non-line power, then line-power */
static gint
device_compare_func (gconstpointer ga, gconstpointer gb)
{
  int ret;
  int state;
  const IndicatorPowerDevice * a = ga;
  const IndicatorPowerDevice * b = gb;
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
  if (!ret && (((a_state == state) && a_time) ||
               ((b_state == state) && b_time)))
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

  if (!ret) /* neither device is charging nor discharging... */
    {
      const int weight_a = get_device_kind_weight (a);
      const int weight_b = get_device_kind_weight (b);

      if (weight_a > weight_b)
        {
          ret = -1;
        }
      else if (weight_a < weight_b)
        {
          ret = 1;
        }
    }

  if (!ret)
    ret = a_state - b_state;

  return ret;
}

/***
****
****  HEADER SECTION
****
***/

static void
count_batteries (GList * devices, int *total, int *inuse)
{
  GList * l;

  for (l=devices; l!=NULL; l=l->next)
    {
      const IndicatorPowerDevice * device = INDICATOR_POWER_DEVICE(l->data);

      if (indicator_power_device_get_kind(device) == UP_DEVICE_KIND_BATTERY ||
          indicator_power_device_get_kind(device) == UP_DEVICE_KIND_UPS)
        {
          ++*total;

          const UpDeviceState state = indicator_power_device_get_state (device);
          if ((state == UP_DEVICE_STATE_CHARGING) ||
              (state == UP_DEVICE_STATE_DISCHARGING))
            ++*inuse;
        }
    }

  g_debug ("count_batteries found %d batteries (%d are charging/discharging)",
           *total, *inuse);
}

static gboolean
should_be_visible (IndicatorPowerService * self)
{
  gboolean visible = TRUE;
  priv_t * p = self->priv;

  const int policy = g_settings_get_enum (p->settings, SETTINGS_ICON_POLICY_S);
  g_debug ("policy is: %d (present==0, charge==1, never==2)", policy);

  if (policy == POWER_INDICATOR_ICON_POLICY_NEVER)
    {
      visible = FALSE;
    }
    else
    {
      int batteries=0, inuse=0;

      count_batteries (p->devices, &batteries, &inuse);

      if (policy == POWER_INDICATOR_ICON_POLICY_PRESENT)
        {
          visible = batteries > 0;
        }
      else if (policy == POWER_INDICATOR_ICON_POLICY_CHARGE)
        {
          visible = inuse > 0;
        }
    }

  g_debug ("should_be_visible: %s", visible?"yes":"no");
  return visible;
}

static GVariant *
create_header_state (IndicatorPowerService * self)
{
  GVariantBuilder b;
  gchar * label = NULL;
  gchar * a11y = NULL;
  GIcon * icon = NULL;
  priv_t * p = self->priv;

  if (p->primary_device != NULL)
    {
      indicator_power_device_get_header (p->primary_device,
                                         g_settings_get_boolean (p->settings, SETTINGS_SHOW_TIME_S),
                                         g_settings_get_boolean (p->settings, SETTINGS_SHOW_PERCENTAGE_S),
                                         &label,
                                         &a11y);

      icon = indicator_power_device_get_gicon (p->primary_device);
    }

  g_variant_builder_init (&b, G_VARIANT_TYPE("a{sv}"));

  g_variant_builder_add (&b, "{sv}", "title", g_variant_new_string (_("Battery")));

  g_variant_builder_add (&b, "{sv}", "visible",
                         g_variant_new_boolean (should_be_visible (self)));

  if (label != NULL)
    g_variant_builder_add (&b, "{sv}", "label", g_variant_new_take_string (label));

  if (icon != NULL)
    {
      GVariant * v;

      if ((v = g_icon_serialize (icon)))
        {
          g_variant_builder_add (&b, "{sv}", "icon", v);
          g_variant_unref (v);
        }

      g_object_unref (icon);
    }

  if (a11y != NULL)
    g_variant_builder_add (&b, "{sv}", "accessible-desc", g_variant_new_take_string (a11y));

  return g_variant_builder_end (&b);
}


/***
****
****  DEVICE SECTION
****
***/

static void
append_device_to_menu (GMenu * menu, const IndicatorPowerDevice * device)
{
  const UpDeviceKind kind = indicator_power_device_get_kind (device);

  if (kind != UP_DEVICE_KIND_LINE_POWER)
  {
    char * label;
    GMenuItem * item;
    GIcon * icon;

    label = indicator_power_device_get_label (device);
    item = g_menu_item_new (label, "indicator.activate-statistics");
    g_free (label);
    g_menu_item_set_action_and_target(item, "indicator.activate-statistics", "s",
                                      indicator_power_device_get_object_path (device));

    if ((icon = indicator_power_device_get_gicon (device)))
      {
        GVariant * v;
        if ((v = g_icon_serialize (icon)))
          {
            g_menu_item_set_attribute_value (item, G_MENU_ATTRIBUTE_ICON, v);
            g_variant_unref (v);
          }

        g_object_unref (icon);
      }

    g_menu_append_item (menu, item);
    g_object_unref (item);
  }
}


static GMenuModel *
create_desktop_devices_section (IndicatorPowerService * self)
{
  GList * l;
  GMenu * menu = g_menu_new ();

  for (l=self->priv->devices; l!=NULL; l=l->next)
    append_device_to_menu (menu, l->data);

  return G_MENU_MODEL (menu);
}

/* https://wiki.ubuntu.com/Power#Phone
 * The spec also discusses including an item for any connected bluetooth
 * headset, but bluez doesn't appear to support Battery Level at this time */
static GMenuModel *
create_phone_devices_section (IndicatorPowerService * self G_GNUC_UNUSED)
{
  GMenu * menu;
  GMenuItem *item;

  menu = g_menu_new ();

  item = g_menu_item_new (_("Charge level"), "indicator.battery-level");
  g_menu_item_set_attribute (item, "x-canonical-type", "s", "com.canonical.indicator.progress");
  g_menu_append_item (menu, item);
  g_object_unref (item);

  return G_MENU_MODEL (menu);
}


/***
****
****  SETTINGS SECTION
****
***/

static void
get_brightness_range (IndicatorPowerService * self, gint * low, gint * high)
{
  int max = 0;
  if (self->priv->brightness_control) {
    max = ib_brightness_control_get_max_value (self->priv->brightness_control);
  } else {
    max = ib_brightness_powerd_control_get_max_value (self->priv->brightness_powerd_control);
  }
  *low  = max * 0.05; /* 5% minimum -- don't let the screen go completely dark */
  *high = max;
}

static gdouble
brightness_to_percentage (IndicatorPowerService * self, int brightness)
{
  int lo, hi;
  get_brightness_range (self,  &lo, &hi);
  return (brightness-lo) / (double)(hi-lo);
}

static int
percentage_to_brightness (IndicatorPowerService * self, double percentage)
{
  int lo, hi;
  get_brightness_range (self,  &lo, &hi);
  return (int)(lo + (percentage*(hi-lo)));
}

static GMenuItem *
create_brightness_menuitem (IndicatorPowerService * self)
{
  int lo, hi;
  GMenuItem * item;

  get_brightness_range (self,  &lo, &hi);

  item = g_menu_item_new (NULL, "indicator.brightness");
  g_menu_item_set_attribute (item, "x-canonical-type", "s", "com.canonical.unity.slider");
  g_menu_item_set_attribute (item, "min-value", "d", brightness_to_percentage (self, lo));
  g_menu_item_set_attribute (item, "max-value", "d", brightness_to_percentage (self, hi));
  g_menu_item_set_attribute (item, "min-icon", "s", "torch-off" );
  g_menu_item_set_attribute (item, "max-icon", "s", "torch-on" );

  return item;
}

static GVariant *
action_state_for_brightness (IndicatorPowerService * self)
{
  priv_t * p = self->priv;
  gint brightness = 0;
  if (p->brightness_control)
    {
      brightness = ib_brightness_control_get_value (p->brightness_control);
    }
  else if (p->brightness_powerd_control)
    {
      brightness = ib_brightness_powerd_control_get_value (p->brightness_powerd_control);
    }
  return g_variant_new_double (brightness_to_percentage (self, brightness));
}

static void
update_brightness_action_state (IndicatorPowerService * self)
{
  g_simple_action_set_state (self->priv->brightness_action,
                             action_state_for_brightness (self));
}

static void
on_brightness_change_requested (GSimpleAction * action      G_GNUC_UNUSED,
                                GVariant      * parameter,
                                gpointer        gself)
{
  IndicatorPowerService * self = INDICATOR_POWER_SERVICE (gself);
  const gdouble percentage = g_variant_get_double (parameter);
  const int brightness = percentage_to_brightness (self, percentage);

  if (self->priv->brightness_control)
    {
      ib_brightness_control_set_value (self->priv->brightness_control, brightness);
    }
  else if (self->priv->brightness_powerd_control)
    {
      ib_brightness_powerd_control_set_value (self->priv->brightness_powerd_control, brightness);
    }

  update_brightness_action_state (self);
}

static GMenuModel *
create_desktop_settings_section (IndicatorPowerService * self G_GNUC_UNUSED)
{
  GMenu * menu = g_menu_new ();

  g_menu_append (menu,
                 _("Show Time in Menu Bar"),
                 "indicator.show-time");

  g_menu_append (menu,
                 _("Show Percentage in Menu Bar"),
                 "indicator.show-percentage");

  g_menu_append (menu,
                 _("Power Settings…"),
                 "indicator.activate-settings");

  return G_MENU_MODEL (menu);
}

static GMenuModel *
create_phone_settings_section (IndicatorPowerService * self G_GNUC_UNUSED)
{
  GMenu * section;
  GMenuItem * item;

  section = g_menu_new ();

  item = create_brightness_menuitem (self);
  g_menu_append_item (section, item);
  update_brightness_action_state (self);
  g_object_unref (item);

  g_menu_append (section, _("Battery settings…"), "indicator.activate-phone-settings");

  return G_MENU_MODEL (section);
}

/***
****
****  SECTION REBUILDING
****
***/

/**
 * A small helper function for rebuild_now().
 * - removes the previous section
 * - adds and unrefs the new section
 */
static void
rebuild_section (GMenu * parent, int pos, GMenuModel * new_section)
{
  g_menu_remove (parent, pos);
  g_menu_insert_section (parent, pos, NULL, new_section);
  g_object_unref (new_section);
}

static void
rebuild_now (IndicatorPowerService * self, guint sections)
{
  priv_t * p = self->priv;
  struct ProfileMenuInfo * phone   = &p->menus[PROFILE_PHONE];
  struct ProfileMenuInfo * desktop = &p->menus[PROFILE_DESKTOP];
  struct ProfileMenuInfo * greeter = &p->menus[PROFILE_DESKTOP_GREETER];

  if (p->conn == NULL) /* we haven't built the menus yet */
    return;

  if (sections & SECTION_HEADER)
    {
      g_simple_action_set_state (p->header_action, create_header_state (self));
    }

  if (sections & SECTION_DEVICES)
    {
      rebuild_section (desktop->submenu, 0, create_desktop_devices_section (self));
      rebuild_section (greeter->submenu, 0, create_desktop_devices_section (self));
    }

  if (sections & SECTION_SETTINGS)
    {
      rebuild_section (desktop->submenu, 1, create_desktop_settings_section (self));
      rebuild_section (phone->submenu, 1, create_desktop_settings_section (self));
    }
}

static inline void
rebuild_header_now (IndicatorPowerService * self)
{
  rebuild_now (self, SECTION_HEADER);
}

static inline void
rebuild_devices_section_now (IndicatorPowerService * self)
{
  rebuild_now (self, SECTION_DEVICES);
}

static inline void
rebuild_settings_section_now (IndicatorPowerService * self)
{
  rebuild_now (self, SECTION_SETTINGS);
}

static void
create_menu (IndicatorPowerService * self, int profile)
{
  GMenu * menu;
  GMenu * submenu;
  GMenuItem * header;
  GMenuModel * sections[16];
  guint i;
  guint n = 0;

  g_assert (0<=profile && profile<N_PROFILES);
  g_assert (self->priv->menus[profile].menu == NULL);

  /* build the sections */

  switch (profile)
    {
      case PROFILE_PHONE:
        sections[n++] = create_phone_devices_section (self);
        sections[n++] = create_phone_settings_section (self);
        break;

      case PROFILE_DESKTOP:
        sections[n++] = create_desktop_devices_section (self);
        sections[n++] = create_desktop_settings_section (self);
        break;

      case PROFILE_DESKTOP_GREETER:
        sections[n++] = create_desktop_devices_section (self);
        break;
    }

  /* add sections to the submenu */

  submenu = g_menu_new ();

  for (i=0; i<n; ++i)
    {
      g_menu_append_section (submenu, NULL, sections[i]);
      g_object_unref (sections[i]);
    }

  /* add submenu to the header */
  header = g_menu_item_new (NULL, "indicator._header");
  g_menu_item_set_attribute (header, "x-canonical-type",
                             "s", "com.canonical.indicator.root");
  g_menu_item_set_submenu (header, G_MENU_MODEL (submenu));
  g_object_unref (submenu);

  /* add header to the menu */
  menu = g_menu_new ();
  g_menu_append_item (menu, header);
  g_object_unref (header);

  self->priv->menus[profile].menu = menu;
  self->priv->menus[profile].submenu = submenu;
}

/***
****  GActions
***/

/* Run a particular program based on an activation */
static void
execute_command (const gchar * cmd)
{
  GError * err = NULL;

  g_debug ("Issuing command '%s'", cmd);

  if (!g_spawn_command_line_async (cmd, &err))
    {
      g_warning ("Unable to start %s: %s", cmd, err->message);
      g_error_free (err);
    }
}

static void
on_settings_activated (GSimpleAction * a      G_GNUC_UNUSED,
                       GVariant      * param  G_GNUC_UNUSED,
                       gpointer        gself  G_GNUC_UNUSED)
{
  static const gchar *control_center_cmd = NULL;

  if (control_center_cmd == NULL)
    {
      if (!g_strcmp0 (g_getenv ("DESKTOP_SESSION"), "xubuntu"))
        {
          control_center_cmd = "xfce4-power-manager-settings";
        }
      else
        {
          gchar *path;

          path = g_find_program_in_path ("unity-control-center");
          if (path != NULL)
            control_center_cmd = "unity-control-center power";
          else
            control_center_cmd = "gnome-control-center power";

          g_free (path);
        }
    }

  execute_command (control_center_cmd);
}

static void
on_statistics_activated (GSimpleAction * a      G_GNUC_UNUSED,
                         GVariant      * param,
                         gpointer        gself  G_GNUC_UNUSED)
{
  char *cmd = g_strconcat ("gnome-power-statistics", " --device ",
                           g_variant_get_string (param, NULL), NULL);
  execute_command (cmd);
  g_free (cmd);
}

static void
on_phone_settings_activated (GSimpleAction * a      G_GNUC_UNUSED,
                             GVariant      * param  G_GNUC_UNUSED,
                             gpointer        gself  G_GNUC_UNUSED)
{
  url_dispatch_send("settings:///system/battery", NULL, NULL);
}

/***
****
***/

static void
init_gactions (IndicatorPowerService * self)
{
  GSimpleAction * a;
  GAction * show_time_action;
  GAction * show_percentage_action;
  priv_t * p = self->priv;

  GActionEntry entries[] = {
    { "activate-settings", on_settings_activated },
    { "activate-phone-settings", on_phone_settings_activated },
    { "activate-statistics", on_statistics_activated, "s" }
  };

  p->actions = g_simple_action_group_new ();

  g_action_map_add_action_entries (G_ACTION_MAP(p->actions),
                                   entries,
                                   G_N_ELEMENTS(entries),
                                   self);

  /* add the header action */
  a = g_simple_action_new_stateful ("_header", NULL, create_header_state (self));
  g_action_map_add_action (G_ACTION_MAP(p->actions), G_ACTION(a));
  p->header_action = a;

  /* add the power-level action */
  a = g_simple_action_new_stateful ("battery-level", NULL, g_variant_new_uint32(0));
  g_simple_action_set_enabled (a, FALSE);
  g_action_map_add_action (G_ACTION_MAP(p->actions), G_ACTION(a));
  p->battery_level_action = a;

  /* add the brightness action */
  a = g_simple_action_new_stateful ("brightness", NULL, action_state_for_brightness (self));
  g_action_map_add_action (G_ACTION_MAP(p->actions), G_ACTION(a));
  g_signal_connect (a, "change-state", G_CALLBACK(on_brightness_change_requested), self);
  p->brightness_action = a;

  /* add the show-time action */
  show_time_action = g_settings_create_action (p->settings, "show-time");
  g_action_map_add_action (G_ACTION_MAP(p->actions), show_time_action);

  /* add the show-percentage action */
  show_percentage_action = g_settings_create_action (p->settings, "show-percentage");
  g_action_map_add_action (G_ACTION_MAP(p->actions), show_percentage_action);

  rebuild_header_now (self);

  g_object_unref (show_time_action);
  g_object_unref (show_percentage_action);
}

/***
****  GDBus Name Ownership & Menu / Action Exporting
***/

static void
on_bus_acquired (GDBusConnection * connection,
                 const gchar     * name,
                 gpointer          gself)
{
  int i;
  guint id;
  GError * err = NULL;
  IndicatorPowerService * self = INDICATOR_POWER_SERVICE(gself);
  priv_t * p = self->priv;
  GString * path = g_string_new (NULL);

  g_debug ("bus acquired: %s", name);

  p->conn = g_object_ref (G_OBJECT (connection));

  /* export the actions */
  if ((id = g_dbus_connection_export_action_group (connection,
                                                   BUS_PATH,
                                                   G_ACTION_GROUP (p->actions),
                                                   &err)))
    {
      p->actions_export_id = id;
    }
  else
    {
      g_warning ("cannot export action group: %s", err->message);
      g_clear_error (&err);
    }

  /* export the menus */
  for (i=0; i<N_PROFILES; ++i)
    {
      struct ProfileMenuInfo * menu = &p->menus[i];

      g_string_printf (path, "%s/%s", BUS_PATH, menu_names[i]);

      if (menu->menu == NULL)
        create_menu (self, i);

      if ((id = g_dbus_connection_export_menu_model (connection,
                                                     path->str,
                                                     G_MENU_MODEL (menu->menu),
                                                     &err)))
        {
          menu->export_id = id;
        }
      else
        {
          g_warning ("cannot export %s menu: %s", path->str, err->message);
          g_clear_error (&err);
        }
    }

  g_string_free (path, TRUE);
}

static void
unexport (IndicatorPowerService * self)
{
  int i;
  priv_t * p = self->priv;

  /* unexport the menus */
  for (i=0; i<N_PROFILES; ++i)
    {
      guint * id = &self->priv->menus[i].export_id;

      if (*id)
        {
          g_dbus_connection_unexport_menu_model (p->conn, *id);
          *id = 0;
        }
    }

  /* unexport the actions */
  if (p->actions_export_id)
    {
      g_dbus_connection_unexport_action_group (p->conn, p->actions_export_id);
      p->actions_export_id = 0;
    }
}

static void
on_name_lost (GDBusConnection * connection G_GNUC_UNUSED,
              const gchar     * name,
              gpointer          gself)
{
  IndicatorPowerService * self = INDICATOR_POWER_SERVICE (gself);

  g_debug ("%s %s name lost %s", G_STRLOC, G_STRFUNC, name);

  unexport (self);

  g_signal_emit (self, signals[SIGNAL_NAME_LOST], 0, NULL);
}

/***
****  Events
***/

static void
on_devices_changed (IndicatorPowerService * self)
{
  priv_t * p = self->priv;
  guint32 battery_level;

  /* update the device list */
  g_list_free_full (p->devices, (GDestroyNotify)g_object_unref);
  p->devices = indicator_power_device_provider_get_devices (p->device_provider);

  /* update the primary device */
  g_clear_object (&p->primary_device);
  p->primary_device = indicator_power_service_choose_primary_device (p->devices);

  /* update the battery-level action's state */
  if (p->primary_device == NULL)
    battery_level = 0;
  else
    battery_level = (int)(indicator_power_device_get_percentage (p->primary_device) + 0.5);
  g_simple_action_set_state (p->battery_level_action, g_variant_new_uint32 (battery_level));

  rebuild_now (self, SECTION_HEADER | SECTION_DEVICES);
}


/***
****  GObject virtual functions
***/

static void
my_get_property (GObject     * o,
                  guint         property_id,
                  GValue      * value,
                  GParamSpec  * pspec)
{
  IndicatorPowerService * self = INDICATOR_POWER_SERVICE (o);
  priv_t * p = self->priv;

  switch (property_id)
    {
      case PROP_DEVICE_PROVIDER:
        g_value_set_object (value, p->device_provider);
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (o, property_id, pspec);
    }
}

static void
my_set_property (GObject       * o,
                 guint           property_id,
                 const GValue  * value,
                 GParamSpec    * pspec)
{
  IndicatorPowerService * self = INDICATOR_POWER_SERVICE (o);

  switch (property_id)
    {
      case PROP_DEVICE_PROVIDER:
        indicator_power_service_set_device_provider (self, g_value_get_object (value));
        break;

      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (o, property_id, pspec);
    }
}

static void
my_dispose (GObject * o)
{
  IndicatorPowerService * self = INDICATOR_POWER_SERVICE(o);
  priv_t * p = self->priv;

  if (p->own_id)
    {
      g_bus_unown_name (p->own_id);
      p->own_id = 0;
    }

  unexport (self);

  if (p->cancellable != NULL)
    {
      g_cancellable_cancel (p->cancellable);
      g_clear_object (&p->cancellable);
    }

  if (p->settings != NULL)
    {
      g_signal_handlers_disconnect_by_data (p->settings, self);

      g_clear_object (&p->settings);
    }

  g_clear_object (&p->brightness_action);
  g_clear_object (&p->battery_level_action);
  g_clear_object (&p->header_action);
  g_clear_object (&p->actions);

  g_clear_object (&p->conn);

  if (p->brightness_control)
    {
      g_clear_pointer (&p->brightness_control, ib_brightness_control_free);
    }
  else if (p->brightness_powerd_control)
    {
      g_clear_pointer (&p->brightness_powerd_control, ib_brightness_powerd_control_free);
    }

  indicator_power_service_set_device_provider (self, NULL);

  G_OBJECT_CLASS (indicator_power_service_parent_class)->dispose (o);
}

/***
****  Instantiation
***/

static void
indicator_power_service_init (IndicatorPowerService * self)
{
  GDBusProxy *powerd_proxy = NULL;
  priv_t * p = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            INDICATOR_TYPE_POWER_SERVICE,
                                            IndicatorPowerServicePrivate);
  self->priv = p;

  p->cancellable = g_cancellable_new ();

  p->settings = g_settings_new ("com.canonical.indicator.power");

  p->brightness_control = NULL;
  p->brightness_powerd_control = NULL;

  powerd_proxy = powerd_get_proxy();
  if (powerd_proxy != NULL)
    {
      p->brightness_powerd_control = ib_brightness_powerd_control_new(powerd_proxy);
    }
  else
    {
      p->brightness_control = ib_brightness_control_new ();
    }

  init_gactions (self);

  g_signal_connect_swapped (p->settings, "changed", G_CALLBACK(rebuild_header_now), self);

  p->own_id = g_bus_own_name (G_BUS_TYPE_SESSION,
                              BUS_NAME,
                              G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT,
                              on_bus_acquired,
                              NULL,
                              on_name_lost,
                              self,
                              NULL);
}

static void
indicator_power_service_class_init (IndicatorPowerServiceClass * klass)
{
  GObjectClass * object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = my_dispose;
  object_class->get_property = my_get_property;
  object_class->set_property = my_set_property;

  g_type_class_add_private (klass, sizeof (IndicatorPowerServicePrivate));

  signals[SIGNAL_NAME_LOST] = g_signal_new (
    INDICATOR_POWER_SERVICE_SIGNAL_NAME_LOST,
    G_TYPE_FROM_CLASS(klass),
    G_SIGNAL_RUN_LAST,
    G_STRUCT_OFFSET (IndicatorPowerServiceClass, name_lost),
    NULL, NULL,
    g_cclosure_marshal_VOID__VOID,
    G_TYPE_NONE, 0);

  properties[PROP_0] = NULL;

  properties[PROP_DEVICE_PROVIDER] = g_param_spec_object (
    "device-provider",
    "Device Provider",
    "Source for power devices",
    G_TYPE_OBJECT,
    G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, LAST_PROP, properties);
}

/***
****  Public API
***/

IndicatorPowerService *
indicator_power_service_new (IndicatorPowerDeviceProvider * device_provider)
{
  GObject * o = g_object_new (INDICATOR_TYPE_POWER_SERVICE,
                              "device-provider", device_provider,
                              NULL);

  return INDICATOR_POWER_SERVICE (o);
}

void
indicator_power_service_set_device_provider (IndicatorPowerService * self,
                                             IndicatorPowerDeviceProvider * dp)
{
  priv_t * p;

  g_return_if_fail (INDICATOR_IS_POWER_SERVICE (self));
  g_return_if_fail (!dp || INDICATOR_IS_POWER_DEVICE_PROVIDER (dp));
  p = self->priv;

  if (p->device_provider != NULL)
    {
      g_signal_handlers_disconnect_by_func (p->device_provider,
                                            G_CALLBACK(on_devices_changed),
                                            self);
      g_clear_object (&p->device_provider);

      g_clear_object (&p->primary_device);

      g_list_free_full (p->devices, g_object_unref);
      p->devices = NULL;
    }

  if (dp != NULL)
    {
      p->device_provider = g_object_ref (dp);

      g_signal_connect_swapped (p->device_provider, "devices-changed",
                                G_CALLBACK(on_devices_changed), self);

      on_devices_changed (self);
    }
}

IndicatorPowerDevice *
indicator_power_service_choose_primary_device (GList * devices)
{
  IndicatorPowerDevice * primary = NULL;

  if (devices != NULL)
    {
      GList * tmp;

      tmp = g_list_copy (devices);
      tmp = g_list_sort (tmp, device_compare_func);
      primary = g_object_ref (tmp->data);

      g_list_free (tmp);
    }

  return primary;
}
