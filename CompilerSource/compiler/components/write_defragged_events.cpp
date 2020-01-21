/** Copyright (C) 2008, 2018 Josh Ventura
*** Copyright (C) 2014 Seth N. Hetu
***
*** This file is a part of the ENIGMA Development Environment.
***
*** ENIGMA is free software: you can redistribute it and/or modify it under the
*** terms of the GNU General Public License as published by the Free Software
*** Foundation, version 3 of the license or any later version.
***
*** This application and its source code is distributed AS-IS, WITHOUT ANY
*** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
*** FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
*** details.
***
*** You should have received a copy of the GNU General Public License along
*** with this code. If not, see <http://www.gnu.org/licenses/>
**/

#include "makedir.h"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <set>

#include "backend/ideprint.h"

using namespace std;


#include "syntax/syncheck.h"
#include "parser/parser.h"

#include "backend/GameData.h"
#include "compiler/compile_common.h"

#include "event_reader/event_parser.h"
#include "languages/lang_CPP.h"

struct EventGroup {
  const EventDescriptor *base_event = nullptr;
  int count = 0;
  EventGroup &operator+=(const Event *ev) {
    if (!base_event) {
      base_event = ev;
    } else if (base_event != ev) {
      if (base_event->internal_id != ev->internal_id) {
        std::cerr << "Two distinct events seem to have the same name ("
                  << base_event->BaseFunctionName() << ")! Check IDs "
                  << base_event->internal_id << " (" << base_event->HumanName()
                  << ") and " << ev->internal_id << " (" << ev->HumanName()
                  << ")" << std::endl;
      } else {
        std::cerr << "FYI: two distinct event pointers exist for event "
                  << base_event->internal_id << " (" << base_event->HumanName()
                  << ")..." << std::endl;
      }
    }
    return *this;
  }
  EventGroup &operator=(const EventDescriptor *ev) {
    base_event = ev;
    return *this;
  }
  const EventDescriptor *operator->() const { return base_event; }
  EventGroup(const Event *e): base_event(e) {}
  EventGroup() = default;
};

struct UsedEventIndex {
  map<string, EventDescriptor> base_methods;
  set<Event> all;
  void insert(int mid, int id) {
    Event ev = translate_legacy_id_pair(mid, id);
    base_methods.insert({ev.BaseFunctionName(), ev});
    all.insert(ev);
  }
  void insert(const Event &ev) {
    base_methods.insert({ev.BaseFunctionName(), ev});
    all.insert(ev);
  }
};

static inline int event_get_number(const buffers::resources::Object::Event &event) {
  if (event.has_name()) {
    std::cerr << "ERROR! IMPLEMENT ME: Event names not supported in compiler.\n";
  }
  return event.number();
}

