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

#include "device.h"
#include "device-provider.h"
#include "service.h"

#define BUS_NAME "com.canonical.indicator.power"
#define BUS_PATH "/com/canonical/indicator/power"

#define SETTINGS_SHOW_TIME_S "show-time"
#define SETTINGS_ICON_POLICY_S "icon-policy"

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
  PROFILE_DESKTOP,
  PROFILE_GREETER,
  N_PROFILES
};

static const char * const menu_names[N_PROFILES] =
{
  "desktop",
  "greeter"
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

  guint own_id;
  guint actions_export_id;
  GDBusConnection * conn;

  struct ProfileMenuInfo menus[N_PROFILES];

  GSimpleActionGroup * actions;
  GSimpleAction * header_action;
  GSimpleAction * show_time_action;

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
      gchar * details;

      indicator_power_device_get_time_details (p->primary_device,
                                               &label,
                                               &details,
                                               &a11y);

      icon = indicator_power_device_get_gicon (p->primary_device);

      g_free (details);
    }

  g_variant_builder_init (&b, G_VARIANT_TYPE("a{sv}"));

  g_variant_builder_add (&b, "{sv}", "visible",
                         g_variant_new_boolean (should_be_visible (self)));

  if (label != NULL)
    {
      if (g_settings_get_boolean (p->settings, SETTINGS_SHOW_TIME_S))
        g_variant_builder_add (&b, "{sv}", "label",
                               g_variant_new_string (label));

      g_free (label);
    }

  if (icon != NULL)
    {
      g_variant_builder_add (&b, "{sv}", "icon", g_icon_serialize (icon));

      g_object_unref (icon);
    }

  if (a11y != NULL)
    {
      g_variant_builder_add (&b, "{sv}", "accessible-desc",
                             g_variant_new_string (a11y));

      g_free (a11y);
    }

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
    char * brief;
    char * label;
    char * a11y;
    GMenuItem * menu_item;
    GIcon * icon = indicator_power_device_get_gicon (device);

    indicator_power_device_get_time_details (device,
                                             &brief,
                                             &label,
                                             &a11y);

    menu_item = g_menu_item_new (label, "indicator.activate-statistics");

    if (icon != NULL)
      {
        g_menu_item_set_attribute_value (menu_item,
                                         G_MENU_ATTRIBUTE_ICON,
                                         g_icon_serialize (icon));
      }

    g_menu_append_item (menu, menu_item);
    g_object_unref (menu_item);

    g_clear_object (&icon);
    g_free (brief);
    g_free (label);
    g_free (a11y);
  }
}


static GMenuModel *
create_devices_section (IndicatorPowerService * self)
{
  GList * l;
  GMenu * menu = g_menu_new ();

  for (l=self->priv->devices; l!=NULL; l=l->next)
    append_device_to_menu (menu, l->data);

  return G_MENU_MODEL (menu);
}


/***
****
****  SETTINGS SECTION
****
***/

