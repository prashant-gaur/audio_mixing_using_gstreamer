#include <fcntl.h>
#include <glib-2.0/glib.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <gst/gst.h>

#define TIMEOUT_ACCESSPOINT (10)

const char* sourceBin1 = "filesrc location=/home/gpr3hi/open_source/audio_samples/Hindi_Audio/Audio_1.wav ! wavparse ! volume volume=0.4";

const char* sourceBin2 = "filesrc location=/home/gpr3hi/open_source/audio_samples/Hindi_Audio/Audio_2.wav ! wavparse ! volume volume=0.8";

const char* sourceBin3 = "filesrc location=/home/gpr3hi/open_source/audio_samples/Hindi_Audio/Audio_4.wav ! wavparse ! volume volume=1.0";

const char* sinkBin = "adder name=mixer ! volume name=sinkvolume ! queue2 ! audioconvert ! audioresample ! autoaudiosink";

GstElement *pipeline, *sourceElement1, *sourceElement2, *sourceElement3, *sinkElement, *mixer;
GstPad *sinkPad1, *ghostSinkPad1, *sinkPad2, *ghostSinkPad2, *sinkPad3, *ghostSinkPad3, *ghostSrcPad1, *ghostSrcPad2, *ghostSrcPad3;

static gboolean bus_call(GstBus* bus, GstMessage* msg, gpointer data)
{
    GMainLoop* loop = (GMainLoop*)data;

    switch (GST_MESSAGE_TYPE(msg)) {

    case GST_MESSAGE_EOS:
        g_print("End of stream \n");
        //g_main_loop_quit (loop);
        break;

    case GST_MESSAGE_ERROR: {
        gchar* debug;
        GError* error;

        gst_message_parse_error(msg, &error, &debug);
        g_free(debug);

        g_printerr("Error: %s\n", error->message);
        g_error_free(error);

        g_main_loop_quit(loop);
        break;
    }
    default:
        break;
    }

    return TRUE;
}
#if 0
void for_each_pipeline_element(const GValue *value, gpointer data)
{
    gchar *name;
	GstElement *element = g_value_get_object(value);

    /* get name */
    g_object_get (G_OBJECT (element), "name", &name, NULL);
    g_print ("The name of the element is '%s'.\n", name);
    g_free (name);

    /* or gst_object_get_name() can also be used */
    gst_element_set_locked_state(element, TRUE);
    gst_bin_remove(GST_BIN(sourceElement1), element);
    gst_element_set_state(element, GST_STATE_NULL);

}
#endif

gboolean remove_source1(gpointer userdata)
{
    g_print("remove_source1 \n");
    g_source_destroy((GSource*)userdata);
    if (gst_pad_send_event(ghostSrcPad1, gst_event_new_flush_start())) {
        g_print("upstream flushing started \n");
        if (gst_pad_send_event(ghostSinkPad1, gst_event_new_eos())) {
            g_print("EOS to flush the downstream \n");
            gst_element_set_locked_state(sourceElement1, TRUE);

            gst_element_set_state(sourceElement1, GST_STATE_NULL);

            gst_pad_unlink(ghostSrcPad1, ghostSinkPad1);
            gst_object_unref(ghostSinkPad1);
            gst_object_unref(ghostSrcPad1);

            gst_bin_remove(GST_BIN(pipeline), sourceElement1);

            /* give back the pad */
            gst_element_release_request_pad(sinkElement, sinkPad1);
            gst_object_unref(sinkPad1);

            return TRUE;
        }
    }

    return FALSE;
}

gboolean add_source3(gpointer userdata)
{
    g_print("add_source3 \n");
    GError* error = NULL;

    g_source_destroy((GSource*)userdata);

    /* Create gstreamer source bin 3 from description */
    sourceElement3 = gst_parse_bin_from_description(sourceBin3, TRUE, &error);
    if (error) {
        g_print("GStreamer -- Error parse source-bin from description. Message: %s \n", error->message);
        g_error_free(error);
        return FALSE;
    }

    /* Add sourceElement1 to pipeline */
    gst_bin_add(GST_BIN(pipeline), sourceElement3);

    /* Request sourceElement3 Pad */
    ghostSrcPad3 = gst_element_get_static_pad(sourceElement3, "src");

    sinkPad3 = gst_element_get_request_pad(mixer, "sink_%u");

    /* NULL will assign a default name for the sinkghost pad */
    ghostSinkPad3 = gst_ghost_pad_new(NULL, sinkPad3);
    gst_pad_set_active(ghostSinkPad3, TRUE);
    if (!gst_element_add_pad(sinkElement, ghostSinkPad3)) {
        g_print("could not add ghostSinkPad2 \n");
        return FALSE;
    }

    if (GST_PAD_LINK_OK != gst_pad_link(ghostSrcPad3, ghostSinkPad3)) {
        g_print("could not link pads 3 \n");
        return FALSE;
    }

    gst_element_set_state(sourceElement3, GST_STATE_PLAYING);

    return TRUE;
}

