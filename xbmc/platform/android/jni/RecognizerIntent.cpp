#include "RecognizerIntent.h"
#include "Activity.h"
#include "ClassLoader.h"

#include "jutils/jutils-details.hpp"

#include <string>


using namespace jni;

std::string  CJNIRecognizerIntent::ACTION_RECOGNIZE_SPEECH;
std::string  CJNIRecognizerIntent::EXTRA_RESULTS;
std::string  CJNIRecognizerIntent::EXTRA_LANGUAGE_MODEL;
std::string  CJNIRecognizerIntent::LANGUAGE_MODEL_FREE_FORM;

const char *CJNIRecognizerIntent::m_classname = "android/speech/RecognizerIntent";

void CJNIRecognizerIntent::PopulateStaticFields()
{
  jhclass clazz = find_class(m_classname);
  ACTION_RECOGNIZE_SPEECH		= jcast<std::string>(get_static_field<jhstring>(clazz, "ACTION_RECOGNIZE_SPEECH"));
  EXTRA_RESULTS          		= jcast<std::string>(get_static_field<jhstring>(clazz, "EXTRA_RESULTS"));
  EXTRA_LANGUAGE_MODEL                  = jcast<std::string>(get_static_field<jhstring>(clazz, "EXTRA_LANGUAGE_MODEL"));
  LANGUAGE_MODEL_FREE_FORM              = jcast<std::string>(get_static_field<jhstring>(clazz, "LANGUAGE_MODEL_FREE_FORM"));
}

CJNIRecognizerIntent::CJNIRecognizerIntent()
: CJNIBase(CJNIRecognizerIntent::m_classname)
{
  m_object = new_object(CJNIContext::getClassLoader().loadClass(GetDotClassName()));
  m_object.setGlobal();
}
