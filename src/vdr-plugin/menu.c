/*
 * menu.c: Web video plugin for the Video Disk Recorder
 *
 * See the README file for copyright information and how to reach the author.
 *
 * $Id$
 */

#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <vdr/skins.h>
#include <vdr/tools.h>
#include <vdr/i18n.h>
#include <vdr/osdbase.h>
#include <vdr/skins.h>
#include <vdr/font.h>
#include <vdr/osd.h>
#include <vdr/interface.h>
#include "menu.h"
#include "download.h"
#include "config.h"
#include "common.h"
#include "history.h"
#include "timer.h"
#include "menu_timer.h"

cCharSetConv csc = cCharSetConv("UTF-8", cCharSetConv::SystemCharacterTable());
struct MenuPointers menuPointers;

typedef enum {
  INPUT_TYPE_TEXT,
  INPUT_TYPE_RADIO,
  INPUT_TYPE_SUBMIT,
} InputItemType;

// --- cURITemplate ----------------------------------------------

class cURITemplate {
private:
  static const char *GetValue(const char *key, const cStringList& keys,
                              const cStringList& values);
public:
  static cString Substitute(const cString& uritemplate, const cStringList& keys,
                            const cStringList& values);
};

cString cURITemplate::Substitute(const cString& uritemplate,
                                 const cStringList& keys,
                                 const cStringList& values)
{
  if ((const char *)uritemplate == NULL)
    return "";

  cVector<char> substituted(2*strlen(uritemplate));
  const char *currentKey = NULL;
  const char *c = uritemplate;
  while (*c) {
    if (currentKey) {
      if (*c == '}') {
        char *key = strndup(currentKey+1, c - (currentKey+1));
        const char *value = cURITemplate::GetValue(key, keys, values);
        if (value) {
          for (const char *v = value; *v; v++) {
            if (*v == ' ') {
              substituted.Append('+');
            } else {
              substituted.Append(*v);
            }
          }
        } else {
          for (const char *v = currentKey; v <= c; v++) {
            substituted.Append(*v);
          }
        }
        currentKey = NULL;
        free(key);
      }
    } else if (*c == '{') {
      currentKey = c;
    } else {
      substituted.Append(*c);
    }

    c++;
  }

  if (currentKey) {
    for (const char *v=currentKey; *v; v++) {
      substituted.Append(*v);
    }
  }

  return cString(strdup(&substituted[0]), true);
}

const char *cURITemplate::GetValue(const char *key,
                                   const cStringList& keys,
                                   const cStringList& values)
{
  int i = keys.Find(key);
  if ((i >= 0) && (i < values.Size())) {
    return values[i];
  } else {
    return NULL;
  }
}

// --- cEditControl ----------------------------------------------

class cEditControl {
public:
  virtual const char *Key() = 0;
  virtual const char *Value() = 0;
};

/* // --- cFormControlWrapper --------------------------------------- */

/* class cFormControlWrapper { */
/* public: */
/*   virtual const char *Key() = 0; */
/*   virtual const char *Value() = 0; */
/*   virtual cOsdItem *CreateOSDItem() = 0; */
/* }; */

/* // --- cTextFieldWrapper ----------------------------------------- */

/* class cTextFieldWrapper : public cFormControlWrapper { */
/* private: */
/*   char *value; */
/*   int valueLen; */
/*   char *key; */
/*   char *name; */
/*   char *allowed; */
/* public: */
/*   cTextFieldWrapper(const char *Name, const char *Key, int MaxLength, const char *Allowed = NULL); */
/*   ~cTextFieldWrapper(); */
/*   virtual const char *Key() { return key; } */
/*   virtual const char *Value(); */
/*   virtual cOsdItem *CreateOSDItem(); */
/* }; */

/* cTextFieldWrapper::cTextFieldWrapper(const char *Name, const char *Key, */
/*                                      int MaxLength, const char *Allowed = NULL) */
/* { */
/*   valueLen = MaxLength; */
/*   value = malloc(valueLen + 1); */
/*   *value = '\0'; */
/*   key = strdup(Key ? Key : ""); */
/*   name = strdup(Name ? Name : ""); */
/*   allowed = Allowed ? strdup(Allowed) : NULL; */
/* } */

