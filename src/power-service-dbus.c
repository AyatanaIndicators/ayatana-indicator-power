/*
An indicator to power related information in the menubar.

Copyright 2011 Codethink Ltd.

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

#include "power-service-dbus.h"

#include <gio/gio.h>

#include "dbus-shared-names.h"
#include "gen-power-service.xml.h"

struct _PowerServiceDbusPrivate
{
  GDBusConnection *bus;
  GCancellable    *bus_cancel;
  guint            dbus_registration;
};

/* GDBus Stuff */
static GDBusNodeInfo      *node_info = NULL;
static GDBusInterfaceInfo *interface_info = NULL;

static void     power_service_dbus_class_init   (PowerServiceDbusClass *klass);
static void     power_service_dbus_init         (PowerServiceDbus *self);
static void     power_service_dbus_dispose      (GObject *object);
static void     power_service_dbus_finalize     (GObject *object);
static void     bus_get_cb                      (GObject      *object,
                                                 GAsyncResult *res,
                                                 gpointer      user_data);


G_DEFINE_TYPE (PowerServiceDbus, power_service_dbus, G_TYPE_OBJECT);

static void
bus_get_cb (GObject      *object,
            GAsyncResult *res,
            gpointer      user_data)
{
  PowerServiceDbus *self = POWER_SERVICE_DBUS (user_data);
  PowerServiceDbusPrivate *priv = self->priv;
  GError *error = NULL;
  GDBusConnection *connection = g_bus_get_finish (res, &error);

  if (error != NULL)
    {
      g_error("OMG! Unable to get a connection to DBus: %s", error->message);
      g_error_free(error);

      return;
    }

  priv->bus = connection;

  if (priv->bus_cancel != NULL)
    {
      g_object_unref (priv->bus_cancel);
      priv->bus_cancel = NULL;
    }

  /* Now register our object on our new connection */
  priv->dbus_registration = g_dbus_connection_register_object (priv->bus,
                                                               INDICATOR_POWER_SERVICE_DBUS_OBJECT,
                                                               interface_info,
                                                               NULL,
                                                               user_data,
                                                               NULL,
                                                               &error);
  if (error != NULL)
    {
      g_error ("Unable to register the object to DBus: %s", error->message);
      g_error_free(error);

      return;
    }
}

static void
power_service_dbus_class_init (PowerServiceDbusClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = power_service_dbus_dispose;
  object_class->finalize = power_service_dbus_finalize;

  /* Setting up the DBus interfaces */
  if (node_info == NULL)
    {
      GError * error = NULL;

      node_info = g_dbus_node_info_new_for_xml (_power_service, &error);
      if (error != NULL)
        {
          g_error ("Unable to parse Power Service Dbus description: %s", error->message);
          g_error_free (error);
        }
    }

  if (interface_info == NULL)
    {
      interface_info = g_dbus_node_info_lookup_interface (node_info, INDICATOR_POWER_SERVICE_DBUS_INTERFACE);

      if (interface_info == NULL)
        {
          g_error ("Unable to find interface '" INDICATOR_POWER_SERVICE_DBUS_INTERFACE "'");
        }
    }

  g_type_class_add_private (klass, sizeof (PowerServiceDbusPrivate));
}

static void
power_service_dbus_init (PowerServiceDbus *self)
{
  PowerServiceDbusPrivate *priv;

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            POWER_SERVICE_DBUS_TYPE,
                                            PowerServiceDbusPrivate);
  priv = self->priv;

  priv->bus = NULL;
  priv->bus_cancel = NULL;
  priv->dbus_registration = 0;

  self->priv->bus_cancel = g_cancellable_new ();
  g_bus_get (G_BUS_TYPE_SESSION,
             priv->bus_cancel,
             bus_get_cb,
             self);
}

static void
power_service_dbus_dispose (GObject *object)
{
  PowerServiceDbus *self = POWER_SERVICE_DBUS (object);
  PowerServiceDbusPrivate *priv = self->priv;

  if (priv->dbus_registration != 0)
    {
      g_dbus_connection_unregister_object (priv->bus,
                                           priv->dbus_registration);
      /* Don't care if it fails, there's nothing we can do */
      priv->dbus_registration = 0;
    }

  if (priv->bus != NULL)
    {
      g_object_unref (priv->bus);
      priv->bus = NULL;
    }

  if (priv->bus_cancel != NULL)
    {
      g_cancellable_cancel (priv->bus_cancel);
      g_object_unref (priv->bus_cancel);
      priv->bus_cancel = NULL;
    }

  G_OBJECT_CLASS (power_service_dbus_parent_class)->dispose (object);
}

static void
power_service_dbus_finalize (GObject *object)
{
  G_OBJECT_CLASS (power_service_dbus_parent_class)->finalize (object);
}

