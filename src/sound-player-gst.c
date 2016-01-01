/*
 * Copyright 2016 Canonical Ltd.
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
 *
 * Authors:
 *   Charles Kerr <charles.kerr@canonical.com>
 */

#include "sound-player.h"
#include "sound-player-gst.h"

#include <gst/gst.h>


/***
****  private struct
***/

typedef struct
{
  int unused;
}
IndicatorPowerSoundPlayerGSTPrivate;

typedef IndicatorPowerSoundPlayerGSTPrivate priv_t;

#define get_priv(o) ((priv_t*)indicator_power_sound_player_gst_get_instance_private(o))


/***
****  GObject boilerplate
***/

static void indicator_power_device_provider_interface_init (
                                IndicatorPowerSoundPlayerInterface * iface);

G_DEFINE_TYPE_WITH_CODE (
  IndicatorPowerSoundPlayerGST,
  indicator_power_sound_player_gst,
  G_TYPE_OBJECT,
  G_ADD_PRIVATE(IndicatorPowerSoundPlayerGST)
  G_IMPLEMENT_INTERFACE (INDICATOR_TYPE_POWER_SOUND_PLAYER,
                         indicator_power_device_provider_interface_init))

/***
****  GSTREAMER
***/

static void
gst_init_once(void)
{
  static gboolean gst_init_checked = FALSE;

  if (G_UNLIKELY(!gst_init_checked))
    {
      GError* error = NULL;
      if (!gst_init_check(NULL, NULL, &error))
        {
          g_critical("Unable to play alarm sound: %s", error->message);
          g_error_free(error);
        }
      gst_init_checked = TRUE;
    }
}

static gboolean bus_callback(GstBus* bus G_GNUC_UNUSED, GstMessage* msg, gpointer gelement)
{
  const GstMessageType message_type = GST_MESSAGE_TYPE(msg);

  if (GST_MESSAGE_SRC(msg) != gelement)
    return G_SOURCE_CONTINUE;

  /* on eos, cleanup the element and cancel our gst bus subscription */
  if (message_type == GST_MESSAGE_EOS)
    {
      g_debug("got GST_MESSAGE_EOS on sound play");
      gst_element_set_state(GST_ELEMENT(gelement), GST_STATE_NULL);
      gst_object_unref(gelement);
      return G_SOURCE_REMOVE;
    }

  /* on stream start, set the media role to 'alert' if we're using pulsesink */
  if (message_type == GST_MESSAGE_STREAM_START)
    {
      GstElement* audio_sink = NULL;
      g_debug("got GST_MESSAGE_STREAM_START on sound play");
      g_object_get(gelement, "audio-sink", &audio_sink, NULL);
      if (audio_sink != NULL)
        {
          GstPluginFeature* feature;
          feature = GST_PLUGIN_FEATURE_CAST(GST_ELEMENT_GET_CLASS(audio_sink)->elementfactory);
          if (feature && g_strcmp0(gst_plugin_feature_get_name(feature), "pulsesink") == 0)
            {
              const gchar* const props_str = "props,media.role=alert";
              GstStructure* props = gst_structure_from_string(props_str, NULL);
              g_debug("setting audio sink properties to '%s'", props_str);
              g_object_set(audio_sink, "stream-properties", props, NULL);
              g_clear_pointer(&props, gst_structure_free);
            }
          gst_object_unref(audio_sink);
        }
    }

  return G_SOURCE_CONTINUE;
}

/***
****  IndicatorPowerSoundPlayer virtual functions
***/

static void
my_play_uri (IndicatorPowerSoundPlayer * self G_GNUC_UNUSED, const gchar * uri)
{
  GstElement * element;
  GstBus * bus;

  /* create the element */
  element = gst_element_factory_make("playbin", NULL);

  /* start listening for gst events */
  bus = gst_pipeline_get_bus(GST_PIPELINE(element));
  gst_bus_add_watch(bus, bus_callback, element);
  gst_object_unref(bus);

  /* play the sound */
  g_debug("Playing '%s'", uri);
  g_object_set(element, "uri", uri, NULL);
  gst_element_set_state(element, GST_STATE_PLAYING);
}

/***
****  Instantiation
***/

static void
indicator_power_sound_player_gst_class_init (IndicatorPowerSoundPlayerGSTClass * klass)
{
  gst_init_once();
}

static void
indicator_power_device_provider_interface_init (IndicatorPowerSoundPlayerInterface * iface)
{
  iface->play_uri = my_play_uri;
}

static void
indicator_power_sound_player_gst_init (IndicatorPowerSoundPlayerGST * self G_GNUC_UNUSED)
{
}

/***
****  Public API
***/

IndicatorPowerSoundPlayer *
indicator_power_sound_player_gst_new(void)
{
  gpointer o = g_object_new (INDICATOR_TYPE_POWER_SOUND_PLAYER_GST, NULL);

  return INDICATOR_POWER_SOUND_PLAYER (o);
}