/* cTextFieldWrapper::~cTextFieldWrapper() { */
/*   free(value); */
/*   free(key); */
/*   free(name); */
/*   if (allowed) */
/*     free(allowed); */
/* } */

/* cMenuEditStrItem *cTextFieldWrapper::CreateOSDItem() { */
/*   return new cMenuEditStrItem(name, value, valueLen, allowed); */
/* } */

/* const char *cTextFieldWrapper::Value() { */
/*   return value; */
/* } */

/* // --- cSelectionWrapper ----------------------------------------- */

/* class cSelectionWrapper : public cFormControlWrapper { */
/* private: */
/*   char *key; */
/*   char **labels; */
/*   int numLabels; */
/*   cStringList values; */
/*   int selectedIndex; */
/*   char *name; */
/* public: */
/*   cSelectionWrapper(const char *Name, const char *Key, */
/*                     const cStringList& Labels, const cStringList& Values); */
/*   ~cSelectionWrapper(); */
/*   virtual const char *Key() { return key; } */
/*   virtual const char *Value(); */
/*   virtual cOsdItem *CreateOSDItem(); */
/* }; */

/* cSelectionWrapper::cSelectionWrapper(const char *Name, const char *Key, */
/*                                      const cStringList& LabelStrings, */
/*                                      const cStringList& ValueStrings) */
/* { */
/*   name = strdup(Name ? Name : ""); */
/*   key = strdup(Key ? Key : ""); */
/*   selectedIndex = 0; */
/*   numLabels = LabelStrings.Size(), */
/*   labels = malloc(numLabels*sizeof(char *)); */
/*   for (int i=0; i<numLabels; i++) { */
/*     labels[i] = strdup(LabelStrings[i]); */
/*   } */
/*   for (int i=0; i<ValueStrings.Size(); i++) { */
/*     values.Append(ValueStrings[i]); */
/*   } */
/* } */

/* cSelectionWrapper::~cSelectionWrapper() { */
/*   for (int i=0; i<numValues; i++) { */
/*     free(labels[i]); */
/*   } */
/*   free(labels); */
/*   free(key); */
/*   free(name); */
/* } */

/* cOsdItem *cSelectionWrapper::CreateOSDItem() { */
/*   return new cMenuEditStraItem(name, &selectedIndex, numLabels, labels); */
/* } */

/* const char *cSelectionWrapper::Value() { */
/*   if (selectedIndex < values.Size()) */
/*     return values[selectedIndex]; */
/*   else */
/*     return ""; */
/* } */

/* // --- cMenuTextField -------------------------------------------- */

/* class cMenuTextField : public cMenuEditStrItem, public cEditControl { */
/* private: */
/*   //char *textBuffer; */
/*   char *keyName; */
/* public: */
/*   cMenuTextField(const char *Name, const char *Key, int MaxLength, const char *Allowed = NULL); */
/*   ~cMenuTextField(); */

/*   const char *Key(); */
/*   const char *Value(); */
/* }; */

/* cMenuTextField::cMenuTextField(const char *Name, const char *Key, int MaxLength, const char *Allowed) */
/* : cMenuEditStrItem(Name, new uint[MaxLength], MaxLength, Allowed) */
/* { */
/*   keyName = strdup(Key); */
/* } */

/* cMenuTextField::~cMenuTextField() { */
/*   free(keyName); */

/*   // FIXME: delete value */
/* } */

/* const char *cMenuTextField::Key() { */
/*   return keyName; */
/* } */

/* const char *cMenuTextField::Value() { */
/*   // FIXME */
/* } */

// --- cOsdSubmitButton ------------------------------------------

class cOsdSubmitButton : public cOsdItem, public cMenuLink {
private:
  cVector<cEditControl *> editControls;
  cString substituted;
  char *uriTemplate;

public:
  cOsdSubmitButton(const char *Text, eOSState State = osUnknown, bool Selectable = true);

  void SetURITemplate(const char *newTemplate);
  void ClearEditControls();
  void AttachEditControl(cEditControl *control);

  const char *GetURL();
  bool HasStream();
};

cOsdSubmitButton::cOsdSubmitButton(const char *Text, eOSState State, bool Selectable)
: cOsdItem(Text, State, Selectable), uriTemplate(NULL)
{
}

