//JContainersNG_Natives_JString.cpp
#include "JContainersNG_Natives.hpp"

// Count actual UTF-8 codepoints, not bytes
static size_t Utf8CharCount(const std::string& s) {
    size_t count = 0;
    for (unsigned char c : s) {
        if ((c & 0xC0) != 0x80) ++count;
    }
    return count;
}

// How many bytes of `s` (starting at `start`) make up `maxChars` codepoints?
// Never splits inside a continuation byte.
static size_t Utf8ByteLengthForChars(const std::string& s, size_t start, size_t maxChars) {
    size_t chars = 0;
    size_t bytes = 0;
    for (size_t i = start; i < s.size(); ) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if ((c & 0xC0) != 0x80) {
            if (chars >= maxChars) break;
            ++chars;
        }
        ++bytes;
        ++i;
    }
    return bytes;
}

int32_t JString_Wrap(RE::StaticFunctionTag*, std::string sourceText, int32_t charactersPerLine) {
    Handle arr = ObjectManager::Get().CreateArray();
    auto ptr = ObjectManager::Get().GetObject(arr);
    if (!ptr) return 0;

    if (charactersPerLine <= 0) {
        std::istringstream iss(sourceText);
        std::string word;
        while (iss >> word) ptr->push_back(word);
        return arr;
    }

    std::string currentLine;
    std::istringstream iss(sourceText);
    std::string word;

    while (iss >> word) {
        size_t wordChars = Utf8CharCount(word);
        size_t lineChars = Utf8CharCount(currentLine);

        if (currentLine.empty()) {
            if (wordChars > static_cast<size_t>(charactersPerLine)) {
                size_t bytePos = 0;
                while (bytePos < word.size()) {
                    size_t chunkBytes = Utf8ByteLengthForChars(word, bytePos, charactersPerLine);
                    ptr->push_back(word.substr(bytePos, chunkBytes));
                    bytePos += chunkBytes;
                }
            }
            else {
                currentLine = word;
            }
        }
        else if (lineChars + 1 + wordChars <= static_cast<size_t>(charactersPerLine)) {
            currentLine += ' ' + word;
        }
        else {
            ptr->push_back(currentLine);
            currentLine = word;
        }
    }
    if (!currentLine.empty()) ptr->push_back(currentLine);
    return arr;
}

int32_t JString_DecodeFormStringToFormId(RE::StaticFunctionTag*, std::string formString) {
    auto form = FormSerializer::DecodeForm(formString);
    return form ? static_cast<int32_t>(form->GetFormID()) : 0;
}

RE::TESForm* JString_DecodeFormStringToForm(RE::StaticFunctionTag*, std::string formString) {
    return FormSerializer::DecodeForm(formString);
}

std::string JString_EncodeFormToString(RE::StaticFunctionTag*, RE::TESForm* value) {
    if (!value) return "";
    return FormSerializer::EncodeForm(value);
}

std::string JString_EncodeFormIdToString(RE::StaticFunctionTag*, int32_t formId) {
    auto form = RE::TESForm::LookupByID(static_cast<RE::FormID>(formId));
    if (!form) return "";
    return FormSerializer::EncodeForm(form);
}

std::string JString_GenerateUUID(RE::StaticFunctionTag*) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);

    const char* hex = "0123456789abcdef";
    std::string uuid;
    uuid.reserve(36);

    for (int i = 0; i < 36; ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            uuid += '-';
        }
        else if (i == 14) {
            uuid += '4';
        }
        else if (i == 19) {
            uuid += hex[dis2(gen)];
        }
        else {
            uuid += hex[dis(gen)];
        }
    }
    return uuid;
}