int main(int argc, char* argv[])
{
    GMainLoop* loop;

    GError* error = NULL;
    GstBus* bus;
    guint bus_watch_id;
    GSource* source;

    /* Initialisation */
    gst_init(&argc, &argv);

    loop = g_main_loop_new(NULL, FALSE);

    /* Init audio mixer pipeline */
    pipeline = gst_pipeline_new("mixer-pipeline");

    /* Create gstreamer source bin 1 from description */
    sourceElement1 = gst_parse_bin_from_description(sourceBin1, TRUE, &error);
    if (error) {
        g_print("GStreamer -- Error parse source-bin from description. Message: %s \n", error->message);
        g_error_free(error);
        return -1;
    }

    /* Create gstreamer source bin 2 from description */
    sourceElement2 = gst_parse_bin_from_description(sourceBin2, TRUE, &error);
    if (error) {
        g_print("GStreamer -- Error parse source-bin from description. Message: %s \n", error->message);
        g_error_free(error);
        return -1;
    }

    /* Create gstreamer sink bin from description */
    sinkElement = gst_parse_bin_from_description(sinkBin, TRUE, &error);
    if (error) {
        g_print("GStreamer -- Error parse source-bin from description. Message: %s \n", error->message);
        g_error_free(error);
        return -1;
    }

    /* Add sourceElement1 to pipeline */
    gst_bin_add(GST_BIN(pipeline), sourceElement1);

    /* Request sourceElement1 Pad*/
    ghostSrcPad1 = gst_element_get_static_pad(sourceElement1, "src");

    /* Add sourceElement2 to pipeline */
    gst_bin_add(GST_BIN(pipeline), sourceElement2);

    /* Request sourceElement2 Pad*/
    ghostSrcPad2 = gst_element_get_static_pad(sourceElement2, "src");

    /* Add sinkElement to pipeline */
    gst_bin_add(GST_BIN(pipeline), sinkElement);

    /* Request sinkElement Pad for Source 1*/
    mixer = gst_bin_get_by_name(GST_BIN(sinkElement), "mixer");
    if (!mixer) {
        g_print("could not get mixer element in sink bin \n");
        return -1;
    } else {
        sinkPad1 = gst_element_get_request_pad(mixer, "sink_%u");

        /* NULL will assign a default name for the sinkghost pad */
        ghostSinkPad1 = gst_ghost_pad_new(NULL, sinkPad1);
        gst_pad_set_active(ghostSinkPad1, TRUE);
        if (!gst_element_add_pad(sinkElement, ghostSinkPad1)) {
            g_print("could not add ghostSinkPad1 \n");
            return -1;
        }

        gst_object_unref(mixer);
    }

    /* link source 1 and sink 1 pads */
    if (GST_PAD_LINK_OK != gst_pad_link(ghostSrcPad1, ghostSinkPad1)) {
        g_print("could not link pads 1 \n");
        return -1;
    }

    /* Request sinkElement Pad for Source 2*/
    mixer = gst_bin_get_by_name(GST_BIN(sinkElement), "mixer");
    if (!mixer) {
        g_print("could not get mixer element in sink bin \n");
        return -1;
    } else {
        sinkPad2 = gst_element_get_request_pad(mixer, "sink_%u");

        /* NULL will assign a default name for the sinkghost pad */
        ghostSinkPad2 = gst_ghost_pad_new(NULL, sinkPad2);
        gst_pad_set_active(ghostSinkPad2, TRUE);
        if (!gst_element_add_pad(sinkElement, ghostSinkPad2)) {
            g_print("could not add ghostSinkPad2 \n");
            return -1;
        }

        gst_object_unref(mixer);
    }

    /* link source 2 and sink 2 pads */
    if (GST_PAD_LINK_OK != gst_pad_link(ghostSrcPad2, ghostSinkPad2)) {
        g_print("could not link pads 2 \n");
        return -1;
    }

    /* we add a message handler */
    bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
    bus_watch_id = gst_bus_add_watch(bus, bus_call, loop);
    gst_object_unref(bus);

    /* Set the pipeline to "playing" state*/
    g_print("Now set pipeline in state playing");
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    /* Adding timeout handler for removing first source */
    source = g_timeout_source_new_seconds(TIMEOUT_ACCESSPOINT);
    g_source_set_callback(source, remove_source1, (GSource*)source, NULL);
    g_source_attach(source, NULL);

    /* Adding timeout handler for adding source 3 */
    source = g_timeout_source_new_seconds(TIMEOUT_ACCESSPOINT);
    g_source_set_callback(source, add_source3, (GSource*)source, NULL);
    g_source_attach(source, NULL);

    /* Iterate */
    g_print("Running...\n");
    g_main_loop_run(loop);

    /* Out of the main loop, clean up nicely */
    g_print("Returned, stopping playback\n");
    gst_element_set_state(pipeline, GST_STATE_NULL);

    g_print("Deleting pipeline\n");
    gst_object_unref(GST_OBJECT(pipeline));
    g_source_remove(bus_watch_id);
    g_main_loop_unref(loop);

    return 0;
}
