#include "ephoto.h"

#define CONFIG_VERSION 10

static int _ephoto_config_load(Ephoto *ephoto);
static Eina_Bool _ephoto_on_config_save(void *data);

static Eet_Data_Descriptor *edd = NULL;

Eina_Bool
ephoto_config_init(Ephoto *ephoto)
{
   Eet_Data_Descriptor_Class eddc;

   if (!eet_eina_stream_data_descriptor_class_set(&eddc, sizeof (eddc),
                                                  "Ephoto_Config",
                                                  sizeof(Ephoto_Config)))
     {
        ERR("Unable to create the config data descriptor!");
        return EINA_FALSE;
     }

   if (!edd) edd = eet_data_descriptor_stream_new(&eddc);

#undef T
#undef D
#define T Ephoto_Config
#define D edd
#define C_VAL(edd, type, member, dtype) \
   EET_DATA_DESCRIPTOR_ADD_BASIC(edd, type, #member, member, dtype)

   C_VAL(D, T, config_version, EET_T_INT);
   C_VAL(D, T, thumb_size, EET_T_INT);
   C_VAL(D, T, thumb_gen_size, EET_T_INT);
   C_VAL(D, T, directory, EET_T_STRING);
   C_VAL(D, T, slideshow_timeout, EET_T_DOUBLE);
   C_VAL(D, T, slideshow_transition, EET_T_STRING);
   C_VAL(D, T, editor, EET_T_STRING);
   C_VAL(D, T, window_width, EET_T_INT);
   C_VAL(D, T, window_height, EET_T_INT);
   C_VAL(D, T, fsel_hide, EET_T_INT);
   switch (_ephoto_config_load(ephoto))
     {
      case 0:
         /* Start a new config */
         ephoto->config->config_version = CONFIG_VERSION;
         ephoto->config->slideshow_timeout = 4.0;
         ephoto->config->slideshow_transition = eina_stringshare_add("fade");
         ephoto->config->editor = eina_stringshare_add("gimp %s");
         ephoto->config->window_width = 900;
         ephoto->config->window_height = 600;
         ephoto->config->fsel_hide = 0;
         break;
      default:
         return EINA_TRUE;
     }

   ephoto_config_save(ephoto);
   return EINA_TRUE;
}

void
ephoto_config_save(Ephoto *ephoto)
{
   _ephoto_on_config_save(ephoto);
}

void
ephoto_config_free(Ephoto *ephoto)
{
   free(ephoto->config);
   ephoto->config = NULL;
}

static void
_close(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *popup = data;

   evas_object_del(popup);
}

static void
_save(void *data, Evas_Object *obj EINA_UNUSED, void *event_info EINA_UNUSED)
{
   Evas_Object *popup = data;
   Ephoto *ephoto = evas_object_data_get(popup, "ephoto");

   if (elm_spinner_value_get(ephoto->config->slide_time) > 0)
     ephoto->config->slideshow_timeout = elm_spinner_value_get(ephoto->config->slide_time);
   if (elm_object_text_get(ephoto->config->slide_trans))
     eina_stringshare_replace(&ephoto->config->slideshow_transition, elm_object_text_get(ephoto->config->slide_trans));

   evas_object_del(popup);
}

static void
_hv_select(void *data EINA_UNUSED, Evas_Object *obj, void *event_info)
{
   elm_object_text_set(obj, elm_object_item_text_get(event_info));
}

static void
_spinner_changed(void *data EINA_UNUSED, Evas_Object *obj, void *event_info EINA_UNUSED)
{
   double val;
   char buf[PATH_MAX];

   val = elm_spinner_value_get(obj);
   snprintf(buf, PATH_MAX, "%%1.0f %s", ngettext("second", "seconds", val));
   elm_spinner_label_format_set(obj, buf);
}

void
ephoto_config_slideshow(Ephoto *ephoto)
{
   Evas_Object *popup, *box, *table, *label, *spinner, *hoversel, *ic, *button;
   const Eina_List *l;
   const char *transition;
   char buf[PATH_MAX];

   popup = elm_popup_add(ephoto->win);
   elm_popup_scrollable_set(popup, EINA_TRUE);
   elm_object_part_text_set(popup, "title,text", _("Slideshow Settings"));
   elm_popup_orient_set(popup, ELM_POPUP_ORIENT_CENTER);

   box = elm_box_add(popup);
   elm_box_horizontal_set(box, EINA_FALSE);
   evas_object_size_hint_weight_set(box, 0.0, 0.0);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box);

   table = elm_table_add(box);
   evas_object_size_hint_weight_set(table, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(table, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(table);

   label = elm_label_add(table);
   memset(buf, 0, PATH_MAX);
   snprintf(buf, PATH_MAX, "<b>%s:</b>", _("Show Each Slide For"));
   elm_object_text_set(label, buf);
   evas_object_size_hint_align_set(label, 0.0, EVAS_HINT_FILL);
   elm_table_pack(table, label, 0, 1, 1, 1);
   evas_object_show(label);

   spinner = elm_spinner_add(table);
   elm_spinner_editable_set(spinner, EINA_TRUE);
   memset(buf, 0, PATH_MAX);
   snprintf(buf, PATH_MAX, "%%1.0f %s", _("seconds"));
   evas_object_smart_callback_add(spinner, "changed", _spinner_changed, NULL);
   elm_spinner_label_format_set(spinner, buf);
   elm_spinner_step_set(spinner, 1);
   elm_spinner_value_set(spinner, ephoto->config->slideshow_timeout);
   elm_spinner_min_max_set(spinner, 1, 60);
   elm_table_pack(table, spinner, 1, 1, 1, 1);
   evas_object_show(spinner);
   ephoto->config->slide_time = spinner;

   label = elm_label_add(table);
   memset(buf, 0, PATH_MAX);
   snprintf(buf, PATH_MAX, "<b>%s:</b>", _("Slide Transition"));
   elm_object_text_set(label, buf);
   evas_object_size_hint_align_set(label, 0.0, EVAS_HINT_FILL);
   elm_table_pack(table, label, 0, 2, 1, 1);
   evas_object_show(label);

   hoversel = elm_hoversel_add(table);
   elm_hoversel_hover_parent_set(hoversel, ephoto->win);
   EINA_LIST_FOREACH(elm_slideshow_transitions_get(ephoto->slideshow), l, transition)
     elm_hoversel_item_add(hoversel, transition, NULL, 0, _hv_select, transition);
   elm_hoversel_item_add(hoversel, "None", NULL, 0, _hv_select, NULL);
   elm_object_text_set(hoversel, ephoto->config->slideshow_transition);
   evas_object_size_hint_weight_set(hoversel, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(hoversel, EVAS_HINT_FILL, EVAS_HINT_FILL);
   elm_table_pack(table, hoversel, 1, 2, 1, 1);
   evas_object_show(hoversel);
   ephoto->config->slide_trans = hoversel;

   elm_box_pack_end(box, table);

   ic = elm_icon_add(popup);
   elm_icon_order_lookup_set(ic, ELM_ICON_LOOKUP_FDO_THEME);
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   elm_icon_standard_set(ic, "document-save");

   button = elm_button_add(popup);
   elm_object_text_set(button, _("Save"));
   elm_object_part_content_set(button, "icon", ic);
   evas_object_smart_callback_add(button, "clicked", _save, popup);
   elm_object_part_content_set(popup, "button1", button);
   evas_object_show(button);

   ic = elm_icon_add(popup);
   elm_icon_order_lookup_set(ic, ELM_ICON_LOOKUP_FDO_THEME);
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   elm_icon_standard_set(ic, "window-close");

   button = elm_button_add(popup);
   elm_object_text_set(button, _("Cancel"));
   elm_object_part_content_set(button, "icon", ic);
   evas_object_smart_callback_add(button, "clicked", _close, popup);
   elm_object_part_content_set(popup, "button2", button);
   evas_object_show(button);

   evas_object_data_set(popup, "ephoto", ephoto);
   elm_object_part_content_set(popup, "default", box);
   evas_object_show(popup);
}

void
ephoto_config_about(Ephoto *ephoto)
{
   Evas_Object *popup, *box, *ic, *button, *label;
   Eina_Strbuf *sbuf = eina_strbuf_new();
   FILE *f;

   popup = elm_popup_add(ephoto->win);
   elm_popup_scrollable_set(popup, EINA_TRUE);
   elm_object_part_text_set(popup, "title,text", _("About Ephoto"));
   elm_popup_orient_set(popup, ELM_POPUP_ORIENT_CENTER);

   box = elm_box_add(popup);
   elm_box_horizontal_set(box, EINA_FALSE);
   evas_object_size_hint_weight_set(box, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(box, EVAS_HINT_FILL, EVAS_HINT_FILL);
   evas_object_show(box);

   label = elm_label_add(box);
   evas_object_size_hint_weight_set(label, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
   evas_object_size_hint_align_set(label, EVAS_HINT_FILL, EVAS_HINT_FILL);
   eina_strbuf_append_printf(sbuf,
     _("Ephoto is a comprehensive image viewer based on the EFL.<br/>"
     "For more information, please visit the Ephoto project page on the Enlightenment wiki:<br/>"
     "https://phab.enlightenment.org/w/projects/ephoto<br/>"
     "Ephoto's source can be found through Enlightenment's git:<br/>"
     "http://git.enlightenment.org/apps/ephoto.git<br/>"
     "<br/>"
     "<b>Authors:</b><br/>"));
   f = fopen(PACKAGE_DATA_DIR "/AUTHORS", "r");
   if (f)
     {
        char buf[PATH_MAX];
        while (fgets(buf, sizeof(buf), f))
          {
             int len;

             len = strlen(buf);
             if (len > 0)
               {
                  if (buf[len - 1] == '\n')
                    {
                       buf[len - 1] = 0;
                       len--;
                    }
                  if (len > 0)
                    {
                       char *p;

                       do
                         {
                            p = strchr(buf, '<');
                            if (p) *p = 0;
                         }
                       while (p);
                       do
                         {
                            p = strchr(buf, '>');
                            if (p) *p = 0;
                         }
                       while (p);
                       eina_strbuf_append_printf(sbuf, "%s<br>", buf);
                    }
                  if (len == 0)
                    eina_strbuf_append_printf(sbuf, "<br>");
               }
          }
        fclose(f);
     }
   elm_object_text_set(label, eina_strbuf_string_get(sbuf));
   elm_box_pack_end(box, label);
   evas_object_show(label);

   ic = elm_icon_add(box);
   elm_icon_order_lookup_set(ic, ELM_ICON_LOOKUP_FDO_THEME);
   evas_object_size_hint_aspect_set(ic, EVAS_ASPECT_CONTROL_VERTICAL, 1, 1);
   elm_icon_standard_set(ic, "window-close");

   button = elm_button_add(box);
   elm_object_text_set(button, _("Close"));
   elm_object_part_content_set(button, "icon", ic);
   evas_object_smart_callback_add(button, "clicked", _close, popup);
   elm_object_part_content_set(popup, "button1", button);
   evas_object_show(button);

   evas_object_data_set(popup, "ephoto", ephoto);
   elm_object_part_content_set(popup, "default", box);
   evas_object_show(popup);
}

static int
_ephoto_config_load(Ephoto *ephoto)
{
   Eet_File *ef;
   char buf[4096], buf2[4096];

   snprintf(buf2, sizeof(buf2), "%s/.config/ephoto", getenv("HOME"));
   ecore_file_mkpath(buf2);
   snprintf(buf, sizeof(buf), "%s/ephoto.cfg", buf2);

   ef = eet_open(buf, EET_FILE_MODE_READ);
   if (!ef)
     {
        ephoto_config_free(ephoto);
        ephoto->config = calloc(1, sizeof(Ephoto_Config));
        return 0;
     }

   ephoto->config = eet_data_read(ef, edd, "config");
   eet_close(ef);

   if (!ephoto->config || ephoto->config->config_version > CONFIG_VERSION)
     {
        ephoto_config_free(ephoto);
        ephoto->config = calloc(1, sizeof(Ephoto_Config));
        return 0;
     }

   if (ephoto->config->config_version < CONFIG_VERSION)
     {
        ecore_file_unlink(buf);
        ephoto_config_free(ephoto);
        ephoto->config = calloc(1, sizeof(Ephoto_Config));
        return 0;
     }
   return 1;
}

static Eina_Bool
_ephoto_on_config_save(void *data)
{
   Ephoto *ephoto = data;
   Eet_File *ef;
   char buf[4096], buf2[4096];

   snprintf(buf, sizeof(buf), "%s/.config/ephoto/ephoto.cfg", getenv("HOME"));
   snprintf(buf2, sizeof(buf2), "%s.tmp", buf);

   ef = eet_open(buf2, EET_FILE_MODE_WRITE);
   if (!ef) goto save_end;

   eet_data_write(ef, edd, "config", ephoto->config, 1);
   if (eet_close(ef)) goto save_end;

   if (!ecore_file_mv(buf2, buf)) goto save_end;

   INF("Config saved");

save_end:
   ecore_file_unlink(buf2);

   return ECORE_CALLBACK_CANCEL;
}
