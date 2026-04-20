#include "AudioDB.h"

namespace fs = std::filesystem;

AudioDB::AudioDB() {
	// 44100 Hz, default format, stereo, 2048 chunk size
	if (AudioHelper::Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
		std::cout << "error: failed to initialize audio";
		exit(0);
	}

	// TODO: change this random no. of channels as needed ? potential bug
	AudioHelper::Mix_AllocateChannels(50);
}

std::string AudioDB::ResolveAudioPathOrDie(const std::string& clip_name_sans_ext) {
	// .wav OR .ogg 
	std::string base = "resources/audio/";
	std::string wav = base + clip_name_sans_ext + ".wav";
	if (fs::exists(wav)) return wav;

	std::string ogg = base + clip_name_sans_ext + ".ogg";
	if (fs::exists(ogg)) return ogg;

	std::cout << "error: failed to play audio clip " << clip_name_sans_ext;
	exit(0);
}

Mix_Chunk* AudioDB::GetChunk(const std::string& clip_name) {
    auto it = chunk_cache.find(clip_name);
    if (it != chunk_cache.end()) return it->second;
    
    std::string path = ResolveAudioPathOrDie(clip_name);
    Mix_Chunk* chunk = AudioHelper::Mix_LoadWAV(path.c_str());
    chunk_cache[clip_name] = chunk;
    return chunk;
}
