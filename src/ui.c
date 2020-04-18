/* Aryan Wadhwani, ui.c, CS 24000, Spring 2020
 * Last updated April 16, 2020
 */

/* Included Libraries */

#include "ui.h"

#include<string.h>
#include<malloc.h>
#include<ftw.h>
#include<ctype.h>

#include "parser.h"

tree_node_t *g_current_node = NULL;
song_data_t *g_current_song = NULL;
song_data_t *g_modified_song = NULL;

bool compare_strings(const char*, const char *);
char* get_file_name(const char *);
char* open_file_dialog();
char* open_folder_dialog();
void add_to_song_list(tree_node_t*, int*);
void remove_list();

// This structure contains all the widgets in GUI
struct ui_widgets {
  GtkBuilder *builder;
  GtkWidget *window;
  GtkWidget *fixed;
  GtkListBox *song_list;
  GtkButton *load_button;
  GtkButton *add_button;
  GtkLabel *file_details;
  GtkSearchBar *search_bar;
  GtkSearchEntry *search_entry;
  GtkSpinButton *time_scale;
  GtkSpinButton *warp_time;
  GtkSpinButton *song_octave;
} g_widgets;

// This structure contains all the global parameters used
// among different GUI pointers
struct parameters{
  char *folder_directory;
  char *selected_song_name;
} g_parameters;

/* Define update_song_list here */

void update_song_list(){
  int count = 0;
  remove_list();
  traverse_in_order(g_song_library, &count, (traversal_func_t) add_to_song_list);
}

void remove_list(){
  GtkListBoxRow *row = gtk_list_box_get_row_at_index(g_widgets.song_list, 0);
  while (row != NULL){
    gtk_widget_destroy(GTK_WIDGET(row));
    row = gtk_list_box_get_row_at_index(g_widgets.song_list, 0);
  }
}

void add_to_song_list(tree_node_t *node, int *count){
  GtkWidget *row = gtk_list_box_row_new();
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
  g_object_set (box, "margin-start", 10, "margin-end", 10, NULL);
  gtk_container_add (GTK_CONTAINER (row), box);
  GtkWidget *song_label = gtk_label_new(node->song_name);
  gtk_container_add_with_properties (GTK_CONTAINER (box), song_label, "expand", TRUE, NULL);
  gtk_widget_show_all(row);
  gtk_list_box_insert(g_widgets.song_list, row, *count);
  (*count)++;
  return;
}
/* Define update_drawing_area here */

void update_drawing_area(){
  return;
}

/* Define update_info here */

void update_info(){
  if (g_current_node){
    char name_string[1024];
    char full_path[1024];
    char note_range[1024];
    char original_length[1024];
    char buffer[4096];
    int low_pitch = 128;
    int high_pitch = -1;
    int length = 0;
    range_of_song(g_current_song, &low_pitch, &high_pitch, &length);
    snprintf(name_string, 1024, "File name: %s", g_current_node->song_name);
    snprintf(full_path, 1024, "Full path: %s", g_current_song->path);
    snprintf(note_range, 1024, "Note range: [%d, %d]", low_pitch, high_pitch);
    snprintf(original_length, 1024, "Original length: %d", length);
    snprintf(buffer, 4096, "%s\n%s\n%s\n%s\n", name_string, full_path, note_range, original_length);
    gtk_label_set_text(g_widgets.file_details, buffer);
  }
  else {
    gtk_label_set_text(g_widgets.file_details, "Select a song from list to start....");
    g_object_set_property(G_OBJECT(g_widgets.time_scale), "editable", false);
  }
}

/* Define update_song here */

void update_song(){
  return;
}


/* Define range_of_song here */

void range_of_song(song_data_t *midi_song, int *low_pitch,
                   int *high_pitch, int *length){
  track_node_t *copy_track = midi_song->track_list;
  while (copy_track != NULL){
    event_node_t *copy_event = copy_track->track->event_list;
    while (copy_event != NULL){
      if (event_type(copy_event->event) == MIDI_EVENT_T){
        if (low_pitch){  
          if (*low_pitch > copy_event->event->midi_event.data[0]){
            *low_pitch = copy_event->event->midi_event.data[0];
          }
        }
        if (high_pitch){
          if (*high_pitch < copy_event->event->midi_event.data[0]){
            *high_pitch = copy_event->event->midi_event.data[0];
          }
        }
      }
      copy_event = copy_event->next_event;
    }
    if (length){
      (*length) = (*length) + copy_track->track->length;
    }
    copy_track = copy_track->next_track;
  }
  return;
}

/* Define activate here */

