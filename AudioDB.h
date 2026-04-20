// AudioDB.h
#ifndef AUDIODB_H
#define AUDIODB_H

#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING

#include <string>
#include <filesystem>
#include <unordered_map>

#include "rapidjson/document.h"

#include "AudioHelper.h"
#include "Helper.h"

class AudioDB {
private:
    static std::string ResolveAudioPathOrDie(const std::string& clip_name_sans_ext);

public:
	AudioDB();
    static inline std::unordered_map<std::string, Mix_Chunk*> chunk_cache;
    static Mix_Chunk* GetChunk(const std::string& clip_name);
};

#endif
