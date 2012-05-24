/*

A simple Device structure used internally by indicator-power

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

#include "device.h"

struct _IndicatorPowerDevicePrivate
{
	UpDeviceKind kind;
	UpDeviceState state;
	gchar * object_path;
	gchar * icon;
	gdouble percentage;
	time_t time;
};

#define INDICATOR_POWER_DEVICE_GET_PRIVATE(o) (INDICATOR_POWER_DEVICE(o)->priv)

/* Properties */
/* Enum for the properties so that they can be quickly found and looked up. */
enum {
	PROP_0,
	PROP_KIND,
	PROP_STATE,
	PROP_OBJECT_PATH,
	PROP_ICON,
	PROP_PERCENTAGE,
	PROP_TIME
};

/* GObject stuff */
static void indicator_power_device_class_init (IndicatorPowerDeviceClass *klass);
static void indicator_power_device_init       (IndicatorPowerDevice *self);
static void indicator_power_device_dispose    (GObject *object);
static void indicator_power_device_finalize   (GObject *object);
static void set_property (GObject*, guint prop_id, const GValue*, GParamSpec* );
static void get_property (GObject*, guint prop_id,       GValue*, GParamSpec* );

G_DEFINE_TYPE (IndicatorPowerDevice, indicator_power_device, G_TYPE_OBJECT);