static GMenuModel *
create_settings_section (IndicatorPowerService * self G_GNUC_UNUSED)
{
  GMenu * menu = g_menu_new ();

  g_menu_append (menu,
                 _("Show Time in Menu Bar"),
                 "indicator.show-time");

  g_menu_append (menu,
                 _("Power Settings\342\200\246"),
                 "indicator.activate-settings");

  return G_MENU_MODEL (menu);
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
  struct ProfileMenuInfo * desktop = &p->menus[PROFILE_DESKTOP];
  struct ProfileMenuInfo * greeter = &p->menus[PROFILE_GREETER];

  if (p->conn == NULL) /* we haven't built the menus yet */
    return;

  if (sections & SECTION_HEADER)
    {
      g_simple_action_set_state (p->header_action, create_header_state (self));
    }

  if (sections & SECTION_DEVICES)
    {
      rebuild_section (desktop->submenu, 0, create_devices_section (self));
      rebuild_section (greeter->submenu, 0, create_devices_section (self));
    }

  if (sections & SECTION_SETTINGS)
    {
      rebuild_section (desktop->submenu, 1, create_settings_section (self));
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

  if (profile == PROFILE_DESKTOP)
    {
      sections[n++] = create_devices_section (self);
      sections[n++] = create_settings_section (self);
    }
  else if (profile == PROFILE_GREETER)
    {
      sections[n++] = create_devices_section (self);
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
  execute_command ("gnome-control-center power");
}

static void
on_statistics_activated (GSimpleAction * a      G_GNUC_UNUSED,
                         GVariant      * param  G_GNUC_UNUSED,
                         gpointer        gself  G_GNUC_UNUSED)
{
  execute_command ("gnome-power-statistics");
}

/* FIXME: use a GBinding to tie the gaction's state and the GSetting together? */

static void
set_show_time_flag (IndicatorPowerService * self, gboolean b)
{
  GVariant * v;
  priv_t * p = self->priv;

  /* update the settings */
  if (b != g_settings_get_boolean (p->settings, SETTINGS_SHOW_TIME_S))
    g_settings_set_boolean (p->settings, SETTINGS_SHOW_TIME_S, b);

  /* update the action state */
  v = g_action_get_state (G_ACTION(p->show_time_action));
  if (b != g_variant_get_boolean (v))
    g_simple_action_set_state (p->show_time_action, g_variant_new_boolean (b));
  g_variant_unref (v);

  rebuild_header_now (self);
}
static void
on_show_time_setting_changed (GSettings * settings, gchar * key, gpointer gself)
{
  set_show_time_flag (INDICATOR_POWER_SERVICE(gself),
                      g_settings_get_boolean (settings, key));
}

static void
on_show_time_action_state_changed (GAction    * action,
                                   GParamSpec * pspec   G_GNUC_UNUSED,
                                   gpointer     gself)
{
  GVariant * v = g_action_get_state (action);
  set_show_time_flag (INDICATOR_POWER_SERVICE(gself),
                      g_variant_get_boolean (v));
  g_variant_unref (v);
}

/* toggles the state */
static void
on_show_time_action_activated (GSimpleAction * simple,
                               GVariant      * parameter G_GNUC_UNUSED,
                               gpointer        unused    G_GNUC_UNUSED)
{
  GVariant * v = g_action_get_state (G_ACTION (simple));
  gboolean flag = g_variant_get_boolean (v);
  g_simple_action_set_state (simple, g_variant_new_boolean (!flag));
  g_variant_unref (v);
}

static void
init_gactions (IndicatorPowerService * self)
{
  GSimpleAction * a;
  gboolean show_time;
  priv_t * p = self->priv;

  GActionEntry entries[] = {
    { "activate-settings", on_settings_activated },
    { "activate-statistics", on_statistics_activated }
  };

  p->actions = g_simple_action_group_new ();

  g_action_map_add_action_entries (G_ACTION_MAP(p->actions),
                                   entries,
                                   G_N_ELEMENTS(entries),
                                   self);

  /* add the header action */
  a = g_simple_action_new_stateful ("_header", NULL, create_header_state (self));
  g_simple_action_group_insert (p->actions, G_ACTION(a));
  p->header_action = a;

  /* add the show-time action */
  show_time = g_settings_get_boolean (p->settings, SETTINGS_SHOW_TIME_S);
  a = g_simple_action_new_stateful ("show-time",
                                    NULL,
                                    g_variant_new_boolean(show_time));
  g_signal_connect (a, "activate",
                    G_CALLBACK(on_show_time_action_activated), self);
  g_signal_connect (a, "notify",
                    G_CALLBACK(on_show_time_action_state_changed), self);
  g_simple_action_group_insert (p->actions, G_ACTION(a));
  p->show_time_action = a;

  rebuild_header_now (self);
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

  /* update the device list */
  g_list_free_full (p->devices, (GDestroyNotify)g_object_unref);
  p->devices = indicator_power_device_provider_get_devices (p->device_provider);

  /* update the primary device */
  g_clear_object (&p->primary_device);
  p->primary_device = indicator_power_service_choose_primary_device (p->devices);

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

  if (p->show_time_action != NULL)
    {
      g_signal_handlers_disconnect_by_data (p->show_time_action, self);

      g_clear_object (&p->show_time_action);
    }

  g_clear_object (&p->header_action);
  g_clear_object (&p->show_time_action);
  g_clear_object (&p->actions);

  g_clear_object (&p->conn);

  indicator_power_service_set_device_provider (self, NULL);

  G_OBJECT_CLASS (indicator_power_service_parent_class)->dispose (o);
}

/***
****  Instantiation
***/

static void
indicator_power_service_init (IndicatorPowerService * self)
{
  priv_t * p = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            INDICATOR_TYPE_POWER_SERVICE,
                                            IndicatorPowerServicePrivate);
  self->priv = p;

  p->cancellable = g_cancellable_new ();

  p->settings = g_settings_new ("com.canonical.indicator.power");

  init_gactions (self);

  g_signal_connect_swapped (p->settings, "changed::" SETTINGS_ICON_POLICY_S,
                            G_CALLBACK(rebuild_header_now), self);
  g_signal_connect (p->settings, "changed::" SETTINGS_SHOW_TIME_S,
                    G_CALLBACK(on_show_time_setting_changed), self);

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
