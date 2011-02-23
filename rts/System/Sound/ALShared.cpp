/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "ALShared.h"
#include "SoundLog.h"

bool CheckError(const char* msg)
{
	ALenum e = alGetError();
	if (e != AL_NO_ERROR)
	{
		LogObject(sound::LOG_SOUND) << msg << ": " << (char*)alGetString(e);
		return false;
	}
	return true;
}
