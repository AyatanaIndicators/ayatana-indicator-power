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

/* Indicator Stuff */
#include <libindicator/indicator.h>
#include <libindicator/indicator-object.h>
#include <libindicator/indicator-service-manager.h>
#include <libindicator/indicator-image-helper.h>

/* DBusMenu */
#include <libido/libido.h>
#if GTK_CHECK_VERSION(3, 0, 0)
#include <libdbusmenu-gtk3/menu.h>
#include <libdbusmenu-gtk3/menuitem.h>
#else
#include <libdbusmenu-gtk/menu.h>
#include <libdbusmenu-gtk/menuitem.h>
#endif

#include "dbus-shared-names.h"

#define DEFAULT_ICON   "battery"

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
  IndicatorServiceManager *service;

  DbusmenuGtkMenu   *menu;
  DbusmenuGtkClient *client;

  GtkLabel *label;
  GtkImage *status_image;

  GCancellable        *status_proxy_cancel;
  GDBusProxy          *status_proxy;
  IdoCalendarMenuItem *ido_entry;
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
indicator_power_init (IndicatorPower *self)
{
  IndicatorPowerPrivate *priv;

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            INDICATOR_POWER_TYPE,
                                            IndicatorPowerPrivate);
  priv = self->priv;

  /* Init variables */
  priv->service = NULL;
  priv->menu = NULL;
  priv->client = NULL;

  /* Do stuff with them */
  priv->service = indicator_service_manager_new_version (INDICATOR_POWER_DBUS_NAME,
                                                         INDICATOR_POWER_DBUS_VERSION);
/*
  g_signal_connect (G_OBJECT (priv->service),
                    INDICATOR_SERVICE_MANAGER_SIGNAL_CONNECTION_CHANGE,
                    G_CALLBACK (connection_changed),
                    self);
*/

  /* Builds the dbusmenu for the service. */
  priv->menu = dbusmenu_gtkmenu_new (INDICATOR_POWER_DBUS_NAME,
                                     INDICATOR_POWER_DBUS_OBJECT);
  priv->client = dbusmenu_gtkmenu_get_client (priv->menu);

/*
  dbusmenu_client_add_type_handler (DBUSMENU_CLIENT (client),
                                    DBUSMENU_ENTRY_MENUITEM_TYPE,
                                    new_entry_item);
*/
}

static void
indicator_power_dispose (GObject *object)
{
  IndicatorPower *self = INDICATOR_POWER (object);
  IndicatorPowerPrivate *priv = self->priv;

  if (priv->service != NULL)
  {
    g_object_unref (G_OBJECT (priv->service));
    priv->service = NULL;
  }

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
    priv->status_image = indicator_image_helper (DEFAULT_ICON "-panel");
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

  if (priv->label == NULL)
  {
    name = gtk_label_get_text (priv->label);

    return name;
  }

  return NULL;
}
