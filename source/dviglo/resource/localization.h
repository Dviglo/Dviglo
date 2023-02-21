// Copyright (c) 2008-2023 the Urho3D project
// Copyright (c) 2022-2023 the Dviglo project
// License: MIT

#pragma once

#include "../core/context.h"
#include "json_value.h"

namespace dviglo
{

/// %Localization subsystem. Stores all the strings in all languages.
class DV_API Localization : public Object
{
    DV_OBJECT(Localization, Object);

public:
    /// Construct.
    explicit Localization();
    /// Destruct. Free all resources.
    ~Localization() override;

    /// Return the number of languages.
    int GetNumLanguages() const { return (int)languages_.Size(); }

    /// Return the index number of current language. The index is determined by the order of loading.
    int GetLanguageIndex() const { return languageIndex_; }

    /// Return the index number of language. The index is determined by the order of loading.
    int GetLanguageIndex(const String& language);
    /// Return the name of current language.
    String GetLanguage();
    /// Return the name of language.
    String GetLanguage(int index);
    /// Set current language.
    void SetLanguage(int index);
    /// Set current language.
    void SetLanguage(const String& language);
    /// Return a string in the current language. Returns String::EMPTY if id is empty. Returns id if translation is not found and logs a warning.
    String Get(const String& id);
    /// Clear all loaded strings.
    void Reset();
    /// Load strings from JSONFile. The file should be UTF8 without BOM.
    void LoadJSONFile(const String& name, const String& language = String::EMPTY);
    /// Load strings from JSONValue.
    void LoadMultipleLanguageJSON(const JSONValue& source);
    /// Load strings from JSONValue for specific language.
    void LoadSingleLanguageJSON(const JSONValue& source, const String& language = String::EMPTY);

private:
    /// Language names.
    Vector<String> languages_;
    /// Index of current language.
    int languageIndex_;
    /// Storage strings: <Language <StringId, Value>>.
    HashMap<StringHash, HashMap<StringHash, String>> strings_;
};

}
