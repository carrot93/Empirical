//  This file is part of Empirical, https://github.com/devosoft/Empirical
//  Copyright (C) Michigan State University, 2015-2017
//  Released under the MIT Software license; see doc/LICENSE
//
//
//  The Document class is built off of Div, but initializes the EMP web framework, if
//  neeeded, and activates itself.  It also provides quick ways to add and lookup
//  widgets.
//
//  For example, you can use doc.AddButon(...) to add a new button to the document,
//  where the ... can be any of the mechanisms to build a new button.  This technique
//  works for any widget type.
//
//  You can also look up any widget by name.  For example, if you previously created a
//  Canvas widget with the HTML id "my_canvas", you can look it up later by using
//  doc.Canvas("my_canvas")


#ifndef EMP_WEB_DOCUMENT_H
#define EMP_WEB_DOCUMENT_H

#include "Button.h"
#include "Canvas.h"
#include "Div.h"
#include "FileInput.h"
#include "Image.h"
#include "Selector.h"
#include "Table.h"
#include "Text.h"
#include "TextArea.h"


namespace emp {
namespace web {

  class Document : public web::Div {
  public:
    Document(const std::string & doc_id) : web::Div(doc_id) { Activate(); }
    ~Document() { ; }

    // Retrieve specific types of widgets.

    // Shortcut adders for Widgets
    template <class... T> web::Button AddButton(T &&... args){
      web::Button new_widget(std::forward<T>(args)...);
      info->Append(new_widget);
      return new_widget;
    }
    template <class... T> web::Canvas AddCanvas(T &&... args){
      web::Canvas new_widget(std::forward<T>(args)...);
      info->Append(new_widget);
      return new_widget;
    }
    template <class... T> web::FileInput AddFileInput(T &&... args){
      web::FileInput new_widget(std::forward<T>(args)...);
      info->Append(new_widget);
      return new_widget;
    }
    template <class... T> web::Image AddImage(T &&... args) {
      web::Image new_widget(std::forward<T>(args)...);
      info->Append(new_widget);
      return new_widget;
    }
    template <class... T> web::Selector AddSelector(T &&... args){
      web::Selector new_widget(std::forward<T>(args)...);
      info->Append(new_widget);
      return new_widget;
    }
    template <class... T> web::Div AddDiv(T &&... args) {
      web::Div new_widget(std::forward<T>(args)...);
      info->Append(new_widget);
      return new_widget;
    }
    template <class... T> web::Table AddTable(T &&... args) {
      web::Table new_widget(std::forward<T>(args)...);
      info->Append(new_widget);
      return new_widget;
    }
    template <class... T> web::Text AddText(T &&... args)  {
      web::Text new_widget(std::forward<T>(args)...);
      info->Append(new_widget);
      return new_widget;
    }
    template <class... T> web::TextArea AddTextArea(T &&... args)  {
      web::TextArea new_widget(std::forward<T>(args)...);
      info->Append(new_widget);
      return new_widget;
    }


    // Setup a quick way to retrieve old widgets by name.
    web::Button Button (const std::string & in_id) { return web::Button(Find(in_id)); }
    web::Canvas Canvas (const std::string & in_id) { return web::Canvas(Find(in_id)); }
    web::FileInput FileInput (const std::string & in_id) { return web::FileInput(Find(in_id)); }
    web::Image Image (const std::string & in_id) { return web::Image(Find(in_id)); }
    web::Selector Selector (const std::string & in_id) { return web::Selector(Find(in_id)); }
    web::Div Div (const std::string & in_id) { return web::Div(Find(in_id)); }
    web::Table Table (const std::string & in_id) { return web::Table(Find(in_id)); }
    web::Text Text (const std::string & in_id) { return web::Text(Find(in_id)); }
    web::TextArea TextArea (const std::string & in_id) { return web::TextArea(Find(in_id)); }

  };

}
}


#endif