void cOsdSubmitButton::SetURITemplate(const char *newTemplate) {
  if (uriTemplate)
    free(uriTemplate);

  if (newTemplate)
    uriTemplate = strdup(newTemplate);
  else
    uriTemplate = NULL;
}

void cOsdSubmitButton::ClearEditControls() {
  editControls.Clear();
}

void cOsdSubmitButton::AttachEditControl(cEditControl *control) {
  editControls.Append(control);
}

const char *cOsdSubmitButton::GetURL() {
  if (uriTemplate) {
    cStringList keys;
    cStringList values;
    for (int i=0; i<editControls.Size(); i++) {
      keys.Append(strdup(editControls[i]->Key()));
      values.Append(strdup(editControls[i]->Value()));
    }
    substituted = cURITemplate::Substitute(uriTemplate, keys, values);
  } else {
    substituted = "";
  }

  return substituted;
}

bool cOsdSubmitButton::HasStream() {
  return false;
}

// --- cFormItem -------------------------------------------------

class cFormItem : public cListObject {
private:
  InputItemType type;
  char *name;
  char *mainLabel;
protected:
  cFormItem(InputItemType _type, const char *_name, const char *_mainLabel);
public:
  virtual ~cFormItem();
  InputItemType GetType();
  const char *GetName();
  virtual const char *GetLabel();
  virtual void AppendValue(const char *value, const char *label);
  virtual cOsdItem *CreateOsdItem() = 0;
};

cFormItem::cFormItem(InputItemType _type, const char *_name, const char *_mainLabel) {
  type = _type;
  name = strdup(_name ? _name : "");
  mainLabel = strdup(_mainLabel ? _mainLabel : "");
}

cFormItem::~cFormItem() {
  if (name)
    free(name);
  if (mainLabel)
    free(mainLabel);
}

InputItemType cFormItem::GetType() {
  return type;
}

const char *cFormItem::GetName() {
  return name;
}

const char *cFormItem::GetLabel() {
  return mainLabel;
}

void cFormItem::AppendValue(const char *value, const char *label) {
  // default implementation does nothing
}

// --- cFormItemText ---------------------------------------------

class cFormItemText : public cFormItem, public cEditControl {
private:
  char *value;
  int valueLen;
  //char *key; // name? FIXME
  char *allowed;
public:
  cFormItemText(const char *_name, const char *_mainLabel, int MaxLength, const char *Allowed = NULL);
  ~cFormItemText();
  const char *Key();
  const char *Value();
  cOsdItem *CreateOsdItem();
};

cFormItemText::cFormItemText(const char *_name, const char *_mainLabel, int MaxLength, const char *Allowed)
: cFormItem(INPUT_TYPE_TEXT, _name, _mainLabel)
{
  valueLen = MaxLength;
  value = (char *)malloc(valueLen + 1);
  *value = '\0';
  //key = strdup(Key ? Key : "");
  allowed = Allowed ? strdup(Allowed) : NULL;
}

cFormItemText::~cFormItemText() {
  free(value);
  //free(key);
  if (allowed)
    free(allowed);
}

const char *cFormItemText::Key() {
  return GetName();
}

const char *cFormItemText::Value() {
  return value;
}

cOsdItem *cFormItemText::CreateOsdItem() {
  return new cMenuEditStrItem(GetName(), value, valueLen, allowed);
}

// --- cFormItemRadio --------------------------------------------

class cFormItemRadio : public cFormItem, public cEditControl {
private:
  cStringList values;
  cStringList labels;

  //char *key; // name? FIXME
  char **labelsArray;
  int labelsArraySize;
  int selectedIndex;

public:
  cFormItemRadio(const char *_name, const char *_mainLabel);
  ~cFormItemRadio();
  const char *Key();
  const char *Value();
  void AppendValue(const char *value, const char *label);
  cOsdItem *CreateOsdItem();
};

cFormItemRadio::cFormItemRadio(const char *_name, const char *_mainLabel)
: cFormItem(INPUT_TYPE_RADIO, _name, _mainLabel)
{
  labelsArray = NULL;
  labelsArraySize = 0;
  selectedIndex = 0;
}

