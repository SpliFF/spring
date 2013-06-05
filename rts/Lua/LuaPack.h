/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_PACK_H
#define LUA_PACK_H

struct lua_State;

class LuaPack {
	public:
		static bool PushEntries(lua_State* L);

	private:
		static int l_pack(lua_State* L);
		static int l_unpack(lua_State* L);
};

#endif /* LUA_PACK_H */
