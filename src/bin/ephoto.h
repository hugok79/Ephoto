#ifndef _EPHOTO_H_
#define _EPHOTO_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <Eet.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_File.h>
#include <Efreet_Mime.h>
#include <Elementary.h>
#include <Eina.h>
#include <Edje.h>
#include <Evas.h>
#include <Eio.h>
#include <Ethumb_Client.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

typedef struct _Ephoto_Config Ephoto_Config;
typedef struct _Ephoto Ephoto;
typedef struct _Ephoto_Entry Ephoto_Entry;
typedef struct _Ephoto_Event_Entry_Create Ephoto_Event_Entry_Create;

typedef enum _Ephoto_State Ephoto_State;
typedef enum _Ephoto_Orient Ephoto_Orient;

Evas_Object *ephoto_window_add(const char *path);
void         ephoto_title_set(Ephoto *ephoto, const char *title);
void         ephoto_thumb_size_set(Ephoto *ephoto, int size);
Evas_Object *ephoto_thumb_add(Ephoto *ephoto, Evas_Object *parent, const char *path);
void         ephoto_thumb_path_set(Evas_Object *o, const char *path);
void         ephoto_directory_set(Ephoto *ephoto, const char *path);

Eina_Bool    ephoto_config_init(Ephoto *em);
void         ephoto_config_save(Ephoto *em);
void         ephoto_config_free(Ephoto *em);
void         ephoto_config_window(Ephoto *em);
void         ephoto_about_window(Ephoto *em);

Evas_Object *ephoto_single_browser_add(Ephoto *ephoto, Evas_Object *parent);
void         ephoto_single_browser_entry_set(Evas_Object *obj, Ephoto_Entry *entry);
void         ephoto_single_browser_path_pending_set(Evas_Object *obj, const char *path);
 /* smart callbacks called:
  * "back" - the user wants to go back to the previous screen.
  */

Evas_Object *ephoto_slideshow_add(Ephoto *ephoto, Evas_Object *parent);
void         ephoto_slideshow_entry_set(Evas_Object *obj, Ephoto_Entry *entry);
 /* smart callbacks called:
  * "back" - the user wants to go back to the previous screen.
  */

Evas_Object *ephoto_thumb_browser_add(Ephoto *ephoto, Evas_Object *parent);

/* smart callbacks called:
 * "selected" - an item in the thumb browser is selected. The selected Ephoto_Entry is passed as event_info argument.
 */

enum _Ephoto_State
{
  EPHOTO_STATE_THUMB,
  EPHOTO_STATE_SINGLE,
  EPHOTO_STATE_SLIDESHOW
};

enum _Ephoto_Orient /* matches with exif orientation tag */
{
  EPHOTO_ORIENT_0 = 1,
  EPHOTO_ORIENT_FLIP_HORIZ = 2,
  EPHOTO_ORIENT_180 = 3,
  EPHOTO_ORIENT_FLIP_VERT = 4,
  EPHOTO_ORIENT_FLIP_VERT_90 = 5,
  EPHOTO_ORIENT_90 = 6,
  EPHOTO_ORIENT_FLIP_HORIZ_90 = 7,
  EPHOTO_ORIENT_270 = 8
};

/* TODO: split into window & global config, allow multi window
 *
 * This also requires single instance, as 2 instances changing the
 * same configuration will lead to problems.
 *
 * Single instance is better done as DBus, using FDO standard methods.
 */
struct _Ephoto_Config
{
   int config_version;
   int thumb_size;
   int thumb_gen_size;
   const char *directory;
   double slideshow_timeout;
   const char *slideshow_transition;
   const char *editor;
   int window_width;
   int window_height;
   Evas_Object *window;
   Evas_Object *slide_time;
   Evas_Object *slide_trans;
   int fsel_hide;
};

struct _Ephoto
{
   Evas_Object *win;
   Evas_Object *bg;
   Evas_Object *panel;
   Evas_Object *pager;

   Evas_Object *thumb_browser;
   Evas_Object *single_browser;
   Evas_Object *slideshow;
   Elm_Object_Item *tb;
   Elm_Object_Item *sb;
   Elm_Object_Item *sl;

   Eina_List *entries;
   Eina_List *direntries;
   Eina_List *thumbs; /* live thumbs that need to be regenerated on changes */

   int thumb_gen_size; /* pending value for thumb_regen */
   Evas_Coord  bottom_bar_size;
   struct {
      Ecore_Timer *thumb_regen;
   } timer;
   struct {
      Ecore_Job *change_dir;
   } job;

   Eio_File *ls;

   Evas_Object *prefs_win;
   Ephoto_State state, prev_state;

   Ephoto_Config *config;
};

struct _Ephoto_Entry
{
   const char *path;
   const char *basename; /* pointer inside path */
   const char *label;
   Ephoto *ephoto;
   Elm_Object_Item *item;
   Eina_List *free_listeners;
   Eina_Bool is_dir;
};

struct _Ephoto_Event_Entry_Create
{
   Ephoto_Entry *entry;
};

Ephoto_Entry *ephoto_entry_new(Ephoto *ephoto, const char *path, const char *label, Eina_File_Type type);
void          ephoto_entry_free(Ephoto_Entry *entry);
void          ephoto_entry_free_listener_add(Ephoto_Entry *entry, void (*cb)(void *data, const Ephoto_Entry *entry), const void *data);
void          ephoto_entry_free_listener_del(Ephoto_Entry *entry, void (*cb)(void *data, const Ephoto_Entry *entry), const void *data);
void          ephoto_entries_free(Ephoto *ephoto);
int	      ephoto_entries_cmp(const void *pa, const void *pb);

extern int __log_domain;
#define DBG(...) EINA_LOG_DOM_DBG(__log_domain, __VA_ARGS__)
#define INF(...) EINA_LOG_DOM_INFO(__log_domain, __VA_ARGS__)
#define ERR(...) EINA_LOG_DOM_ERR(__log_domain, __VA_ARGS__)

static inline Eina_Bool
_ephoto_eina_file_direct_info_image_useful(const Eina_File_Direct_Info *info)
{
   const char *type, *bname;
   int i = 0;

   const char *filters[] =
   {
     "png", "jpeg", "jpg", "eet", "xpm", "tiff", "gif", "svg", "webp", "pmaps",
     "bmp", "tga", "wbmp", "ico", "psd", "jp2k", "generic"
   };

   bname = info->path + info->name_start;
   if (bname[0] == '.') return EINA_FALSE;
   if ((info->type != EINA_FILE_REG) && (info->type != EINA_FILE_UNKNOWN))
     return EINA_FALSE;

   type = strrchr(bname, '.');
   if(!type) return EINA_FALSE;
   int count = sizeof(filters)/sizeof(filters[0]);
   for (i=0; i < count; i++)
     {
        if (!strcasecmp(type+1, filters[i]))
          return evas_object_image_extension_can_load_get(bname);
     }
   return EINA_FALSE;
   /* seems that this does not play nice with threads */
   //if (!(type = efreet_mime_type_get(info->path))) return EINA_FALSE;
   //return strncmp(type, "image/", sizeof("image/") - 1) == 0;
}

extern int EPHOTO_EVENT_ENTRY_CREATE;
extern int EPHOTO_EVENT_POPULATE_START;
extern int EPHOTO_EVENT_POPULATE_END;
extern int EPHOTO_EVENT_POPULATE_ERROR;

#endif