cFormItemRadio::~cFormItemRadio() {
  if (labelsArray) {
    for (int i=0; i<labelsArraySize; i++) {
      free(labelsArray[i]);
    }
    free(labelsArray);
  }
}

void cFormItemRadio::AppendValue(const char *value, const char *label) {
  values.Append(strdup(value ? value : ""));
  labels.Append(strdup(label ? label : ""));
}

const char *cFormItemRadio::Key() {
  return GetName();
}

const char *cFormItemRadio::Value() {
  if (selectedIndex < values.Size())
    return values[selectedIndex];
  else
    return "";
}

cOsdItem *cFormItemRadio::CreateOsdItem() {
  if (!labelsArray) {
    labelsArray = (char **)malloc(labels.Size()*sizeof(char *));
    for (int i=0; i<labels.Size(); i++) {
      labelsArray[i] = strdup(labels[i]);
    }
  }

  return new cMenuEditStraItem(GetName(), &selectedIndex,
                               labelsArraySize, labelsArray);
}

// --- cFormItemSubmit -------------------------------------------

class cFormItemSubmit : public cFormItem {
private:
  char *value;
public:
  cFormItemSubmit(const char *_name, const char *_mainLabel);
  ~cFormItemSubmit();
  const char *GetLabel();
  void AppendValue(const char *value, const char *label);
  cOsdItem *CreateOsdItem();
};

cFormItemSubmit::cFormItemSubmit(const char *_name, const char *_mainLabel)
: cFormItem(INPUT_TYPE_SUBMIT, _name, _mainLabel)
{
  value = strdup("");
}

cFormItemSubmit::~cFormItemSubmit() {
  if (value)
    free(value);
}

const char *cFormItemSubmit::GetLabel() {
  return value;
}

void cFormItemSubmit::AppendValue(const char *_value, const char *_label) {
  if (value)
    free(value);
  value = strdup(_value ? _value : "");
}

cOsdItem *cFormItemSubmit::CreateOsdItem() {
  return new cOsdSubmitButton(csc.Convert(GetLabel()));
}

// --- cFormItemList ---------------------------------------------

cFormItem *cFormItemList::FindByName(const char *name) {
  cFormItem *item = inputItems.First();
  while (item) {
    if (strcmp(item->GetName(), name) == 0) {
      return item;
    }
    item = inputItems.Next(item);
  }

  return NULL;
}

void cFormItemList::AddInputItem(const char *name, const char *type,
                                 const char *mainLabel, const char *value,
                                 const char *valueLabel)
{
  if (!name || !type)
    return;

  cFormItem *item = FindByName(name);
  if (item) {
    item->AppendValue(value, valueLabel);
  } else {
    cFormItem *item = FormItemFactory(type, name, mainLabel);
    item->AppendValue(value, valueLabel);
    inputItems.Add(item);
  }
}

cFormItem *cFormItemList::FormItemFactory(const char *type,
                                          const char *name,
                                          const char *mainLabel) {
  if (strcmp(type, "radio") == 0) {
    return new cFormItemRadio(name, mainLabel);
  } else if (strcmp(type, "submit") == 0) {
    return new cFormItemSubmit(name, mainLabel);
  } else {
    if (strcmp(type, "text") != 0)
      warning("Unexpected <input> type %s", type);
    return new cFormItemText(name, mainLabel, 255);
  }
}

void cFormItemList::CreateAndAppendOsdItems(cList<cOsdItem> *destination,
                                            const char *uriTemplate) {
  cVector<cEditControl *> editControls;
  cVector<cOsdSubmitButton *> submitButtons;
  cFormItem *inputItem;
  for (inputItem=inputItems.First(); inputItem; inputItems.Next(inputItem)) {
    cOsdItem *osdItem = inputItem->CreateOsdItem();
    if (inputItem->GetType() == INPUT_TYPE_SUBMIT) {
      assert(dynamic_cast<cOsdSubmitButton *>(osdItem));
      submitButtons.Append(static_cast<cOsdSubmitButton *>(osdItem));
    } else {
      cEditControl *edit = dynamic_cast<cEditControl *>(inputItem);
      assert(edit);
      editControls.Append(edit);
    }

    destination->Add(osdItem);
  }

  for (int i=0; i<submitButtons.Size(); i++) {
    cOsdSubmitButton *submitButton = submitButtons[i];
    submitButton->SetURITemplate(uriTemplate);
    submitButton->ClearEditControls();
    for (int j=0; j<editControls.Size(); j++) {
      submitButton->AttachEditControl(editControls[j]);
    }
  }
}