int lang_CPP::compile_writeDefraggedEvents(const GameData &game, const ParsedObjectVec &parsed_objects)
{
  /* Generate a new list of events used by the objects in
  ** this game. Only events on this list will be exported.
  ***********************************************************/
  UsedEventIndex used_events;

  // Defragged events must be written before object data, or object data cannot
  // determine which events were used.
  for (size_t i = 0; i < game.objects.size(); i++) {
    for (int ii = 0; ii < game.objects[i]->events().size(); ii++) {
      const int mid = game.objects[i]->events(ii).type();
      const int  id = event_get_number(game.objects[i]->events(ii));
      used_events.insert(mid,id);
    }
  }

  ofstream wto((codegen_directory + "Preprocessor_Environment_Editable/IDE_EDIT_evparent.h").c_str());
  wto << license;

  //Write timeline/moment names. Timelines are like scripts, but we don't have to worry about arguments or return types.
  for (size_t i = 0; i < game.timelines.size(); i++) {
    for (int j = 0; j < game.timelines[i]->moments().size(); j++) {
      wto << "void TLINE_" << game.timelines[i].name << "_MOMENT_"
          << game.timelines[i]->moments(j).step() <<"();\n";
    }
  }
  wto <<"\n";

  /* Some events are included in all objects, even if the user
  ** hasn't specified code for them. Account for those here.
  ***********************************************************/
  for (const EventDescriptor &event : event_declarations()) {
    // We may not be using this event, but it may have default code.
    if (event.HasDefaultCode()) {  // (defaulted includes "constant")
      // Defaulted events may NOT be parameterized.
      used_events.insert(Event(event));

      for (parsed_object *obj : parsed_objects) { // Then shell it out into the other objects.
        bool exists = false;
        for (unsigned j = 0; j < obj->events.size; j++) {
          Event ev2 = translate_legacy_id_pair(obj->events[j].mainId,
                                               obj->events[j].id);
          if (ev2.internal_id == event.internal_id) {
            exists = true;
            break;
          }
        }
        if (!exists) {
          std::cout << "EVENT SYSTEM: Adding a " << event.HumanName()
                    << " event with default code.\n";
          auto hack = reverse_lookup_legacy_event(event);
          obj->events[obj->events.size] = parsed_event(hack.mid, hack.id, obj);
        }
      }
    }
  }

  /* Now we forge a top-level tier for object declaration
  ** that defines default behavior for any obect's unused events.
  *****************************************************************/
  wto << "namespace enigma" << endl << "{" << endl;

  wto << "  struct event_parent: " << system_get_uppermost_tier() << endl;
  wto << "  {" << endl;
  for (const auto &str_ev : used_events.base_methods) {
    const string &fname = str_ev.first;
    const EventDescriptor &event = str_ev.second;
    const bool e_is_inst = event.IsInstance();
    if (event.HasSubCheck() && !e_is_inst) {
      wto << "    inline virtual bool myevent_" << fname << "_subcheck() { return false; }\n";
    }
    wto << (e_is_inst ? "    virtual void    myevent_" : "    virtual variant myevent_") << fname << "()";
    if (event.HasDefaultCode())
      wto << endl << "    {" << endl << "  " << event.DefaultCode() << endl << (e_is_inst ? "    }" : "    return 0;\n    }") << endl;
    else wto << (e_is_inst ? " { } // No default " : " { return 0; } // No default ") << event.HumanName() << " code." << endl;
  }

  //The event_parent also contains the definitive lookup table for all timelines, as a fail-safe in case localized instances can't find their own timelines.
  wto << "    virtual void timeline_call_moment_script(int timeline_index, int moment_index) {\n";
  wto << "      switch (timeline_index) {\n";
  for (size_t i = 0; i < game.timelines.size(); i++) {
    wto << "        case " << game.timelines[i].id() <<": {\n";
    wto << "          switch (moment_index) {\n";
    for (int j = 0; j < game.timelines[i]->moments().size(); j++) {
      wto << "            case " <<j <<": {\n";
      wto << "              ::TLINE_" << game.timelines[i].name << "_MOMENT_"
                                      << game.timelines[i]->moments(j).step() << "();\n";
      wto << "              break;\n";
      wto << "            }\n";
    }
    wto << "          }\n";
    wto << "        }\n";
    wto << "        break;\n";
  }
  wto << "      }\n";
  wto << "    }\n";

  wto << "    //virtual void unlink() {} // This is already declared at the super level." << endl;
  wto << "    virtual variant myevents_perf(int type, int numb) {return 0;}" << endl;
  wto << "    event_parent() {}" << endl;
  wto << "    event_parent(unsigned _x, int _y): " << system_get_uppermost_tier() << "(_x,_y) {}" << endl;
  wto << "  };" << endl;
  wto << "}" << endl;
  wto.close();


  /* Now we write the actual event sequence code, as
  ** well as an initializer function for the whole system.
  ***************************************************************/
  wto.open((codegen_directory + "Preprocessor_Environment_Editable/IDE_EDIT_events.h").c_str());
  wto << license;
  wto << "namespace enigma" << endl << "{" << endl;

  // Start by defining storage locations for our event lists to iterate.
  for (const auto &evfun : used_events.base_methods)
    wto << "  event_iter *event_" << evfun.first << ";" << endl;

  // Here's the initializer
  wto << "  int event_system_initialize()" << endl << "  {" << endl;
  wto << "    events = new event_iter[" << used_events.base_methods.size() << "]; // Allocated here; not really meant to change." << endl;

  int obj_high_id = 0;
  for (parsed_object *obj : parsed_objects) {
    if (obj->id > obj_high_id) obj_high_id = obj->id;
  }
  wto << "    objects = new objectid_base[" << (obj_high_id+1) << "]; // Allocated here; not really meant to change." << endl;

  int ind = 0;
  for (const auto &evfun : used_events.base_methods)
    wto << "    event_" << evfun.first << " = events + " << ind++ << ";  event_"
        << evfun.first << "->name = \"" << evfun.second.HumanName() << "\";\n";
  wto << "    return 0;" << endl;
  wto << "  }" << endl;

    // Game setting initaliser
  wto << "  int game_settings_initialize()" << endl << "  {" << endl;
  // This should only effect texture interpolation if it has not already been enabled
  if (!game.settings.general().show_cursor())
    wto << "    window_set_cursor(cr_none);" << endl;

  if (game.settings.windowing().stay_on_top())
    wto << "    window_set_stayontop(true);" << endl;

  wto << "    return 0;" << endl;
  wto << "  }" << endl;

  wto << "  variant ev_perf(int type, int numb)\n  {\n    return ((enigma::event_parent*)(instance_event_iterator->inst))->myevents_perf(type, numb);\n  }\n";

  /* Some Super Checks are more complicated than others, requiring a function. Export those functions here. */
  for (const auto &event : used_events.all) {
    if (event.HasSuperCheckFunction()) {
      wto << "  static inline bool supercheck_" << event.FunctionName()
          << "() " << event.SuperCheckFunction() << "\n\n";
    }
  }

  /* Now the event sequence */
  wto << "  int ENIGMA_events()" << endl << "  {" << endl;
  for (const EventDescriptor &event : event_declarations()) {
    auto it = used_events.base_methods.find(event.BaseFunctionName());
    if (it == used_events.base_methods.end()) continue;
    if (!event.UsesEventLoop()) continue;

    string base_indent = string(4, ' ');
    bool callsubcheck =   event.HasSubCheck()   && !event.IsInstance();
    bool emitsupercheck = event.HasSuperCheck() && !event.IsInstance();
    const string fname = event.BaseFunctionName();

    if (event.HasInsteadCode()) {
      wto << base_indent << event.InsteadCode() + "\n\n";
    } else if (emitsupercheck) {
      if (event.HasSuperCheckExpression()) {
        wto << base_indent << "if (" << Event(event).SuperCheckExpression() << ")\n";
      } else {
        wto << base_indent << "if (myevent_" << fname + "_supercheck())\n";
      }
      wto <<   base_indent << "  for (instance_event_iterator = event_" << fname << "->next; instance_event_iterator != NULL; instance_event_iterator = instance_event_iterator->next) {\n";
      if (callsubcheck) {
        wto << base_indent << "    if (((enigma::event_parent*)(instance_event_iterator->inst))->myevent_" << fname << "_subcheck()) {\n";
      }
      wto <<   base_indent << "      ((enigma::event_parent*)(instance_event_iterator->inst))->myevent_" << fname << "();\n";
      if (callsubcheck) {
        wto << base_indent << "    }\n";
      }
      wto <<   base_indent << "    if (enigma::room_switching_id != -1) goto after_events;\n"
          <<   base_indent << "  }\n";
    } else {
      wto <<   base_indent << "for (instance_event_iterator = event_" << fname << "->next; instance_event_iterator != NULL; instance_event_iterator = instance_event_iterator->next) {\n";
      if (callsubcheck) {
        wto << base_indent << "  if (((enigma::event_parent*)(instance_event_iterator->inst))->myevent_" << fname << "_subcheck()) {\n"; }
      wto <<   base_indent << "    ((enigma::event_parent*)(instance_event_iterator->inst))->myevent_" << fname << "();\n";
      if (callsubcheck) {
        wto << base_indent << "  }\n";
      }
      wto <<   base_indent << "  if (enigma::room_switching_id != -1) goto after_events;\n"
          <<   base_indent << "}\n";
    }
    wto <<     base_indent << endl
        <<     base_indent << "enigma::update_globals();" << endl
        <<     base_indent << endl;
  }
  wto << "    after_events:" << endl;
  if (game.settings.shortcuts().let_escape_end_game())
    wto << "    if (keyboard_check_pressed(vk_escape)) game_end();" << endl;
  if (game.settings.shortcuts().let_f4_switch_fullscreen())
    wto << "    if (keyboard_check_pressed(vk_f4)) window_set_fullscreen(!window_get_fullscreen());" << endl;
  if (game.settings.shortcuts().let_f1_show_game_info())
    wto << "    if (keyboard_check_pressed(vk_f1)) show_info();" << endl;
  if (game.settings.shortcuts().let_f9_screenshot())
    wto << "    if (keyboard_check_pressed(vk_f9)) {}" << endl;   //TODO: Screenshot function
  if (game.settings.shortcuts().let_f5_save_f6_load())  //TODO: uncomment after game save and load fucntions implemented
  {
    wto << "    //if (keyboard_check_pressed(vk_f5)) game_save('_save" << game.settings.general().game_id() << ".sav');" << endl;
    wto << "    //if (keyboard_check_pressed(vk_f6)) game_load('_save" << game.settings.general().game_id() << ".sav');" << endl;
  }
  // Handle room switching/game restart.
  wto << "    enigma::dispose_destroyed_instances();" << endl;
  wto << "    enigma::rooms_switch();" << endl;
  wto << "    enigma::set_room_speed(room_speed);" << endl;
  wto << "    " << endl;
  wto << "    return 0;" << endl;
  wto << "  } // event function" << endl;

  // Done, end the namespace
  wto << "} // namespace enigma" << endl;
  wto.close();

  return 0;
}
