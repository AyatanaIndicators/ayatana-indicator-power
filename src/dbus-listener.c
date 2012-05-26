/*

Listens on DBus for Power changes and passes them to an IndicatorPower

Copyright 2012 Canonical Ltd.

Authors:
    Charles Kerr <charles.kerr@canonical.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
version 3.0 as published by the Free Software Foundation.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License version 3.0 for more details.

You should have received a copy of the GNU General Public
License along with this library. If not, see
<http://www.gnu.org/licenses/>.
*/

#include "dbus-listener.h"
#include "indicator-power.h"

#define DBUS_SERVICE          "org.gnome.SettingsDaemon"
#define DBUS_PATH             "/org/gnome/SettingsDaemon"
#define POWER_DBUS_PATH       DBUS_PATH "/Power"
#define POWER_DBUS_INTERFACE  "org.gnome.SettingsDaemon.Power"

struct _IndicatorPowerDbusListenerPrivate
{
	IndicatorPower * ipower;

	GCancellable * proxy_cancel;
	GDBusProxy * proxy;
	guint watcher_id;
};

#define INDICATOR_POWER_DBUS_LISTENER_GET_PRIVATE(o) (INDICATOR_POWER_DBUS_LISTENER(o)->priv)

/* Properties */
/* Enum for the properties so that they can be quickly found and looked up. */
enum {
	PROP_0,
	PROP_INDICATOR
};

/* GObject stuff */
static void indicator_power_dbus_listener_class_init (IndicatorPowerDbusListenerClass *klass);
static void indicator_power_dbus_listener_init       (IndicatorPowerDbusListener *self);
static void indicator_power_dbus_listener_dispose    (GObject *object);
static void indicator_power_dbus_listener_finalize   (GObject *object);
static void set_property (GObject*, guint prop_id, const GValue*, GParamSpec* );
static void get_property (GObject*, guint prop_id,       GValue*, GParamSpec* );

static void gsd_appeared_callback (GDBusConnection *connection, const gchar *name, const gchar *name_owner, gpointer user_data);

G_DEFINE_TYPE (IndicatorPowerDbusListener, indicator_power_dbus_listener, G_TYPE_OBJECT);

static void
indicator_power_dbus_listener_class_init (IndicatorPowerDbusListenerClass *klass)
{
	GParamSpec * pspec;
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (IndicatorPowerDbusListenerPrivate));

	object_class->dispose = indicator_power_dbus_listener_dispose;
	object_class->finalize = indicator_power_dbus_listener_finalize;
	object_class->set_property = set_property;
	object_class->get_property = get_property;

	pspec = g_param_spec_object (INDICATOR_POWER_DBUS_LISTENER_INDICATOR,
                                     "indicator",
                                     "The IndicatorPower to notify when power changes are received",
                                     INDICATOR_POWER_TYPE, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (object_class, PROP_INDICATOR, pspec);
}

/* Initialize an instance */
static void
indicator_power_dbus_listener_init (IndicatorPowerDbusListener *self)
{
	IndicatorPowerDbusListenerPrivate * priv;

        priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            INDICATOR_POWER_DBUS_LISTENER_TYPE,
                                            IndicatorPowerDbusListenerPrivate);
        priv->ipower = NULL;

        priv->watcher_id = g_bus_watch_name (G_BUS_TYPE_SESSION,
                                             DBUS_SERVICE,
                                             G_BUS_NAME_WATCHER_FLAGS_NONE,
                                             gsd_appeared_callback,
                                             NULL,
                                             self,
                                             NULL);

	self->priv = priv;
}

static void
set_indicator (IndicatorPowerDbusListener * self, GObject * ipower)
{
	IndicatorPowerDbusListenerPrivate * priv = self->priv;

	if (priv->ipower != NULL)
		g_object_remove_weak_pointer (G_OBJECT(priv->ipower), (gpointer*)&priv->ipower);

	priv->ipower = INDICATOR_POWER(ipower);

	if (priv->ipower != NULL)
		g_object_add_weak_pointer (G_OBJECT(priv->ipower), (gpointer*)&priv->ipower);
};

static void
indicator_power_dbus_listener_dispose (GObject *object)
{
	IndicatorPowerDbusListener * self = INDICATOR_POWER_DBUS_LISTENER(object);
	IndicatorPowerDbusListenerPrivate * priv = self->priv;

	g_clear_object (&priv->proxy);
	g_clear_object (&priv->proxy_cancel);

	set_indicator (self, NULL);

	G_OBJECT_CLASS (indicator_power_dbus_listener_parent_class)->dispose (object);
}