// --- cXMLMenu --------------------------------------------------

cXMLMenu::cXMLMenu(const char *Title, int c0, int c1, int c2,
				       int c3, int c4)
: cOsdMenu(Title, c0, c1, c2, c3, c4)
{
}

bool cXMLMenu::Deserialize(const char *xml) {
  xmlDocPtr doc = xmlParseMemory(xml, strlen(xml));
  if (!doc) {
    xmlErrorPtr xmlerr =  xmlGetLastError();
    if (xmlerr) {
      error("libxml error: %s", xmlerr->message);
    }

    return false;
  }

  xmlNodePtr node = xmlDocGetRootElement(doc);
  if (node)
    node = node->xmlChildrenNode;

  while (node) {
    if (node->type == XML_ELEMENT_NODE) {
      if (!ParseRootChild(doc, node)) {
        warning("Failed to parse menu tag: %s", (char *)node->name);
      }
    }
    node = node->next;
  }

  xmlFreeDoc(doc);
  return true;
}

int cXMLMenu::Load(const char *xmlstr) {
  Clear();
  Deserialize(xmlstr);

  return 0;
}


// --- cNavigationMenu -----------------------------------------------------

cNavigationMenu::cNavigationMenu(cHistory *_history,
                                 cProgressVector& dlsummaries)
  : cXMLMenu("", 25), summaries(dlsummaries),
    title(NULL), reference(NULL), shortcutMode(0), history(_history)
{
  UpdateHelp();
}

cNavigationMenu::~cNavigationMenu() {
  menuPointers.navigationMenu = NULL;
  Clear();
  if (reference)
    free(reference);
}

bool cNavigationMenu::ParseRootChild(xmlDocPtr doc, xmlNodePtr node) {
  if (!xmlStrcmp(node->name, BAD_CAST "ul")) {
    ParseUL(doc, node);
  } else if (!xmlStrcmp(node->name, BAD_CAST "form")) {
    ParseForm(doc, node);
  } else if (!xmlStrcmp(node->name, BAD_CAST "title")) {
    NewTitle(doc, node);
  } else {
    return false;
  }

  return true;
}

void cNavigationMenu::ParseUL(xmlDocPtr doc, xmlNodePtr node) {
  xmlNodePtr child = node->children;
  while (child) {
    if (xmlStrEqual(child->name, BAD_CAST "il")) {
      CreateLinkElement(doc, child);
    }
    child = child->next;
  }
}

void cNavigationMenu::CreateLinkElement(xmlDocPtr doc, xmlNodePtr node) {
  xmlNodePtr child = node->children;

  while (child) {
    if (xmlStrEqual(child->name, BAD_CAST "a")) {
      xmlChar *href = xmlGetProp(child, BAD_CAST "href");
      if (href) {
        xmlChar *title = xmlNodeListGetString(doc, child->xmlChildrenNode, 1);
        if (!title) {
          title = xmlCharStrdup("???");
        }
        xmlChar *cls = xmlGetProp(child, BAD_CAST "class");
        bool isStream = !cls || !xmlStrEqual(cls, BAD_CAST "webvi");

        CreateAndAddOSDLink((char *)title, (char *)href, isStream);

        if (cls)
          xmlFree(cls);
        xmlFree(title);
        xmlFree(href);
      }

      break;
    }

    child = child->next;
  }
}

void cNavigationMenu::CreateAndAddOSDLink(const char *title, const char *href,
                                          bool isStream) {
  char *strippedTitle = compactspace(strdup(title));
  const char *titleconv = csc.Convert(strippedTitle);
  free(strippedTitle);
  strippedTitle = NULL;
  cOsdItem *item = new cOsdItem(titleconv);
  cSimpleLink *objlinkdata = NULL;
  cSimpleLink *linkdata = NULL;
  if (href)
    linkdata = new cSimpleLink(href);
  if (isStream) {
    // stream link
    objlinkdata = new cSimpleLink(href);
  } else {
    // navigation link
    char *bracketed = (char *)malloc((strlen(titleconv)+3)*sizeof(char));
    if (bracketed) {
      bracketed[0] = '\0';
      strcat(bracketed, "[");
      strcat(bracketed, titleconv);
      strcat(bracketed, "]");
      item->SetText(bracketed, false);
    }
  }
  AddLinkItem(item, linkdata, objlinkdata);
}