static void
indicator_power_device_class_init (IndicatorPowerDeviceClass *klass)
{
	GParamSpec * pspec;
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (IndicatorPowerDevicePrivate));

	object_class->dispose = indicator_power_device_dispose;
	object_class->finalize = indicator_power_device_finalize;
	object_class->set_property = set_property;
	object_class->get_property = get_property;

	pspec = g_param_spec_int (INDICATOR_POWER_DEVICE_KIND, "kind", "The device's UpDeviceKind",
                                  UP_DEVICE_KIND_UNKNOWN, UP_DEVICE_KIND_LAST, UP_DEVICE_KIND_UNKNOWN,
			          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (object_class, PROP_KIND, pspec);

	pspec = g_param_spec_int (INDICATOR_POWER_DEVICE_STATE, "state", "The device's UpDeviceState",
                                  UP_DEVICE_STATE_UNKNOWN, UP_DEVICE_STATE_LAST, UP_DEVICE_STATE_UNKNOWN,
			          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (object_class, PROP_STATE, pspec);

	pspec = g_param_spec_string (INDICATOR_POWER_DEVICE_OBJECT_PATH, "object path", "The device's DBus object path", NULL,
			             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (object_class, PROP_OBJECT_PATH, pspec);

	pspec = g_param_spec_string (INDICATOR_POWER_DEVICE_ICON, "icon", "The device's icon", NULL,
			             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (object_class, PROP_ICON, pspec);

	pspec = g_param_spec_double (INDICATOR_POWER_DEVICE_PERCENTAGE, "percentage", "percent charged",
                                     0.0, 100.0, 0.0,
			             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (object_class, PROP_PERCENTAGE, pspec);

	pspec = g_param_spec_uint64 (INDICATOR_POWER_DEVICE_TIME, "time", "time left",
                                     0, G_MAXUINT64, 0,
			             G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);
	g_object_class_install_property (object_class, PROP_TIME, pspec);
}

/* Initialize an instance */
static void
indicator_power_device_init (IndicatorPowerDevice *self)
{
	IndicatorPowerDevicePrivate * priv;

        priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            INDICATOR_POWER_DEVICE_TYPE,
                                            IndicatorPowerDevicePrivate);
        priv->kind = UP_DEVICE_KIND_UNKNOWN;
        priv->state = UP_DEVICE_STATE_UNKNOWN;
        priv->object_path = NULL;
        priv->icon = NULL;
        priv->percentage = 0.0;
        priv->time = 0;

	self->priv = priv;
}

static void
indicator_power_device_dispose (GObject *object)
{
	G_OBJECT_CLASS (indicator_power_device_parent_class)->dispose (object);
}

static void
indicator_power_device_finalize (GObject *object)
{
	IndicatorPowerDevice * self = INDICATOR_POWER_DEVICE(object);
	IndicatorPowerDevicePrivate * priv = self->priv;

	//g_clear_pointer (&priv->object_path, g_free);
	//g_clear_pointer (&priv->icon, g_free);
	g_free (priv->object_path);
	priv->object_path = NULL;
	g_free (priv->icon);
	priv->icon = NULL;
}

/***
****
***/

static void
get_property (GObject * o, guint  prop_id, GValue * value, GParamSpec * pspec)
{
        IndicatorPowerDevice * self = INDICATOR_POWER_DEVICE(o);
        IndicatorPowerDevicePrivate * priv = self->priv;

        switch (prop_id)
	{
		case PROP_KIND:
			g_return_if_fail (G_VALUE_HOLDS_INT(value));
			g_value_set_int (value, priv->kind);
			break;

		case PROP_STATE:
			g_return_if_fail (G_VALUE_HOLDS_INT(value));
			g_value_set_int (value, priv->state);
			break;

		case PROP_OBJECT_PATH:
			g_return_if_fail (G_VALUE_HOLDS_STRING(value));
			g_value_set_string (value, priv->object_path);
			break;

		case PROP_ICON:
			g_return_if_fail (G_VALUE_HOLDS_STRING(value));
			g_value_set_string (value, priv->icon);
			break;

		case PROP_PERCENTAGE:
			g_return_if_fail (G_VALUE_HOLDS_DOUBLE(value));
			g_value_set_double (value, priv->percentage);
			break;

		case PROP_TIME:
			g_return_if_fail (G_VALUE_HOLDS_UINT64(value));
			g_value_set_uint64 (value, priv->time);
			break;

		default:
                	G_OBJECT_WARN_INVALID_PROPERTY_ID (o, prop_id, pspec);
			break;
	}
}

static void
set_property (GObject * o, guint prop_id, const GValue * value, GParamSpec * pspec)
{
        IndicatorPowerDevice * self = INDICATOR_POWER_DEVICE(o);
        IndicatorPowerDevicePrivate * priv = self->priv;

        switch (prop_id)
	{
		case PROP_KIND:
			priv->kind = g_value_get_int (value);
			break;

		case PROP_STATE:
			priv->state = g_value_get_int (value);
			break;

		case PROP_OBJECT_PATH:
			g_free (priv->object_path);
			priv->object_path = g_value_dup_string (value);
			break;

		case PROP_ICON:
			g_free (priv->icon);
			priv->icon = g_value_dup_string (value);
			break;

		case PROP_PERCENTAGE:
			priv->percentage = g_value_get_double (value);
			break;

		case PROP_TIME:
			priv->time = g_value_get_uint64(value);
			break;

		default:
                	G_OBJECT_WARN_INVALID_PROPERTY_ID (o, prop_id, pspec);
			break;
	}
}

/***
****
***/

UpDeviceKind
indicator_power_device_get_kind  (const IndicatorPowerDevice * device)
{
	g_return_val_if_fail (INDICATOR_IS_POWER_DEVICE(device), UP_DEVICE_KIND_UNKNOWN);

        return device->priv->kind;
}

UpDeviceState
indicator_power_device_get_state (const IndicatorPowerDevice * device)
{
	g_return_val_if_fail (INDICATOR_IS_POWER_DEVICE(device), UP_DEVICE_STATE_UNKNOWN);

        return device->priv->state;
}

const gchar *
indicator_power_device_get_object_path (const IndicatorPowerDevice * device)
{
	g_return_val_if_fail (INDICATOR_IS_POWER_DEVICE(device), UP_DEVICE_KIND_UNKNOWN);

        return device->priv->object_path;
}

const gchar *
indicator_power_device_get_icon (const IndicatorPowerDevice * device)
{
	g_return_val_if_fail (INDICATOR_IS_POWER_DEVICE(device), UP_DEVICE_KIND_UNKNOWN);

        return device->priv->icon;
}

gdouble
indicator_power_device_get_percentage (const IndicatorPowerDevice * device)
{
	g_return_val_if_fail (INDICATOR_IS_POWER_DEVICE(device), UP_DEVICE_KIND_UNKNOWN);

        return device->priv->percentage;
}

time_t
indicator_power_device_get_time (const IndicatorPowerDevice * device)
{
	g_return_val_if_fail (INDICATOR_IS_POWER_DEVICE(device), UP_DEVICE_KIND_UNKNOWN);

        return device->priv->time;
}

/***
****
***/

IndicatorPowerDevice *
indicator_power_device_new (const gchar * object_path,
                            UpDeviceKind  kind,
                            const gchar * icon_path,
                            gdouble percentage,
                            UpDeviceState state,
                            time_t timestamp)
{
	GObject * o = g_object_new (INDICATOR_POWER_DEVICE_TYPE,
	                            INDICATOR_POWER_DEVICE_KIND, kind,
	                            INDICATOR_POWER_DEVICE_STATE, state,
	                            INDICATOR_POWER_DEVICE_OBJECT_PATH, object_path,
	                            INDICATOR_POWER_DEVICE_ICON, icon_path,
	                            INDICATOR_POWER_DEVICE_PERCENTAGE, percentage,
	                            INDICATOR_POWER_DEVICE_TIME, (guint64)timestamp,
	                            NULL);
	return INDICATOR_POWER_DEVICE(o);
}

IndicatorPowerDevice *
indicator_power_device_new_from_variant (GVariant * v)
{
	UpDeviceKind kind = UP_DEVICE_KIND_UNKNOWN;
	UpDeviceState state = UP_DEVICE_STATE_UNKNOWN;
	const gchar * icon = NULL;
	const gchar * object_path = NULL;
	gdouble percentage = 0;
	guint64 time = 0;

	g_variant_get (v, "(&su&sdut)",
	               &object_path,
	               &kind,
	               &icon,
	               &percentage,
	               &state,
	               &time);

	return indicator_power_device_new (object_path,
	                                   kind,
	                                   icon,
	                                   percentage,
	                                   state,
	                                   time);
}