static void
indicator_power_dbus_listener_finalize (GObject *object)
{
	G_OBJECT_CLASS (indicator_power_dbus_listener_parent_class)->finalize (object);
}

/***
****
***/

static void
get_property (GObject * o, guint  prop_id, GValue * value, GParamSpec * pspec)
{
        IndicatorPowerDbusListener * self = INDICATOR_POWER_DBUS_LISTENER(o);
        IndicatorPowerDbusListenerPrivate * priv = self->priv;

        switch (prop_id)
	{
		case PROP_INDICATOR:
			g_value_set_object (value, priv->ipower);
			break;
	}
}

static void
set_property (GObject * o, guint prop_id, const GValue * value, GParamSpec * pspec)
{
        IndicatorPowerDbusListener * self = INDICATOR_POWER_DBUS_LISTENER(o);

        switch (prop_id)
	{
		case PROP_INDICATOR:
			set_indicator (self, g_value_get_object(value));
			break;
	}
}

/***
****
***/

static void
get_devices_cb (GObject      * source_object,
                GAsyncResult * res,
                gpointer       user_data)
{
	GError *error;
	int device_count = 0;
	GVariant * devices_container;
	IndicatorPowerDevice ** devices = NULL;
	IndicatorPowerDbusListener * self = INDICATOR_POWER_DBUS_LISTENER (user_data);
        IndicatorPowerDbusListenerPrivate * priv = self->priv;

	/* build an array of IndicatorPowerDevices from the DBus response */
	error = NULL;
	devices_container = g_dbus_proxy_call_finish (G_DBUS_PROXY (source_object), res, &error);
	if (devices_container == NULL)
	{
		g_message ("Couldn't get devices: %s\n", error->message);
		g_error_free (error);
	}
	else
	{
		gsize i;
		GVariant * devices_variant = g_variant_get_child_value (devices_container, 0);
		device_count = devices_variant ? g_variant_n_children (devices_variant) : 0;
		devices = g_new0 (IndicatorPowerDevice*, device_count);

		for (i=0; i<device_count; i++)
		{
			GVariant * v = g_variant_get_child_value (devices_variant, i);
			devices[i] = indicator_power_device_new_from_variant (v);
			g_variant_unref (v);
		}

		g_variant_unref (devices_variant);
		g_variant_unref (devices_container);
	}

	if (priv->ipower != NULL)
	{
		indicator_power_set_devices (priv->ipower, devices, device_count);
	}

	g_free (devices);
}

static void
request_device_list (IndicatorPowerDbusListener * self)
{
	g_dbus_proxy_call (self->priv->proxy,
	                   "GetDevices",
	                   NULL,
	                   G_DBUS_CALL_FLAGS_NONE,
	                   -1,
	                   self->priv->proxy_cancel,
	                   get_devices_cb,
	                   self);
}

static void
receive_properties_changed (GDBusProxy *proxy                  G_GNUC_UNUSED,
                            GVariant   *changed_properties     G_GNUC_UNUSED,
                            GStrv       invalidated_properties G_GNUC_UNUSED,
                            gpointer    user_data)
{
	request_device_list (INDICATOR_POWER_DBUS_LISTENER(user_data));
}


static void
service_proxy_cb (GObject      *object,
                  GAsyncResult *res,
                  gpointer      user_data)
{
	GError * error = NULL;
        IndicatorPowerDbusListener * self = INDICATOR_POWER_DBUS_LISTENER(user_data);
        IndicatorPowerDbusListenerPrivate * priv = self->priv;

	priv->proxy = g_dbus_proxy_new_for_bus_finish (res, &error);
	g_clear_object (&priv->proxy_cancel);

	if (error != NULL) {
		g_error ("Error creating proxy: %s", error->message);
		g_error_free (error);
		return;
	}

	/* we want to change the primary device changes */
	g_signal_connect (priv->proxy,
	                  "g-properties-changed",
	                  G_CALLBACK (receive_properties_changed),
	                  user_data);

	/* get the initial state */
	request_device_list (self);
}

static void
gsd_appeared_callback (GDBusConnection *connection,
                       const gchar     *name,
                       const gchar     *name_owner,
                       gpointer         user_data)
{
        IndicatorPowerDbusListener * self = INDICATOR_POWER_DBUS_LISTENER(user_data);
        IndicatorPowerDbusListenerPrivate * priv = self->priv;

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