void cNavigationMenu::AddLinkItem(cOsdItem *item,
                                  cLinkBase *ref, 
                                  cLinkBase *streamref) {
  Add(item);

  if (ref)
    links.Append(ref);
  else
    links.Append(NULL);

  if (streamref)
    streams.Append(streamref);
  else
    streams.Append(NULL);
}

void cNavigationMenu::ParseForm(xmlDocPtr doc, xmlNodePtr node) {
  xmlChar *urltemplate = xmlGetProp(node, BAD_CAST "action");
  xmlNodePtr child = node->children;
  while (child) {
    if (xmlStrEqual(child->name, BAD_CAST "li")) {
      ParseFormItem(formItems, doc, child);
    }
    child = child->next;
  }

  formItems.CreateAndAppendOsdItems(this, (const char *)urltemplate);

  xmlFree(urltemplate);
}

void cNavigationMenu::ParseFormItem(cFormItemList& formItems, xmlDocPtr doc,
                                    xmlNodePtr node) {
  xmlChar *mainLabel = xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
  if (!mainLabel)
    mainLabel = xmlCharStrdup("???");
  xmlNodePtr child = node->children;
  while (child) {
    if (xmlStrEqual(child->name, BAD_CAST "input")) {
      xmlChar *type = xmlGetProp(child, BAD_CAST "type");
      xmlChar *name = xmlGetProp(child, BAD_CAST "name");
      xmlChar *value = xmlGetProp(child, BAD_CAST "value");
      formItems.AddInputItem((const char *)name, (const char *)type,
                             (const char*)mainLabel, (const char*)value, NULL);
      if (value)
        xmlFree(value);
      if (name)
        xmlFree(name);
      if (type)
        xmlFree(type);
      
    } else if (xmlStrEqual(child->name, BAD_CAST "label")) {
      xmlChar *itemLabel = xmlNodeListGetString(doc, child->xmlChildrenNode, 1);
      xmlNodePtr inputNode = child->children;
      while (inputNode) {
        if (xmlStrEqual(inputNode->name, BAD_CAST "input")) {
          xmlChar *type = xmlGetProp(inputNode, BAD_CAST "type");
          xmlChar *name = xmlGetProp(inputNode, BAD_CAST "name");
          xmlChar *value = xmlGetProp(inputNode, BAD_CAST "value");
          formItems.AddInputItem((const char*)name, (const char*)type,
                                 (const char *)mainLabel, (const char*)value,
                                 (const char*)itemLabel);
          if (value)
            xmlFree(value);
          if (name)
            xmlFree(name);
          if (type)
            xmlFree(type);
        }
        inputNode = inputNode->next;
      }
      if (itemLabel)
        xmlFree(itemLabel);
    }
    child = child->next;
  }
  xmlFree(mainLabel);
}

void cNavigationMenu::NewTitle(xmlDocPtr doc, xmlNodePtr node) {
  xmlChar *newtitle = xmlNodeListGetString(doc, node->xmlChildrenNode, 1);
  if (newtitle) {
    const char *conv = csc.Convert((char *)newtitle);
    SetTitle(conv);
    if (title)
      free(title);
    title = strdup(conv);
    title = compactspace(title);
    xmlFree(newtitle);
  }
}