void activate(GtkApplication *app, gpointer user_data){
  g_widgets.builder = gtk_builder_new_from_file("src/ui.glade");
  g_widgets.window = gtk_application_window_new(app);
  
  gtk_window_set_title(GTK_WINDOW(g_widgets.window), "MIDI Library");
  gtk_window_set_resizable(GTK_WINDOW(g_widgets.window), false);
  gtk_window_set_default_size(GTK_WINDOW(g_widgets.window), 950, 720);
  gtk_window_set_position(GTK_WINDOW(g_widgets.window), GTK_WIN_POS_CENTER);

  g_widgets.fixed = GTK_WIDGET(gtk_builder_get_object(g_widgets.builder, "fixed_grid"));
  gtk_container_add(GTK_CONTAINER(g_widgets.window), g_widgets.fixed);

  g_widgets.file_details = GTK_LABEL(gtk_builder_get_object(g_widgets.builder, "file_details"));

  g_widgets.song_list = GTK_LIST_BOX(gtk_builder_get_object(g_widgets.builder, "song_list"));
  g_signal_connect(g_widgets.song_list, "row-activated", G_CALLBACK(song_selected_cb), NULL);

  g_widgets.search_bar = GTK_SEARCH_BAR(gtk_builder_get_object(g_widgets.builder, "search_bar"));
  g_widgets.search_entry = GTK_SEARCH_ENTRY(gtk_builder_get_object(g_widgets.builder, "search_song"));
  gtk_search_bar_connect_entry(g_widgets.search_bar, GTK_ENTRY(g_widgets.search_entry));
  g_signal_connect(g_widgets.search_entry, "search-changed", G_CALLBACK(search_bar_cb), NULL);

  g_widgets.time_scale = GTK_SPIN_BUTTON(gtk_builder_get_object(g_widgets.builder, "time_scale"));
  g_signal_connect(g_widgets.time_scale, "value-changed", G_CALLBACK(time_scale_cb), NULL);

  g_widgets.warp_time = GTK_SPIN_BUTTON(gtk_builder_get_object(g_widgets.builder, "warp_scale"));
  g_signal_connect(g_widgets.warp_time, "value-changed", G_CALLBACK(warp_time_cb), NULL);

  g_widgets.song_octave = GTK_SPIN_BUTTON(gtk_builder_get_object(g_widgets.builder, "octave_scale"));
  g_signal_connect(g_widgets.song_octave, "value-changed", G_CALLBACK(song_octave_cb), NULL);

  g_widgets.load_button = GTK_BUTTON(gtk_builder_get_object(g_widgets.builder, "load_directory"));
  g_signal_connect(g_widgets.load_button, "clicked", G_CALLBACK(load_songs_cb), NULL);
  
  g_widgets.add_button = GTK_BUTTON(gtk_builder_get_object(g_widgets.builder, "add_song"));
  g_signal_connect(g_widgets.add_button, "clicked", G_CALLBACK(add_song_cb), NULL);
  
  g_signal_connect(g_widgets.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
  gtk_builder_connect_signals(g_widgets.builder, NULL);
  gtk_widget_show_all(g_widgets.window);
}

/* Define add_song_cb here */

void add_song_cb(GtkButton *button, gpointer user_data){
  char* file_name = open_file_dialog();
  if (file_name == NULL){
    return;
  }
  tree_node_t *new_node = malloc(sizeof(tree_node_t));
  new_node->song = parse_file(file_name);
  new_node->song_name = get_file_name(new_node->song->path);
  new_node->left_child = NULL;
  new_node->right_child = NULL;
  tree_insert(&g_song_library, new_node);
  g_free(file_name);
  file_name = NULL;
  update_song_list();
}

char* open_file_dialog(){
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_OPEN;
  char *file_name = NULL;
  GtkWidget *dialog = gtk_file_chooser_dialog_new("Select Song to Import", GTK_WINDOW(g_widgets.window), 
                                       action, "_Cancel", GTK_RESPONSE_CANCEL,
                                       "_Open File", GTK_RESPONSE_ACCEPT, NULL);
  gint result = gtk_dialog_run (GTK_DIALOG(dialog));
  if (result == GTK_RESPONSE_ACCEPT){
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
    file_name = gtk_file_chooser_get_filename (chooser);
  }
  gtk_widget_destroy(dialog);
  return file_name;
}

/* Define load_songs_cb here */

void load_songs_cb(GtkButton *button, gpointer user_data){
  if (g_parameters.folder_directory != NULL){
    if (g_song_library){
      free_library(g_song_library);
      g_song_library = NULL;
    }
    g_current_node = NULL;
    g_current_song = NULL;
    g_modified_song = NULL;
    g_free(g_parameters.folder_directory);
    g_parameters.folder_directory = NULL;
  }
  g_parameters.folder_directory = open_folder_dialog();
  if (g_parameters.folder_directory == NULL){
    return;
  }
  make_library(g_parameters.folder_directory);
  update_song_list();
  update_info();
}

char* open_folder_dialog(){
  GtkFileChooserAction action = GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER;
  char *file_name = NULL;
  GtkWidget *dialog = gtk_file_chooser_dialog_new("Select Songs Directory", GTK_WINDOW(g_widgets.window), 
                                       action, "_Cancel", GTK_RESPONSE_CANCEL,
                                       "_Open Folder", GTK_RESPONSE_ACCEPT, NULL);
  gint result = gtk_dialog_run (GTK_DIALOG(dialog));
  if (result == GTK_RESPONSE_ACCEPT){
    GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
    file_name = gtk_file_chooser_get_filename (chooser);
  }
  gtk_widget_destroy(dialog);
  return file_name;
}

/* Define song_selected_cb here */

void song_selected_cb(GtkListBox *list_box, GtkListBoxRow *row){
  g_current_node = NULL;
  g_current_song = NULL;
  if (g_modified_song){
    free_song(g_modified_song);
  }
  g_modified_song = NULL;

  GList *list = gtk_container_get_children(GTK_CONTAINER(row));
  GtkWidget *box = GTK_WIDGET(list->data);
  list = gtk_container_get_children(GTK_CONTAINER(box));
  GtkWidget *label = GTK_WIDGET(list->data);
  const char *song_name = gtk_label_get_text(GTK_LABEL(label));
  g_current_node = *(find_parent_pointer(&g_song_library, song_name));
  g_current_song = g_current_node->song;
  g_modified_song = parse_file(g_current_node->song->path);
  update_info();
}

/* Define search_bar_cb here */

void search_bar_cb(GtkSearchBar *search_bar, gpointer user_data){
  update_song_list();
  const char *search_string = gtk_entry_get_text(GTK_ENTRY(g_widgets.search_entry));
  GtkListBoxRow *row = gtk_list_box_get_row_at_index(g_widgets.song_list, 0);
  int count = 0;
  while (row != NULL){
    GList *list = gtk_container_get_children(GTK_CONTAINER(row));
    GtkWidget *box = GTK_WIDGET(list->data);
    list = gtk_container_get_children(GTK_CONTAINER(box));
    GtkWidget *label = GTK_WIDGET(list->data);
    const char *song_name = gtk_label_get_text(GTK_LABEL(label));
    if (compare_strings(song_name, search_string) == false){
      gtk_widget_destroy(GTK_WIDGET(row));
      row = gtk_list_box_get_row_at_index(g_widgets.song_list, count);
    } else {
      row = gtk_list_box_get_row_at_index(g_widgets.song_list, ++count);
    }
  }
}

bool compare_strings(const char* string, const char *pattern){
  char string_low[strlen(string)];
  char pattern_low[strlen(pattern)];
  strcpy(string_low, string);
  strcpy(pattern_low, pattern);
  for (int i = 0; i < strlen(string_low); i++){
    string_low[i] = tolower(string_low[i]);
  }
  for (int i = 0; i < strlen(pattern_low); i++){
    pattern_low[i] = tolower(pattern_low[i]);
  }
  if (strstr(string_low, pattern_low) == NULL){
    return false;
  }
  return true;
}

/* Define time_scale_cb here */

void time_scale_cb(GtkSpinButton *time_scale, gpointer user_data){

}

/* Define draw_cb here */

gboolean draw_cb(GtkDrawingArea *draw_area, cairo_t *painter, gpointer user_data){
  return false;  
}

/* Define warp_time_cb here */

void warp_time_cb(GtkSpinButton *warp_scale, gpointer user_data){
  
}

/* Define song_octave_cb here */

void song_ocatave_cb(GtkSpinButton *octave_scale, gpointer user_data){
  
}

/* Define instrument_map_cb here */

void instrument_map_cb(GtkComboBoxText *picked_inst, gpointer user_data){
}

/* Define note_map_cb here */

void note_map_cb(GtkComboBoxText *picked_note, gpointer user_data){
  
}

/* Define save_song_cb here */

void save_song_cb(GtkButton *button, gpointer user_data){

}

/* Define remove_song_cb here */

void remove_song(GtkButton *button, gpointer user_data){

}

/*
 * Function called prior to main that sets up the instrument to color mapping
 */

void build_color_palette()
{
  static GdkRGBA palette[16];	

  memset(COLOR_PALETTE, 0, sizeof(COLOR_PALETTE));
  char* color_specs[] = {
    // Piano, red
    "#ff0000",
    // Chromatic percussion, brown
    "#8b4513",
    // Organ, purple
    "#800080",
    // Guitar, green
    "#00ff00",
    // Bass, blue
    "#0000ff",
    // Strings, cyan
    "#00ffff",
    // Ensemble, teal
    "#008080",
    // Brass, orange
    "#ffa500",
    // Reed, magenta
    "#ff00ff",
    // Pipe, yellow
    "ffff00",
    // Synth lead, indigo
    "#4b0082",
    // Synth pad, dark slate grar
    "#2f4f4f",
    // Synth effects, silver
    "#c0c0c0",
    // Ehtnic, olive
    "#808000",
    // Percussive, silver
    "#c0c0c0",
    // Sound effects, gray
    "#808080",
  };

  for (int i = 0; i < 16; ++i) {
    gdk_rgba_parse(&palette[i], color_specs[i]);
    for (int j = 0; j < 8; ++j) {
      COLOR_PALETTE[i * 8 + j] = &palette[i];
    }
  }
} /* build_color_palette() */