eOSState cNavigationMenu::ProcessKey(eKeys Key)
{
  cWebviTimer *timer;
  bool hasStreams;
  int old = Current();
  eOSState state = cXMLMenu::ProcessKey(Key);
  bool validItem = Current() >= 0 && Current() < links.Size();

  if (HasSubMenu())
    return state;

  if (state == osUnknown) {
    switch (Key) {
    case kInfo:
      // The alternative link is active only when object links are
      // present.
      if (validItem && streams.At(Current()))
        state = Select(links.At(Current()), LT_REGULAR);
      break;

    case kOk:
      // Primary action: download media object or, if not a media
      // link, follow the navigation link.
      if (validItem) {
        if (streams.At(Current()))
          state = Select(streams.At(Current()), LT_MEDIA);
        else
          state = Select(links.At(Current()), LT_REGULAR);
      }
      break;

    case kRed:
      if (shortcutMode == 0) {
        state = HistoryBack();
      } else {
        menuPointers.statusScreen = new cStatusScreen(summaries);
        state = AddSubMenu(menuPointers.statusScreen);
      }
      break;

    case kGreen:
      if (shortcutMode == 0) {
        state = HistoryForward();
      } else {
        return AddSubMenu(new cWebviTimerListMenu(cWebviTimerManager::Instance()));
      }
      break;

    case kYellow:
      if (shortcutMode == 0) {
        hasStreams = false;
        for (int i=0; i < streams.Size(); i++) {
          if (streams[i]) {
            hasStreams = true;
            break;
          }
        }

        if (hasStreams || Interface->Confirm(tr("No streams on this page, create timer anyway?"))) {
          timer = cWebviTimerManager::Instance().Create(title, reference);
          if (timer)
            return AddSubMenu(new cEditWebviTimerMenu(*timer, true, false));
        }

        state = osContinue;
      }
      break;

    case kBlue:
      if (shortcutMode == 0) {
        // Secondary action: start streaming if a media object
        if (validItem && streams.At(Current()))
          state = Select(streams.At(Current()), LT_STREAMINGMEDIA);
      }
      break;

    case k0:
      shortcutMode = shortcutMode == 0 ? 1 : 0;
      UpdateHelp();
      break;

    default:
      break;
    }
  } else {
    // If the key press caused the selected item to change, we need to
    // update the help texts.
    //
    // In cMenuEditStrItem key == kOk with state == osContinue
    // indicates leaving the edit mode. We want to update the help
    // texts in this case also.
    if ((old != Current()) || 
        ((Key == kOk) && (state == osContinue))) {
      UpdateHelp();
    }
  }

  return state;
}

eOSState cNavigationMenu::Select(cLinkBase *link, eLinkType type)
{
  if (!link) {
    return osContinue;
  }
  char *ref = link->GetURL();
  if (!ref) {
    error("link->GetURL() == NULL in cNavigationMenu::Select");
    return osContinue;
  }

  if (type == LT_MEDIA) {
    cDownloadProgress *progress = summaries.NewDownload();
    cFileDownloadRequest *req = \
      new cFileDownloadRequest(history->Current()->GetID(), ref, progress);
    cWebviThread::Instance().AddRequest(req);

    Skins.Message(mtInfo, tr("Downloading in the background"));
  } else if (type == LT_STREAMINGMEDIA) {
    cWebviThread::Instance().AddRequest(new cStreamUrlRequest(history->Current()->GetID(),
						     ref));
    Skins.Message(mtInfo, tr("Starting player..."));
    return osEnd;
  } else {
    cWebviThread::Instance().AddRequest(new cMenuRequest(history->Current()->GetID(),
        REQT_MENU, ref));
    Skins.Message(mtStatus, tr("Retrieving..."));
  }

  return osContinue;
}

void cNavigationMenu::Clear(void) {
  cXMLMenu::Clear();
  SetTitle("");
  if (title)
    free(title);
  title = NULL;
  for (int i=0; i < links.Size(); i++) {
    if (links[i])
      delete links[i];
    if (streams[i])
      delete streams[i];
  }
  links.Clear();
  streams.Clear();
}

void cNavigationMenu::Populate(const cHistoryObject *page, const char *statusmsg) {
  Load(page->GetOSD());
  
  if (reference)
    free(reference);
  reference = strdup(page->GetReference());

  // Make sure that an item is selected (if there is at least
  // one). The help texts are not updated correctly if no item is
  // selected.

  SetCurrent(Get(page->GetSelected()));
  UpdateHelp();
  SetStatus(statusmsg);
}

eOSState cNavigationMenu::HistoryBack() {
  cHistoryObject *cur = history->Current();

  if (cur)
    cur->RememberSelected(Current());

  cHistoryObject *page = history->Back();
  if (page) {
    Populate(page);
    Display();
  }
  return osContinue;
}

eOSState cNavigationMenu::HistoryForward() {
  cHistoryObject *before = history->Current();
  cHistoryObject *after = history->Forward();

  if (before)
    before->RememberSelected(Current());

  // Update only if the menu really changed
  if (before != after) {
    Populate(after);
    Display();
  }
  return osContinue;
}

void cNavigationMenu::UpdateHelp() {
  const char *red = NULL;
  const char *green = NULL;
  const char *yellow = NULL;
  const char *blue = NULL;

  if (shortcutMode == 0) {
    red = (history->Current() != history->First()) ? tr("Back") : NULL;
    green = (history->Current() != history->Last()) ? tr("Forward") : NULL;
    yellow = (Current() >= 0) ? tr("Create timer") : NULL;
    blue = ((Current() >= 0) && (streams.At(Current()))) ? tr("Play") : NULL;
  } else {
    red = tr("Status");
    green = tr("Timers");
  }

  SetHelp(red, green, yellow, blue);
}

// --- cStatusScreen -------------------------------------------------------

cStatusScreen::cStatusScreen(cProgressVector& dlsummaries)
  : cOsdMenu(tr("Unfinished downloads"), 40), summaries(dlsummaries)
{
  int charsperline = cOsd::OsdWidth() / cFont::GetFont(fontOsd)->Width('M');
  SetCols(charsperline-5);

  UpdateHelp();
  Update();
}

cStatusScreen::~cStatusScreen() {
  menuPointers.statusScreen = NULL;
}

void cStatusScreen::Update() {
  int c = Current();

  Clear();

  if (summaries.Size() == 0) {
    SetTitle(tr("No active downloads"));
  } else {

    for (int i=0; i<summaries.Size(); i++) {
      cString dltitle;
      cDownloadProgress *s = summaries[i];
      dltitle = cString::sprintf("%s\t%s",
				 (const char *)s->GetTitle(),
				 (const char *)s->GetPercentage());

      Add(new cOsdItem(dltitle));
    }

    if (c >= 0)
      SetCurrent(Get(c));
  }

  lastupdate = time(NULL);

  UpdateHelp();
  Display();
}

bool cStatusScreen::NeedsUpdate() {
  return (Count() > 0) && (time(NULL) - lastupdate >= updateInterval);
}

eOSState cStatusScreen::ProcessKey(eKeys Key) {
  cFileDownloadRequest *req;
  int old = Current();
  eOSState state = cOsdMenu::ProcessKey(Key);

  if (HasSubMenu())
    return state;

  if (state == osUnknown) {
    switch (Key) {
    case kYellow:
      if ((Current() >= 0) && (Current() < summaries.Size())) {
	if (summaries[Current()]->IsFinished()) {
	  delete summaries[Current()];
	  summaries.Remove(Current());
	  Update();
	} else if ((req = summaries[Current()]->GetRequest()) && 
		   !req->IsFinished()) {
	  req->Abort();
          Update();
	}
      }
      return osContinue;

    case kOk:
    case kInfo:
      if (summaries[Current()]->Error()) {
        cString msg = cString::sprintf("%s\n%s: %s",
                      (const char *)summaries[Current()]->GetTitle(),
                      tr("Error"),
                      (const char *)summaries[Current()]->GetStatusPharse());
        return AddSubMenu(new cMenuText(tr("Error details"), msg));
      } else {
        cString msg = cString::sprintf("%s (%s)",
                      (const char *)summaries[Current()]->GetTitle(),
                      (const char *)summaries[Current()]->GetPercentage());
        return AddSubMenu(new cMenuText(tr("Download details"), msg));
      }

      return osContinue;

    default:
      break;
    }
  } else {
    // Update help if the key press caused the menu item to change.
    if (old != Current())
      UpdateHelp();
  }

  return state;
}

void cStatusScreen::UpdateHelp() {
  bool remove = false;
  if ((Current() >= 0) && (Current() < summaries.Size())) {
    if (summaries[Current()]->IsFinished()) {
      remove = true;
    }
  }

  const char *yellow = remove ? tr("Remove") : tr("Abort");

  SetHelp(NULL, NULL, yellow, NULL);
}
